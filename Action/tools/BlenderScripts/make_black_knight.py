# 黒騎士（敵）のモデルを、SK_Player の骨格の上に形から作って FBX に書き出す。
#
# 使い方（Blender 5.2 で確認）:
#   blender -b --python make_black_knight.py -- <SK_Player.FBX> <出力フォルダ>
#
# - 骨格は SK_Player.FBX のものをそのまま使う（プレイヤーの体のメッシュだけを消す）。
#   ボーンの名前・ローカル姿勢が同じなので、プレイヤーのアニメーション FBX がそのまま流せる。
# - 鎧は骨ごとに固く付ける（1つの部品は1本の骨、肩当て・胸・マントだけは2〜4本に分けて付ける）。
# - テクスチャは色のマスを 4×4 に並べた1枚（albedo）と、同じ並びのメタリック・なめらかさ（spec）の1枚。
#   各部品は自分のマスの中で、上が明るく下が暗いグラデーションになるように UV を張る。
import bpy
import bmesh
import math
import os
import sys

import numpy as np
from mathutils import Matrix, Vector

argv = sys.argv[sys.argv.index("--") + 1:]
SRC_FBX = argv[0]
OUT_DIR = argv[1]
OUT_NAME = "BlackKnight"

# ---------------------------------------------------------------------------
# 色のマス（4×4、1マス 64px）
# ---------------------------------------------------------------------------
ARMOR, TRIM, GLOW, CAPE_OUT, CAPE_IN, LEATHER, BLADE, HORN = range(8)

# (上の色, 下の色)  0〜255
CELL_COLORS = {
    ARMOR: ((62, 62, 76), (24, 24, 30)),        # 黒い鋼
    TRIM: ((168, 166, 184), (92, 90, 106)),     # くすんだ銀の縁取り
    GLOW: ((240, 190, 255), (186, 92, 255)),    # 紫の光（目・胸の石・剣の溝）
    CAPE_OUT: ((40, 34, 48), (14, 12, 18)),     # マントの表（黒）
    CAPE_IN: ((150, 22, 38), (70, 8, 18)),      # マントの裏・前垂れ（深紅）
    LEATHER: ((66, 58, 62), (32, 28, 30)),      # 革（剣の柄・ベルト）
    BLADE: ((150, 154, 172), (78, 80, 96)),     # 刃（暗い鋼）
    HORN: ((52, 46, 58), (16, 14, 20)),         # 角
}
# (メタリック, なめらかさ)  0〜1。シェーダーは spec の r をメタリック、a をなめらかさとして読む。
CELL_SPEC = {
    ARMOR: (0.8, 0.62),
    TRIM: (1.0, 0.72),
    GLOW: (0.0, 0.0),
    CAPE_OUT: (0.0, 0.12),
    CAPE_IN: (0.0, 0.2),
    LEATHER: (0.2, 0.3),
    BLADE: (1.0, 0.82),
    HORN: (0.3, 0.55),
}
TEX_SIZE = 256
CELL_PX = 64
CELL_MARGIN = 6


def cell_uv(cell, frac_u, frac_v):
    """マスの中の位置（0〜1）を UV に直す。"""
    cx, cy = cell % 4, cell // 4
    inner = CELL_PX - CELL_MARGIN * 2
    u = (cx * CELL_PX + CELL_MARGIN + frac_u * inner) / TEX_SIZE
    v = (cy * CELL_PX + CELL_MARGIN + frac_v * inner) / TEX_SIZE
    return u, v


def make_textures():
    albedo = np.zeros((TEX_SIZE, TEX_SIZE, 4), dtype=np.float32)
    spec = np.zeros((TEX_SIZE, TEX_SIZE, 4), dtype=np.float32)
    for cell, (top, bottom) in CELL_COLORS.items():
        cx, cy = cell % 4, cell // 4
        x0, y0 = cx * CELL_PX, cy * CELL_PX
        for py in range(CELL_PX):
            f = min(max((py - CELL_MARGIN) / (CELL_PX - CELL_MARGIN * 2), 0.0), 1.0)
            col = [(bottom[i] + (top[i] - bottom[i]) * f) / 255.0 for i in range(3)]
            albedo[y0 + py, x0:x0 + CELL_PX, :3] = col
            albedo[y0 + py, x0:x0 + CELL_PX, 3] = 1.0
        metal, smooth = CELL_SPEC[cell]
        spec[y0:y0 + CELL_PX, x0:x0 + CELL_PX] = (metal, 0.0, 0.0, smooth)
    images = {}
    for name, arr in (("albedo", albedo), ("spec", spec)):
        img = bpy.data.images.new(f"{OUT_NAME}_{name}", TEX_SIZE, TEX_SIZE, alpha=True)
        img.colorspace_settings.name = "sRGB" if name == "albedo" else "Non-Color"
        img.pixels.foreach_set(arr.ravel())
        img.filepath_raw = os.path.join(OUT_DIR, f"{OUT_NAME}_{name}.png")
        img.file_format = "PNG"
        img.save()
        images[name] = img
    return images


# ---------------------------------------------------------------------------
# 骨格（SK_Player）を読み込む
# ---------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC_FBX)
ARM = next(o for o in bpy.data.objects if o.type == "ARMATURE")
for o in [o for o in bpy.data.objects if o.type == "MESH"]:
    bpy.data.objects.remove(o, do_unlink=True)


def bone_head(name):
    """ボーンの付け根のワールド座標（m）。前は -Y、上は +Z、キャラクターの左は +X。"""
    return ARM.matrix_world @ ARM.data.bones[name].head_local


def side_name(s, base):
    return f"J_Bip_{'L' if s > 0 else 'R'}_{base}"


# ---------------------------------------------------------------------------
# 形を作る道具
# ---------------------------------------------------------------------------
PARTS = []


def rigid(bone):
    return lambda p: {bone: 1.0}


def add_part(verts, faces, cell, weight, recalc=True, bevel=0.0, outward=None):
    """部品を1つ足す。outward を渡すと、面をその向き（面の中心→外向きのベクトル）にそろえる。"""
    PARTS.append(dict(verts=[Vector(v) for v in verts], faces=[tuple(f) for f in faces],
                      cell=cell, weight=weight, recalc=recalc, bevel=bevel, outward=outward))


def ring(center, u, v, ru, rv, n, phase=0.0):
    c, u, v = Vector(center), Vector(u), Vector(v)
    return [c + u * (ru * math.cos(phase + 2 * math.pi * k / n)) + v * (rv * math.sin(phase + 2 * math.pi * k / n))
            for k in range(n)]


def loft(rings, cap_start=True, cap_end=True, start_point=None, end_point=None):
    """同じ点数の輪を順につないだ筒。端は面でふさぐか、1点にすぼめる。"""
    n = len(rings[0])
    verts = [p for r in rings for p in r]
    faces = []
    for i in range(len(rings) - 1):
        for k in range(n):
            a, b = i * n + k, i * n + (k + 1) % n
            faces.append((a, b, b + n, a + n))
    if start_point is not None:
        idx = len(verts)
        verts.append(Vector(start_point))
        faces += [(idx, (k + 1) % n, k) for k in range(n)]
    elif cap_start:
        faces.append(tuple(reversed(range(n))))
    if end_point is not None:
        idx = len(verts)
        verts.append(Vector(end_point))
        base = (len(rings) - 1) * n
        faces += [(idx, base + k, base + (k + 1) % n) for k in range(n)]
    elif cap_end:
        base = (len(rings) - 1) * n
        faces.append(tuple(base + k for k in range(n)))
    return verts, faces


def frame_from_dir(d):
    d = Vector(d).normalized()
    helper = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
    u = d.cross(helper).normalized()
    v = d.cross(u).normalized()
    return u, v


def tube(points, radii, n=8, tip=True):
    """折れ線に沿って太さを変えながら伸ばす（角・剣の鍔など）。最後の太さが 0 なら先をとがらせる。"""
    pts = [Vector(p) for p in points]
    rings = []
    last = len(pts) - 1 if (tip and radii[-1] == 0) else len(pts)
    for i in range(last):
        d = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)])
        u, v = frame_from_dir(d)
        rings.append(ring(pts[i], u, v, radii[i], radii[i], n))
    if tip and radii[-1] == 0:
        return loft(rings, cap_start=True, end_point=pts[-1])
    return loft(rings)


def box(center, size, axes=None):
    """中心・大きさ（各軸の全長）・向き（3本の軸）で決める箱。"""
    c = Vector(center)
    ax = axes or (Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)))
    hx, hy, hz = (s / 2 for s in size)
    verts = [c + ax[0] * (sx * hx) + ax[1] * (sy * hy) + ax[2] * (sz * hz)
             for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)]
    faces = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    return verts, faces


def ellipsoid(center, radii, rings_n=8, seg=12, zmin=-1.0, matrix=None):
    """楕円体。zmin を上げると下を平らに切ったドームになる（肩当て）。"""
    rings = []
    a0 = math.asin(max(-1.0, min(1.0, zmin)))
    steps = rings_n
    for i in range(steps):
        a = a0 + (math.pi / 2 - a0) * i / steps
        rings.append([Vector((math.cos(a) * math.cos(t) * radii[0], math.cos(a) * math.sin(t) * radii[1],
                              math.sin(a) * radii[2]))
                      for t in (2 * math.pi * k / seg for k in range(seg))])
    verts, faces = loft(rings, cap_start=True, end_point=(0, 0, radii[2]))
    m = matrix or Matrix.Identity(4)
    c = Vector(center)
    return [c + (m @ v) for v in verts], faces


def octa(center, rx, ry, rz):
    c = Vector(center)
    verts = [c + Vector(v) for v in ((rx, 0, 0), (-rx, 0, 0), (0, ry, 0), (0, -ry, 0), (0, 0, rz), (0, 0, -rz))]
    faces = [(0, 2, 4), (2, 1, 4), (1, 3, 4), (3, 0, 4), (2, 0, 5), (1, 2, 5), (3, 1, 5), (0, 3, 5)]
    return verts, faces


def body_ring(z, rx, yf, yb, n=16, yc=0.0, keel=0.0, keel_width=1):
    """胴の断面。前（-Y）と後ろ（+Y）で奥行きを変え、前の真ん中を keel だけ前に出す（鎧の稜線）。"""
    pts = []
    for k in range(n):
        a = -math.pi / 2 + 2 * math.pi * k / n   # k=0 が真正面
        s = math.sin(a)
        y = yc + (yf if s < 0 else yb) * s
        if keel and (k <= keel_width - 1 or k >= n - keel_width + 1):
            y -= keel * (1.0 if k == 0 else 0.45)
        pts.append(Vector((rx * math.cos(a), y, z)))
    return pts


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def blend(z_hi_bone, z_lo_bone, z_lo, z_hi):
    def w(p):
        t = smoothstep(z_lo, z_hi, p.z)
        return {z_hi_bone: t, z_lo_bone: 1 - t}
    return w


# ---------------------------------------------------------------------------
# 部品
# ---------------------------------------------------------------------------
def build_helmet():
    head = "J_Bip_C_Head"
    yc = 0.005
    prof = [(1.36, .092, .102), (1.40, .108, .118), (1.46, .116, .126), (1.52, .116, .124),
            (1.57, .103, .110), (1.605, .072, .078), (1.625, .030, .034)]
    rings = []
    for z, rx, ry in prof:
        keel = 0.022 if 1.37 < z < 1.6 else 0.008
        rings.append(body_ring(z, rx, ry, ry * 0.98, n=16, yc=yc, keel=keel))
    v, f = loft(rings, cap_start=True, end_point=(0, yc, 1.632))
    add_part(v, f, ARMOR, rigid(head))
    # 首を覆う下の縁
    v, f = loft([body_ring(1.345, .098, .108, .104, yc=yc), body_ring(1.372, .100, .112, .108, yc=yc)])
    add_part(v, f, TRIM, rigid(head))
    # 目のすき間（V 字に吊り上がった光）と、その上の眉の縁取り
    for s in (1, -1):
        a = Vector((s * 0.012, -0.142, 1.462))
        b = Vector((s * 0.088, -0.098, 1.492))
        d = (b - a).normalized()
        n_out = Vector((s * 0.35, -1, 0)).normalized()
        up = d.cross(n_out).normalized()
        if up.z < 0:
            up = -up
        axes = (d, n_out, up)
        v, f = box((a + b) / 2, ((b - a).length, 0.02, 0.013), axes)
        add_part(v, f, GLOW, rigid(head))
        v, f = box((a + b) / 2 + up * 0.019 + n_out * 0.004, ((b - a).length * 1.12, 0.02, 0.012), axes)
        add_part(v, f, TRIM, rigid(head))
    # 角（こめかみから後ろへ反り上がる）
    for s in (1, -1):
        pts = [(s * .100, .000, 1.515), (s * .150, .025, 1.555), (s * .190, .070, 1.610),
               (s * .212, .135, 1.690), (s * .205, .200, 1.765), (s * .185, .245, 1.815)]
        v, f = tube(pts, [.030, .026, .020, .014, .008, 0], n=8)
        add_part(v, f, HORN, rigid(head))
        # 角の根元の輪
        v, f = tube([(s * .092, -.004, 1.508), (s * .118, .006, 1.528)], [.036, .036], n=8, tip=False)
        add_part(v, f, TRIM, rigid(head))
    # 額から頭頂への稜線
    v, f = tube([(0, -.150, 1.50), (0, -.128, 1.56), (0, -.085, 1.605), (0, -.02, 1.635), (0, .06, 1.62)],
                [.010, .010, .010, .009, 0], n=6)
    add_part(v, f, TRIM, rigid(head))


def build_torso():
    # 喉当て（首）
    v, f = loft([body_ring(1.29, .088, .090, .085, yc=.02), body_ring(1.385, .080, .084, .080, yc=.02)])
    add_part(v, f, ARMOR, rigid("J_Bip_C_Neck"))
    # 胸当て
    prof = [(1.335, .095, .070, .075), (1.310, .145, .105, .095), (1.250, .150, .125, .100),
            (1.180, .138, .122, .098), (1.110, .122, .106, .092), (1.060, .112, .096, .088)]
    rings = [body_ring(z, rx, yf, yb, keel=(0.018 if 1.07 < z < 1.32 else 0.0)) for z, rx, yf, yb in prof]
    v, f = loft(rings[::-1])
    add_part(v, f, ARMOR, blend("J_Bip_C_UpperChest", "J_Bip_C_Chest", 1.11, 1.21))
    # 胸当ての下の縁取り
    v, f = loft([body_ring(1.052, .116, .100, .092), body_ring(1.072, .118, .104, .094)])
    add_part(v, f, TRIM, rigid("J_Bip_C_Chest"))
    # 胸の紫の石と台座
    v, f = octa((0, -.150, 1.235), .042, .014, .052)
    add_part(v, f, TRIM, rigid("J_Bip_C_UpperChest"))
    v, f = octa((0, -.160, 1.235), .022, .010, .030)
    add_part(v, f, GLOW, rigid("J_Bip_C_UpperChest"))
    # 腹の板（2段）
    v, f = loft([body_ring(1.020, .114, .098, .090), body_ring(1.075, .110, .094, .088)])
    add_part(v, f, ARMOR, lambda p: {"J_Bip_C_Chest": .5, "J_Bip_C_Spine": .5})
    v, f = loft([body_ring(0.975, .116, .100, .092), body_ring(1.030, .111, .095, .089)])
    add_part(v, f, ARMOR, rigid("J_Bip_C_Spine"))
    # ベルトとバックル
    v, f = loft([body_ring(0.935, .118, .104, .097), body_ring(0.985, .117, .102, .095)])
    add_part(v, f, LEATHER, rigid("J_Bip_C_Hips"))
    v, f = box((0, -.112, .960), (.055, .018, .044))
    add_part(v, f, TRIM, rigid("J_Bip_C_Hips"), bevel=.004)
    v, f = octa((0, -.123, .960), .012, .006, .016)
    add_part(v, f, GLOW, rigid("J_Bip_C_Hips"))


def curved_plate(axis_xy, radius, a0, a1, z_top, z_bot, thick, seg=8, flare=0.0):
    """縦の軸のまわりに曲がった板（草摺）。a0〜a1 は軸から見た角度（-90° が前）。"""
    cx, cy = axis_xy
    outer, inner = [], []
    for k in range(seg + 1):
        a = math.radians(a0 + (a1 - a0) * k / seg)
        for z, r in ((z_top, radius), (z_bot, radius + flare)):
            outer.append(Vector((cx + r * math.cos(a), cy + r * math.sin(a), z)))
            inner.append(Vector((cx + (r - thick) * math.cos(a), cy + (r - thick) * math.sin(a), z)))
    verts = outer + inner
    n = len(outer)
    faces = []
    for k in range(seg):
        a, b, c, d = 2 * k, 2 * k + 2, 2 * k + 3, 2 * k + 1   # 外側: 上→上→下→下
        faces.append((a, d, c, b))
        faces.append((a + n, b + n, c + n, d + n))
    faces.append((0, n, n + 1, 1))
    faces.append((2 * seg, 2 * seg + 1, 2 * seg + 1 + n, 2 * seg + n))
    for k in range(seg):
        faces.append((2 * k, 2 * k + 2, 2 * k + 2 + n, 2 * k + n))
        faces.append((2 * k + 1, 2 * k + 1 + n, 2 * k + 3 + n, 2 * k + 3))
    return verts, faces


def build_skirt():
    # 草摺（太ももの前と外を覆う3段の板）。太ももと一緒に動く。
    for s in (1, -1):
        bone = side_name(s, "UpperLeg")
        axis = (s * 0.085, 0.0)
        a0, a1 = (-125, 5) if s > 0 else (-175, -55)
        for k, (zt, zb, r) in enumerate(((.945, .855, .112), (.875, .785, .108), (.805, .715, .104))):
            v, f = curved_plate(axis, r, a0, a1, zt, zb, .010, flare=.008)
            add_part(v, f, ARMOR, rigid(bone))
            v, f = curved_plate(axis, r + .004, a0, a1, zb + .012, zb, .012, flare=.008)
            add_part(v, f, TRIM, rigid(bone))
    # 前垂れ（深紅の布）。上は腰、下は両脚の真ん中について揺れる。
    build_sheet(lambda u, t: Vector((u * (0.075 - 0.012 * t), -0.112 - 0.018 * t - 0.01 * math.sin(t * 3),
                                     0.955 - t * 0.43)),
                cols=4, rows=6, outer_cell=CAPE_IN, inner_cell=CAPE_OUT, out_dir=lambda p: Vector((0, -1, 0)),
                weight=lambda p, u, t: _front_cloth_weight(t), hem=lambda c: 0.04 * ((c * 37) % 3) / 2)


def _front_cloth_weight(t):
    legs = 0.55 * smoothstep(0.25, 1.0, t)
    return {"J_Bip_C_Hips": 1 - legs, "J_Bip_L_UpperLeg": legs / 2, "J_Bip_R_UpperLeg": legs / 2}


def build_sheet(pos, cols, rows, outer_cell, inner_cell, out_dir, weight, thickness=0.007, hem=None):
    """布（表と裏の2枚）。pos(u, t) は u∈[-1,1]（横）、t∈[0,1]（上→下）で位置を返す。
    hem(列) を渡すと、列ごとに裾の長さを変えてギザギザにする。"""
    grid, params = [], []
    for c in range(cols + 1):
        u = -1 + 2 * c / cols
        t_end = 1.0 + (hem(c) if hem else 0.0)
        col = []
        for r in range(rows + 1):
            t = t_end * r / rows
            col.append(pos(u, t))
            params.append((u, t))
        grid.append(col)
    verts = [p for col in grid for p in col]
    faces = []
    for c in range(cols):
        for r in range(rows):
            a = c * (rows + 1) + r
            faces.append((a, a + 1, a + rows + 2, a + rows + 1))

    def oriented(fs, vs, sign):
        res = []
        for fc in fs:
            p = [vs[i] for i in fc]
            nrm = (p[1] - p[0]).cross(p[2] - p[0])
            ctr = sum(p, Vector()) / len(p)
            if nrm.dot(out_dir(ctr)) * sign < 0:
                fc = tuple(reversed(fc))
            res.append(fc)
        return res

    out_faces = oriented(faces, verts, 1)
    inner = [p - out_dir(p).normalized() * thickness for p in verts]
    in_faces = oriented(faces, inner, -1)
    wfun = {}
    for i, p in enumerate(verts):
        wfun[i] = weight(p, *params[i])
    add_part(verts, out_faces, outer_cell, lambda p, _w=wfun, _v=verts: _w[_nearest(_v, p)], recalc=False)
    add_part(inner, in_faces, inner_cell, lambda p, _w=wfun, _v=inner: _w[_nearest(_v, p)], recalc=False)


def _nearest(vs, p):
    best, bi = 1e9, 0
    for i, q in enumerate(vs):
        d = (q - p).length_squared
        if d < best:
            best, bi = d, i
    return bi


def build_cape():
    def theta_max(t):
        return math.radians(100 - 30 * smoothstep(0, 0.22, t))

    def pos(u, t):
        th = u * theta_max(t)
        r = 0.112 + 0.17 * smoothstep(0, 0.35, t) + 0.07 * t
        r += 0.016 * min(t * 3, 1) * math.sin(u * 5.0 * math.pi)
        yc = 0.012 + 0.085 * smoothstep(0, 0.18, t) + 0.13 * t
        z = 1.36 - t * 1.08
        return Vector((r * math.sin(th), yc + r * math.cos(th), z))

    def out_dir(p):
        return Vector((p.x, p.y - 0.03, 0)).normalized() if (p.x, p.y) != (0, 0.03) else Vector((0, 1, 0))

    def weight(p, u, t):
        legs = 0.42 * smoothstep(0.45, 1.0, t)
        chest = (1 - smoothstep(0.08, 0.5, t)) * (1 - legs)
        hips = (1 - legs) - chest
        side = min(max(0.5 + p.x / 0.3, 0.0), 1.0)
        return {"J_Bip_C_UpperChest": chest, "J_Bip_C_Hips": hips,
                "J_Bip_L_UpperLeg": legs * side, "J_Bip_R_UpperLeg": legs * (1 - side)}

    def hem(c):
        return (0.10 if c % 2 else 0.0) + 0.05 * ((c * 7919) % 5) / 4

    build_sheet(pos, cols=22, rows=14, outer_cell=CAPE_OUT, inner_cell=CAPE_IN, out_dir=out_dir,
                weight=weight, thickness=0.008, hem=hem)

    # 首の後ろで立ち上がる襟
    def collar(u, t):
        th = u * math.radians(118)
        r = 0.108 + 0.05 * t
        return Vector((r * math.sin(th), 0.03 + r * math.cos(th), 1.30 + t * 0.17))

    build_sheet(collar, cols=12, rows=3, outer_cell=CAPE_OUT, inner_cell=CAPE_IN,
                out_dir=lambda p: Vector((p.x, p.y - 0.03, 0)).normalized(),
                weight=lambda p, u, t: {"J_Bip_C_UpperChest": 1.0}, thickness=0.008)


def build_arms():
    for s in (1, -1):
        upper, lower, hand = side_name(s, "UpperArm"), side_name(s, "LowerArm"), side_name(s, "Hand")
        shoulder = side_name(s, "Shoulder")
        X = Vector((s, 0, 0))
        # 肩当て（3枚重ね、外へ傾ける）。腕と肩の骨の両方につけ、腕を下ろしたときに半分だけついていく。
        w_pauldron = lambda p, _u=upper, _s=shoulder: {_u: 0.6, _s: 0.4}
        for k, (cx, cz, radii, tilt) in enumerate((
                (0.150, 1.338, (.112, .128, .080), 22),
                (0.195, 1.292, (.090, .118, .064), 48),
                (0.228, 1.245, (.072, .104, .052), 70))):
            m = Matrix.Rotation(math.radians(-s * tilt), 4, "Y")
            v, f = ellipsoid((s * cx, .022, cz), radii, rings_n=6, seg=14, zmin=-0.25, matrix=m)
            add_part(v, f, ARMOR, w_pauldron)
            rim = [ring((0, 0, 0), (1, 0, 0), (0, 1, 0), radii[0] * 1.02 * math.cos(math.asin(-0.25)),
                        radii[1] * 1.02 * math.cos(math.asin(-0.25)), 14)]
            z0 = -0.25 * radii[2]
            rim_rings = [[(m @ (p + Vector((0, 0, z0 + dz)))) + Vector((s * cx, .022, cz)) for p in rim[0]]
                         for dz in (-0.004, 0.012)]
            v, f = loft(rim_rings, cap_start=False, cap_end=False)
            add_part(v, f, TRIM, w_pauldron)
        # 一番上の肩当ての尖り
        m = Matrix.Rotation(math.radians(-s * 22), 4, "Y")
        top = m @ Vector((0, 0, .08)) + Vector((s * .150, .022, 1.338))
        v, f = tube([top - Vector((0, 0, .01)), top + Vector((s * .03, .01, .06)), top + Vector((s * .045, .03, .10))],
                    [.022, .012, 0], n=6)
        add_part(v, f, TRIM, w_pauldron)
        # 二の腕
        y0, z0 = .025, 1.283
        v, f = loft([ring((s * x, y0, z0), (0, 1, 0), (0, 0, 1), ry, rz, 8, math.pi / 8)
                     for x, ry, rz in ((.110, .064, .060), (.215, .062, .058), (.315, .055, .052))])
        add_part(v, f, ARMOR, rigid(upper))
        # 肘当て
        v, f = ellipsoid((s * .330, .030, z0), (.058, .064, .060), rings_n=5, seg=10)
        add_part(v, f, ARMOR, rigid(lower))
        v, f = tube([(s * .330, .060, z0), (s * .345, .115, z0 + .005), (s * .360, .150, z0 + .01)],
                    [.022, .012, 0], n=6)
        add_part(v, f, TRIM, rigid(lower))
        # 前腕の籠手
        v, f = loft([ring((s * x, y0, z0), (0, 1, 0), (0, 0, 1), ry, rz, 8, math.pi / 8)
                     for x, ry, rz in ((.355, .057, .054), (.450, .054, .051), (.520, .049, .046))])
        add_part(v, f, ARMOR, rigid(lower))
        for x in (.358, .515):
            v, f = loft([ring((s * (x + dx), y0, z0), (0, 1, 0), (0, 0, 1), .059, .056, 8, math.pi / 8)
                         for dx in (-.006, .006)])
            add_part(v, f, TRIM, rigid(lower))
        # 手首から先（籠手の口と、握った拳）
        v, f = loft([ring((s * x, y0, z0), (0, 1, 0), (0, 0, 1), r, r * .95, 8, math.pi / 8)
                     for x, r in ((.520, .052), (.565, .066))])
        add_part(v, f, ARMOR, rigid(hand))
        v, f = box((s * .610, .012, 1.276), (.090, .096, .074))
        add_part(v, f, ARMOR, rigid(hand), bevel=.012)
        v, f = box((s * .632, .012, 1.312), (.052, .100, .016))
        add_part(v, f, TRIM, rigid(hand), bevel=.004)
        v, f = box((s * .590, -.046, 1.262), (.050, .030, .034))
        add_part(v, f, ARMOR, rigid(hand), bevel=.006)


def build_legs():
    for s in (1, -1):
        up, low, foot, toe = (side_name(s, n) for n in ("UpperLeg", "LowerLeg", "Foot", "ToeBase"))
        cx = s * 0.083
        # 腿当て
        v, f = loft([[Vector((p.x, p.y, p.z)) for p in body_ring(z, rx, ry, ry, n=10)]
                     for z, rx, ry in ((.870, .078, .082), (.700, .074, .078), (.560, .064, .068))])
        v = [p + Vector((cx, 0, 0)) for p in v]
        add_part(v, f, ARMOR, rigid(up))
        # 膝当てと、外側の扇の板
        v, f = ellipsoid((s * .077, -.046, .528), (.062, .048, .068), rings_n=5, seg=12)
        add_part(v, f, ARMOR, rigid(low))
        m = Matrix.Rotation(math.radians(s * 90), 4, "Y")
        v, f = ellipsoid((s * .125, -.015, .530), (.045, .040, .010), rings_n=3, seg=10, matrix=m)
        add_part(v, f, TRIM, rigid(low))
        # すね当て（前に稜線）
        rings = [body_ring(z, rx, ry, ry * .92, n=10, keel=.012) for z, rx, ry in
                 ((.505, .066, .070), (.330, .062, .066), (.150, .051, .054))]
        v, f = loft(rings[::-1])
        v = [p + Vector((s * .077, .010, 0)) for p in v]
        add_part(v, f, ARMOR, rigid(low))
        v, f = loft([body_ring(z, .069, .073, .068, n=10) for z in (.488, .505)])
        v = [p + Vector((s * .077, .010, 0)) for p in v]
        add_part(v, f, TRIM, rigid(low))
        # 鉄靴（かかと→つま先）
        prof = [(.070, .044, .075, .070), (.025, .052, .080, .080), (-.040, .053, .058, .058),
                (-.100, .046, .042, .042), (-.150, .032, .030, .030)]
        rings = [ring((s * .077, y, zc), (1, 0, 0), (0, 0, 1), wx, wz, 8, math.pi / 8) for y, wx, zc, wz in prof]
        v, f = loft(rings, cap_start=True, end_point=(s * .077, -.192, .018))
        add_part(v, f, ARMOR, lambda p, _f=foot, _t=toe: {_f: smoothstep(-0.10, -0.04, p.y),
                                                          _t: 1 - smoothstep(-0.10, -0.04, p.y)})
        v, f = loft([ring((s * .077, -.062 + dy, .060), (1, 0, 0), (0, 0, 1), .056, .062, 8, math.pi / 8)
                     for dy in (-.006, .006)])
        add_part(v, f, TRIM, rigid(foot))


def build_sword():
    hand = "J_Bip_R_Hand"
    fist = Vector((-.610, .012, 1.272))
    fwd, out = Vector((0, -1, 0)), Vector((-1, 0, 0))
    a = math.radians(35)
    d = (fwd * math.cos(a) + out * math.sin(a)).normalized()   # 刃の向き（腕を下ろすと前下を向く）
    w = (out - d * out.dot(d)).normalized()                     # 刃の幅の向き
    t = d.cross(w).normalized()                                 # 刃の厚みの向き
    P = lambda sd, sw=0.0, st=0.0: fist + d * sd + w * sw + t * st
    R = rigid(hand)
    # 柄頭・柄
    v, f = octa((0, 0, 0), .030, .030, .030)
    add_part([P(-.175) + (Matrix((w, t, d)).transposed() @ p) for p in v], f, TRIM, R)
    v, f = octa((0, 0, 0), .014, .014, .016)
    add_part([P(-.205) + (Matrix((w, t, d)).transposed() @ p) for p in v], f, GLOW, R)
    v, f = tube([P(-.160), P(0.0)], [.017, .017], n=8, tip=False)
    add_part(v, f, LEATHER, R)
    # 鍔（両端が刃の方へ反る）と、真ん中の紫の石
    pts = [P(.045, -.150), P(.020, -.105), P(.012, -.050), P(.012, 0), P(.012, .050), P(.020, .105), P(.045, .150)]
    v, f = tube(pts[:4], [.012, .016, .019, .019], n=6, tip=False)
    add_part(v, f, TRIM, R)
    v, f = tube(pts[3:], [.019, .019, .016, .012], n=6, tip=False)
    add_part(v, f, TRIM, R)
    for tip_from, tip_to in ((pts[1], pts[0]), (pts[5], pts[6])):
        v, f = tube([tip_from, tip_to, tip_to + (tip_to - tip_from).normalized() * .03], [.012, .009, 0], n=6)
        add_part(v, f, TRIM, R)
    v, f = box(P(.022), (.07, .040, .055), (w, t, d))
    add_part(v, f, ARMOR, R, bevel=.006)
    for st in (.021, -.021):
        v, f = octa((0, 0, 0), .016, .004, .022)
        add_part([P(.024, 0, st) + (Matrix((w, t, d)).transposed() @ p) for p in v], f, GLOW, R)
    # 刃（断面はひし形）
    rings = []
    for sd, hw, ht in ((.050, .034, .008), (.35, .031, .008), (.70, .027, .007), (.93, .018, .006), (1.04, .006, .003)):
        c = P(sd)
        rings.append([c + w * hw, c + t * ht, c - w * hw, c - t * ht])
    v, f = loft(rings, cap_start=True, end_point=P(1.10))
    add_part(v, f, BLADE, R)
    # 刃の溝の光（表と裏）
    for st in (.0068, -.0068):
        v, f = box(P(.43, 0, st), (.008, .002, .70), (w, t, d))
        add_part(v, f, GLOW, R)


build_helmet()
build_torso()
build_skirt()
build_cape()
build_arms()
build_legs()
build_sword()

# ---------------------------------------------------------------------------
# 1つのメッシュにまとめる
# ---------------------------------------------------------------------------
all_verts, all_faces, face_cells, face_part, vert_weights = [], [], [], [], []
part_zrange = []
for pi, part in enumerate(PARTS):
    bm = bmesh.new()
    bverts = [bm.verts.new(v) for v in part["verts"]]
    for fc in part["faces"]:
        try:
            bm.faces.new([bverts[i] for i in fc])
        except ValueError:
            pass
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    if part["recalc"]:
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if part["bevel"] > 0:
        bmesh.ops.bevel(bm, geom=list(bm.edges), offset=part["bevel"], segments=1, affect="EDGES",
                        profile=0.5, clamp_overlap=True)
    bm.verts.index_update()
    base = len(all_verts)
    zs = [v.co.z for v in bm.verts]
    part_zrange.append((min(zs), max(zs)))
    for v in bm.verts:
        all_verts.append(v.co.copy())
        vert_weights.append(part["weight"](v.co.copy()))
    for fc in bm.faces:
        all_faces.append(tuple(base + v.index for v in fc.verts))
        face_cells.append(part["cell"])
        face_part.append(pi)
    bm.free()

me = bpy.data.meshes.new(OUT_NAME)
me.from_pydata([tuple(v) for v in all_verts], [], all_faces)
me.update()
uv = me.uv_layers.new(name="UVMap")
for poly in me.polygons:
    zmin, zmax = part_zrange[face_part[poly.index]]
    for li in poly.loop_indices:
        z = all_verts[me.loops[li].vertex_index].z
        f = 0.5 if zmax - zmin < 1e-6 else (z - zmin) / (zmax - zmin)
        uv.data[li].uv = cell_uv(face_cells[poly.index], 0.5, f)
me.shade_smooth()
me.set_sharp_from_angle(angle=math.radians(38))

obj = bpy.data.objects.new(OUT_NAME, me)
bpy.context.scene.collection.objects.link(obj)
obj.parent = ARM
obj.matrix_parent_inverse = ARM.matrix_world.inverted()
for i, ws in enumerate(vert_weights):
    for bone, wt in ws.items():
        if wt <= 1e-4:
            continue
        vg = obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
        vg.add([i], wt, "REPLACE")
mod = obj.modifiers.new("Armature", "ARMATURE")
mod.object = ARM

# マテリアル（base color に albedo、specular IOR level に spec をつなぐと、FBX の DiffuseColor・SpecularFactor に入る。
# エンジンは名前に specular を含むテクスチャを spec マップとして読む）
images = make_textures()
mat = bpy.data.materials.new(OUT_NAME)
mat.use_nodes = True
nt = mat.node_tree
bsdf = nt.nodes["Principled BSDF"]
tex_a = nt.nodes.new("ShaderNodeTexImage")
tex_a.image = images["albedo"]
nt.links.new(tex_a.outputs["Color"], bsdf.inputs["Base Color"])
tex_s = nt.nodes.new("ShaderNodeTexImage")
tex_s.image = images["spec"]
nt.links.new(tex_s.outputs["Color"], bsdf.inputs["Specular IOR Level"])
me.materials.append(mat)

print("PARTS", len(PARTS), "VERTS", len(me.vertices), "TRIS", sum(len(p.vertices) - 2 for p in me.polygons))

out_fbx = os.path.join(OUT_DIR, f"{OUT_NAME}.fbx")
bpy.ops.export_scene.fbx(filepath=out_fbx, axis_forward="Y", axis_up="Z", add_leaf_bones=False,
                         primary_bone_axis="Y", secondary_bone_axis="X", bake_anim=False,
                         object_types={"EMPTY", "ARMATURE", "MESH"}, apply_unit_scale=True,
                         apply_scale_options="FBX_SCALE_NONE", use_armature_deform_only=False,
                         path_mode="STRIP", use_mesh_modifiers=False)
print("EXPORTED", out_fbx)
