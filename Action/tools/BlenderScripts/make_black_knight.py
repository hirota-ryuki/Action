# 黒騎士（敵）のモデルを、SK_Player の骨格の上に形から作って FBX に書き出す。
# 見た目は西洋の厳かな騎士（グレートヘルム・十字の紋章・長い前垂れとマント）。
# 色は主人公（紫の上着と飾り帯）に合わせ、くすんだ黒い鋼に古い金の縁取り・深い紫の布にする。
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
ARMOR, TRIM, GLOW, CAPE_OUT, CAPE_IN, LEATHER, BLADE, CLOTH = range(8)

# (上の色, 下の色)  0〜255
CELL_COLORS = {
    ARMOR: ((64, 66, 74), (24, 24, 30)),        # くすんだ黒い鋼
    TRIM: ((196, 164, 104), (112, 84, 44)),     # 古い金の縁取り
    GLOW: ((200, 160, 245), (128, 80, 200)),    # 紫の宝石（今は使っていない）
    CAPE_OUT: ((56, 40, 78), (22, 16, 32)),     # マントの表（深い紫）
    CAPE_IN: ((30, 26, 36), (14, 12, 18)),      # マントの裏・兜のすき間（暗い色）
    LEATHER: ((78, 54, 40), (38, 26, 20)),      # 革（剣の柄・ベルト）
    BLADE: ((214, 218, 230), (132, 136, 152)),  # 刃（明るい鋼）
    CLOTH: ((88, 58, 124), (42, 28, 64)),       # 前垂れの布（主人公の上着の紫を深くした色）
}
# (メタリック, なめらかさ)  0〜1。シェーダーは spec の r をメタリック、a をなめらかさとして読む。
CELL_SPEC = {
    ARMOR: (0.8, 0.62),
    TRIM: (1.0, 0.70),
    GLOW: (0.3, 0.8),
    CAPE_OUT: (0.0, 0.12),
    CAPE_IN: (0.0, 0.2),
    LEATHER: (0.2, 0.3),
    BLADE: (1.0, 0.85),
    CLOTH: (0.0, 0.18),
}
# 剣の柄の傾き。前（-Y）から指先の方へ何度傾けて握るか。make_black_knight_anims.py と合わせる。
SWORD_ANGLE = math.radians(20)
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
    # 西洋のグレートヘルム（筒形で頭頂は平ら。正面に稜線）
    prof = [(1.345, .106, .116), (1.40, .114, .124), (1.48, .118, .128), (1.56, .116, .126),
            (1.615, .110, .120), (1.640, .096, .106)]
    rings = [body_ring(z, rx, ry, ry * 0.98, n=16, yc=yc, keel=0.012) for z, rx, ry in prof]
    v, f = loft(rings, cap_start=True, end_point=(0, yc, 1.652))
    add_part(v, f, ARMOR, rigid(head))
    # 首を覆う下の縁
    v, f = loft([body_ring(1.330, .108, .120, .116, yc=yc), body_ring(1.355, .110, .123, .119, yc=yc)])
    add_part(v, f, TRIM, rigid(head))
    # 正面の金の縦の帯（目のすき間と金の眉で十字になる）
    for zs in ((1.360, 1.400, 1.440, 1.462), (1.494, 1.560, 1.615, 1.645)):
        front = {1.360: -.129, 1.400: -.137, 1.440: -.140, 1.462: -.141, 1.494: -.141, 1.560: -.139,
                 1.615: -.133, 1.645: -.118}
        v, f = tube([(0, front[z] - .004, z) for z in zs], [.010] * len(zs), n=6, tip=False)
        add_part(v, f, TRIM, rigid(head))
    v, f = tube([(0, -.118, 1.645), (0, -.060, 1.656), (0, .020, 1.656)], [.010, .010, .010], n=6, tip=False)
    add_part(v, f, TRIM, rigid(head))
    # 目のすき間（暗い横の切れ目）と、その上の金の眉
    for s in (1, -1):
        a = Vector((s * 0.012, -0.142, 1.478))
        b = Vector((s * 0.088, -0.088, 1.478))
        d = (b - a).normalized()
        n_out = Vector((s * 0.35, -1, 0)).normalized()
        up = d.cross(n_out).normalized()
        if up.z < 0:
            up = -up
        axes = (d, n_out, up)
        v, f = box((a + b) / 2, ((b - a).length, 0.02, 0.011), axes)
        add_part(v, f, CAPE_IN, rigid(head))
        v, f = box((a + b) / 2 + up * 0.019 + n_out * 0.004, ((b - a).length * 1.12, 0.02, 0.012), axes)
        add_part(v, f, TRIM, rigid(head))
    # 右の頬の息穴（暗い小さな切れ目）
    for k in range(3):
        v, f = box((-.058, -.106, 1.392 + k * .022), (.034, .02, .007), (Vector((.89, -.45, 0)).normalized(),
                                                                          Vector((-.45, -.89, 0)).normalized(),
                                                                          Vector((0, 0, 1))))
        add_part(v, f, CAPE_IN, rigid(head))


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
    # 胸の金の十字の紋章
    v, f = box((0, -.146, 1.205), (.024, .012, .150))
    add_part(v, f, TRIM, blend("J_Bip_C_UpperChest", "J_Bip_C_Chest", 1.11, 1.21), bevel=.003)
    v, f = box((0, -.148, 1.240), (.096, .012, .024))
    add_part(v, f, TRIM, rigid("J_Bip_C_UpperChest"), bevel=.003)
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
    # 前垂れ（紫の布、裾は真ん中が長い）。上は腰、下は両脚の真ん中について揺れる。
    build_sheet(lambda u, t: Vector((u * (0.092 - 0.010 * t), -0.112 - 0.018 * t - 0.01 * math.sin(t * 3),
                                     0.955 - t * 0.60)),
                cols=4, rows=8, outer_cell=CLOTH, inner_cell=CAPE_IN, out_dir=lambda p: Vector((0, -1, 0)),
                weight=lambda p, u, t: _front_cloth_weight(t), hem=lambda c: -0.05 * abs(c - 2) / 2)


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
        z = 1.36 - t * 1.24   # 裾は足首の高さまで
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
        u = -1 + 2 * c / 22
        return 0.06 * (1 - u * u)   # 真ん中が少し長い、整った裾

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
                (0.168, 1.335, (.132, .146, .084), 24),
                (0.214, 1.280, (.106, .134, .066), 50),
                (0.246, 1.230, (.084, .120, .054), 72))):
            m = Matrix.Rotation(math.radians(-s * tilt), 4, "Y")
            # 面を少なくして、丸みより板の角を見せる
            v, f = ellipsoid((s * cx, .022, cz), radii, rings_n=3, seg=10, zmin=-0.25, matrix=m)
            add_part(v, f, ARMOR, w_pauldron)
            rim = [ring((0, 0, 0), (1, 0, 0), (0, 1, 0), radii[0] * 1.02 * math.cos(math.asin(-0.25)),
                        radii[1] * 1.02 * math.cos(math.asin(-0.25)), 10)]
            z0 = -0.25 * radii[2]
            rim_rings = [[(m @ (p + Vector((0, 0, z0 + dz)))) + Vector((s * cx, .022, cz)) for p in rim[0]]
                         for dz in (-0.004, 0.012)]
            v, f = loft(rim_rings, cap_start=False, cap_end=False)
            add_part(v, f, TRIM, w_pauldron)
        # 二の腕
        y0, z0 = .025, 1.283
        v, f = loft([ring((s * x, y0, z0), (0, 1, 0), (0, 0, 1), ry, rz, 8, math.pi / 8)
                     for x, ry, rz in ((.110, .064, .060), (.215, .062, .058), (.315, .055, .052))])
        add_part(v, f, ARMOR, rigid(upper))
        # 肘当て
        v, f = ellipsoid((s * .330, .030, z0), (.058, .064, .060), rings_n=5, seg=10)
        add_part(v, f, ARMOR, rigid(lower))
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
        fist = Vector((s * .610, .012, 1.272))
        # 右手は柄（刃の向き）に合わせて拳を回し、柄の真ん中を握る。左手は回さない。
        rot = Matrix.Rotation(-SWORD_ANGLE, 3, "Z") if s < 0 else Matrix.Identity(3)
        axes = tuple(rot @ Vector(a) for a in ((1, 0, 0), (0, 1, 0), (0, 0, 1)))
        v, f = box(fist + rot @ Vector((0, 0, .004)), (.090, .092, .074), axes)
        add_part(v, f, ARMOR, rigid(hand), bevel=.012)
        v, f = box(fist + rot @ Vector((s * .022, 0, .040)), (.052, .096, .016), axes)
        add_part(v, f, TRIM, rigid(hand), bevel=.004)
        # 親指（柄の手前を上から押さえる）
        v, f = box(fist + rot @ Vector((-s * .012, -.050, .020)), (.050, .026, .030), axes)
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
    a = SWORD_ANGLE
    d = (fwd * math.cos(a) + out * math.sin(a)).normalized()   # 刃の向き（腕を下ろすと前下を向く）
    w = (out - d * out.dot(d)).normalized()                     # 刃の幅の向き
    t = d.cross(w).normalized()                                 # 刃の厚みの向き
    P = lambda sd, sw=0.0, st=0.0: fist + d * sd + w * sw + t * st
    R = rigid(hand)
    # 柄頭・柄（拳は柄の真ん中を握る）
    v, f = octa((0, 0, 0), .026, .026, .030)
    add_part([P(-.118) + (Matrix((w, t, d)).transposed() @ p) for p in v], f, TRIM, R)
    v, f = tube([P(-.105), P(.090)], [.016, .017], n=8, tip=False)
    add_part(v, f, LEATHER, R)
    # 金の鍔（まっすぐな十字の鍔）
    pts = [P(.100, -.150), P(.100, -.100), P(.100, -.050), P(.100, 0), P(.100, .050), P(.100, .100), P(.100, .150)]
    v, f = tube(pts[:4], [.012, .016, .019, .019], n=6, tip=False)
    add_part(v, f, TRIM, R)
    v, f = tube(pts[3:], [.019, .019, .016, .012], n=6, tip=False)
    add_part(v, f, TRIM, R)
    for end in (pts[0], pts[6]):
        v, f = octa((0, 0, 0), .020, .020, .020)
        add_part([end + (Matrix((w, t, d)).transposed() @ p) for p in v], f, TRIM, R)
    v, f = box(P(.110), (.07, .040, .050), (w, t, d))
    add_part(v, f, TRIM, R, bevel=.006)
    # 刃（断面はひし形）
    rings = []
    for sd, hw, ht in ((.130, .033, .008), (.43, .030, .008), (.78, .026, .007), (1.00, .017, .006), (1.10, .006, .003)):
        c = P(sd)
        rings.append([c + w * hw, c + t * ht, c - w * hw, c - t * ht])
    v, f = loft(rings, cap_start=True, end_point=P(1.16))
    add_part(v, f, BLADE, R)
    # 刃の根元の金の飾り（表と裏）
    for st in (.0066, -.0066):
        v, f = box(P(.20, 0, st), (.014, .002, .10), (w, t, d))
        add_part(v, f, TRIM, R)


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
