# 黒騎士（敵）のモデルを、SK_Player の骨格の上に形から作って FBX に書き出す。
# 見た目は「素早い暗殺者型の騎士」。7頭身強の細身で、薄い板を重ねた甲冑、後頭部から垂らしたフード、
# 腰から下の3枚の長い垂れ布、ゆるく反った細身の片手剣。曲線の紋様の一部を紫に光らせる。
#
# 使い方（Blender 5.1 で確認）:
#   blender -b --python make_black_knight.py -- <SK_Player.FBX> <出力フォルダ> [--stage N] [--preview <フォルダ>]
#
#   --stage N    1: シルエット（下地の体・兜・フード・垂れ布・剣の形）  2: 装甲の板
#                3: 装飾と発光（紋様・縁取り）  4: マテリアル（既定。FBX を書き出すのはこのときだけ）
#   --preview    その段階のモデルを正面・側面・斜めからレンダリングして <フォルダ>/stage<N>.png に書く。
#
# - 骨格は SK_Player.FBX のものを使い（プレイヤーの体のメッシュだけを消す）、揺れ用の骨を足す。
#     フード: J_Sec_Hood_1〜3（頭の子）  垂れ布: J_Sec_ClothF・ClothL・ClothR_1〜3（腰の子）
#     剣: J_Sec_Sword（右手の子。剣はこの骨に固く付く）
# - 鎧は骨ごとに固く付ける。体の下地・垂れ布・フードの尾は、骨の並びに沿って重みをなめらかに分ける。
# - テクスチャは色のマスを 4×4 に並べた1枚（albedo）と、同じ並びのメタリック・なめらかさ（spec）の1枚。
#   ゲームのエンジンには発光がないので、発光のマスは albedo を明るい紫にして代わりにする。
#   Blender のプレビュー（stage 4）では、発光（Emission）とトゥーン寄りの陰影・輪郭線を付けて描く。
import bpy
import bmesh
import math
import os
import sys

import numpy as np
from mathutils import Matrix, Vector
from mathutils.kdtree import KDTree

argv = sys.argv[sys.argv.index("--") + 1:]
SRC_FBX = argv[0]
OUT_DIR = argv[1]
STAGE = int(argv[argv.index("--stage") + 1]) if "--stage" in argv else 4
PREVIEW_DIR = argv[argv.index("--preview") + 1] if "--preview" in argv else None
OUT_NAME = "BlackKnight"

# ---------------------------------------------------------------------------
# 色のマス（4×4、1マス 64px）
# ---------------------------------------------------------------------------
ARMOR, TRIM, GLOW, CLOTH_OUT, CLOTH_IN, LEATHER, BLADE, VOID, SUIT = range(9)

# (上の色, 下の色)  0〜255
CELL_COLORS = {
    ARMOR: ((34, 34, 44), (20, 20, 27)),        # 本体の黒（#1a1a22 前後）
    TRIM: ((150, 126, 82), (98, 80, 50)),       # くすんだ金の縁取り・紋様
    GLOW: ((150, 108, 255), (124, 80, 245)),    # 紫の発光ライン（#8a5cff 前後）
    CLOTH_OUT: ((38, 32, 50), (19, 16, 26)),    # 垂れ布・フードの表（紫がかった黒）
    CLOTH_IN: ((22, 19, 28), (12, 10, 16)),     # 垂れ布・フードの裏
    LEATHER: ((46, 37, 33), (25, 20, 18)),      # 革（剣の柄・ベルト）
    BLADE: ((132, 136, 152), (78, 82, 96)),     # 刃（くすんだ鋼）
    VOID: ((6, 6, 9), (3, 3, 5)),               # 兜の目のすき間
    SUIT: ((30, 28, 36), (15, 14, 19)),         # 鎧の下の服（関節）
}
# (メタリック, なめらかさ)  0〜1。シェーダーは spec の r をメタリック、a をなめらかさとして読む。
# 金属はメタリック 0.6・ラフネス 0.5（なめらかさ 0.5）に抑える。
CELL_SPEC = {
    ARMOR: (0.6, 0.5),
    TRIM: (0.6, 0.5),
    GLOW: (0.0, 0.3),
    CLOTH_OUT: (0.0, 0.15),
    CLOTH_IN: (0.0, 0.1),
    LEATHER: (0.1, 0.3),
    BLADE: (0.6, 0.5),
    VOID: (0.0, 0.0),
    SUIT: (0.1, 0.25),
}
GLOW_COLOR = (0.54, 0.36, 1.0)   # #8a5cff
# 剣の柄の傾き。前（-Y）から指先の方へ何度傾けて握るか。make_black_knight_anims.py と合わせる。
SWORD_ANGLE = math.radians(20)
FIST_R = Vector((-.610, .012, 1.272))   # 右の拳の中心（T ポーズ）。make_black_knight_anims.py と同じ。
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


def make_textures(save=True):
    albedo = np.zeros((TEX_SIZE, TEX_SIZE, 4), dtype=np.float32)
    spec = np.zeros((TEX_SIZE, TEX_SIZE, 4), dtype=np.float32)
    emit = np.zeros((TEX_SIZE, TEX_SIZE, 4), dtype=np.float32)
    emit[..., 3] = 1.0
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
        if cell == GLOW:
            emit[y0:y0 + CELL_PX, x0:x0 + CELL_PX, :3] = 1.0
    images = {}
    for name, arr in (("albedo", albedo), ("spec", spec), ("emission", emit)):
        img = bpy.data.images.new(f"{OUT_NAME}_{name}", TEX_SIZE, TEX_SIZE, alpha=True)
        img.colorspace_settings.name = "sRGB" if name == "albedo" else "Non-Color"
        img.pixels.foreach_set(arr.ravel())
        if save and name != "emission":   # 発光のマスクはプレビュー用（ゲームは使わない）
            img.filepath_raw = os.path.join(OUT_DIR, f"{OUT_NAME}_{name}.png")
            img.file_format = "PNG"
            img.save()
        else:
            img.pack()
        images[name] = img
    return images


# ---------------------------------------------------------------------------
# 骨格（SK_Player）を読み込み、揺れ用・剣の骨を足す
# ---------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC_FBX)
ARM = next(o for o in bpy.data.objects if o.type == "ARMATURE")
for o in [o for o in bpy.data.objects if o.type == "MESH"]:
    bpy.data.objects.remove(o, do_unlink=True)


def sword_axes():
    """剣の向き。d: 刃の向き（拳から切っ先）、e: 刃の向き（刃のある側）、f: 刀身の平らな面の向き。"""
    fwd, out = Vector((0, -1, 0)), Vector((-1, 0, 0))
    d = (fwd * math.cos(SWORD_ANGLE) + out * math.sin(SWORD_ANGLE)).normalized()
    e = Vector((0, 0, -1))           # T ポーズでは刃が下、峰が上
    f = d.cross(e).normalized()
    return d, e, f


# (名前, 親, 付け根, 先)  座標はワールド（m）
EXTRA_BONES = [
    ("J_Sec_Hood_1", "J_Bip_C_Head", (0, .118, 1.470), (0, .122, 1.310)),
    ("J_Sec_Hood_2", "J_Sec_Hood_1", (0, .122, 1.310), (0, .124, 1.150)),
    ("J_Sec_Hood_3", "J_Sec_Hood_2", (0, .124, 1.150), (0, .126, 0.990)),
]
CLOTH_PANELS = {
    # 名前: (骨の付け根の x, y, 上の z, 裾の z)
    "F": (0.0, -.100, .962, .400),
    "L": (.072, .105, .962, .230),
    "R": (-.072, .105, .962, .230),
}
for key, (cx, cy, z0, z1) in CLOTH_PANELS.items():
    for k in range(3):
        za = z0 + (z1 - z0) * k / 3
        zb = z0 + (z1 - z0) * (k + 1) / 3
        parent = "J_Bip_C_Hips" if k == 0 else f"J_Sec_Cloth{key}_{k}"
        EXTRA_BONES.append((f"J_Sec_Cloth{key}_{k + 1}", parent, (cx, cy, za), (cx, cy, zb)))
_d, _e, _f = sword_axes()
EXTRA_BONES.append(("J_Sec_Sword", "J_Bip_R_Hand", tuple(FIST_R), tuple(FIST_R + _d * 0.2)))


def add_bones():
    ARM.select_set(True)
    bpy.context.view_layer.objects.active = ARM
    bpy.ops.object.mode_set(mode="EDIT")
    inv = ARM.matrix_world.inverted()
    eb = ARM.data.edit_bones
    for name, parent, head, tail in EXTRA_BONES:
        b = eb.new(name)
        b.head = inv @ Vector(head)
        b.tail = inv @ Vector(tail)
        b.roll = 0.0
        b.parent = eb[parent]
        b.use_connect = False
    bpy.ops.object.mode_set(mode="OBJECT")


add_bones()


def side_name(s, base):
    return f"J_Bip_{'L' if s > 0 else 'R'}_{base}"


# ---------------------------------------------------------------------------
# 形を作る道具
# ---------------------------------------------------------------------------
PARTS = []


def rigid(bone):
    return lambda p: {bone: 1.0}


def add_part(verts, faces, cell, weight, stage, recalc=True, bevel=0.0):
    if stage > STAGE:
        return
    PARTS.append(dict(verts=[Vector(v) for v in verts], faces=[tuple(f) for f in faces],
                      cell=cell, weight=weight, recalc=recalc, bevel=bevel))


def lerp(a, b, t):
    return a + (b - a) * t


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def interp_table(table, z):
    """(z, 値...) を z の小さい順に並べた表から、z の値を直線で補間する。"""
    if z <= table[0][0]:
        return table[0][1:]
    for a, b in zip(table, table[1:]):
        if z <= b[0]:
            t = (z - a[0]) / (b[0] - a[0])
            return tuple(lerp(x, y, t) for x, y in zip(a[1:], b[1:]))
    return table[-1][1:]


def chain(coord, points):
    """骨の並びに沿った重み。points は (座標, 骨) を座標の順に並べたもの。間は直線で分ける。"""
    if coord <= points[0][0]:
        return {points[0][1]: 1.0}
    for (ca, ba), (cb, bb) in zip(points, points[1:]):
        if coord <= cb:
            t = (coord - ca) / (cb - ca)
            if ba == bb:
                return {ba: 1.0}
            return {ba: 1 - t, bb: t}
    return {points[-1][1]: 1.0}


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


def box(center, size, axes=None):
    """中心・大きさ（各軸の全長）・向き（3本の軸）で決める箱。"""
    c = Vector(center)
    ax = axes or (Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)))
    hx, hy, hz = (s / 2 for s in size)
    verts = [c + ax[0] * (sx * hx) + ax[1] * (sy * hy) + ax[2] * (sz * hz)
             for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)]
    faces = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    return verts, faces


def surf_normal(pos, u, t, out_hint):
    e = 1e-3
    du = Vector(pos(u + e, t)) - Vector(pos(u - e, t))
    dt = Vector(pos(u, t + e)) - Vector(pos(u, t - e))
    n = du.cross(dt)
    hint = out_hint(Vector(pos(u, t)))
    if n.length < 1e-10:
        n = hint.copy()
    n.normalize()
    if n.dot(hint) < 0:
        n = -n
    return n


PLATE_RES = 1.05   # 板の分割の細かさ（1 で元の分割）。ポリゴン数は主人公（VRoid）の約4万1千までにおさめる


def plate(pos, cols, rows, thick, cell, weight, out_hint, stage=2):
    """薄い板。pos(u, t) は u∈[-1,1]（横）・t∈[0,1]（縦）で表の面の位置を返す。裏は内側へ thick だけ下げ、縁も閉じる。"""
    cols = max(2, round(cols * PLATE_RES))
    rows = max(1, round(rows * PLATE_RES))
    G = [Vector(pos(-1 + 2 * c / cols, r / rows)) for c in range(cols + 1) for r in range(rows + 1)]
    N = [surf_normal(pos, -1 + 2 * c / cols, r / rows, out_hint) for c in range(cols + 1) for r in range(rows + 1)]
    inner = [p - n * thick for p, n in zip(G, N)]
    M = len(G)

    def idx(c, r):
        return c * (rows + 1) + r

    faces = []
    for c in range(cols):
        for r in range(rows):
            q = (idx(c, r), idx(c + 1, r), idx(c + 1, r + 1), idx(c, r + 1))
            faces.append(q)
            faces.append(tuple(i + M for i in reversed(q)))
    for c in range(cols):
        for r in (0, rows):
            a, b = idx(c, r), idx(c + 1, r)
            faces.append((a, b, b + M, a + M))
    for r in range(rows):
        for c in (0, cols):
            a, b = idx(c, r), idx(c, r + 1)
            faces.append((a, b, b + M, a + M))
    add_part(G + inner, faces, cell, weight, stage)


def ribbon(points, normals, width, height, cell, weight, stage, base=0.0004, taper=True):
    """点の並びに沿った細い帯（紋様・縁取り・鍔）。normals の向きに height だけ盛り上げる。"""
    pts = [Vector(p) for p in points]
    rings = []
    n = len(pts)
    for i, (p, nr) in enumerate(zip(pts, normals)):
        tng = pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]
        b = tng.cross(nr).normalized()
        s = i / (n - 1)
        w = width * ((0.35 + 0.65 * math.sin(math.pi * s)) if taper else 1.0) / 2
        lo, hi = nr * base, nr * (base + height)
        rings.append([p + lo - b * w, p + lo + b * w, p + hi + b * w, p + hi - b * w])
    v, f = loft(rings)
    add_part(v, f, cell, weight, stage)


def bezier(p0, p1, p2, p3, n=18):
    pts = []
    for i in range(n + 1):
        s = i / n
        a, b, c, d = (1 - s) ** 3, 3 * s * (1 - s) ** 2, 3 * s * s * (1 - s), s ** 3
        pts.append(tuple(a * x0 + b * x1 + c * x2 + d * x3 for x0, x1, x2, x3 in zip(p0, p1, p2, p3)))
    return pts


def inlay(pos, uv_path, width, height, cell, weight, out_hint, stage=3, taper=True):
    """面（pos）の上に、uv の道筋に沿って細い帯を貼る。"""
    pts = [Vector(pos(u, t)) for u, t in uv_path]
    nrm = [surf_normal(pos, u, t, out_hint) for u, t in uv_path]
    ribbon(pts, nrm, width, height, cell, weight, stage, taper=taper)


def edge_trim(pos, t, weight, out_hint, u0=-1.0, u1=1.0, width=0.008, height=0.0025, n=24, stage=3):
    """板の上下の縁（t=0 か t=1）に沿った金の縁取り。"""
    path = [(lerp(u0, u1, i / n), t) for i in range(n + 1)]
    inlay(pos, path, width, height, TRIM, weight, out_hint, stage, taper=False)


def radial_hint(center, axis=None):
    """center から外へ向かう向き（axis を渡すと、その軸からの向き）。板の表裏を決めるのに使う。"""
    c = Vector(center)

    def f(p):
        v = p - c
        if axis is not None:
            a = Vector(axis).normalized()
            v = v - a * v.dot(a)
        return v.normalized() if v.length > 1e-9 else Vector((0, 0, 1))
    return f


# ---------------------------------------------------------------------------
# 体の面（胴・脚・腕・兜）。角度 θ は前（-Y）が 0、キャラクターの左（+X）へ回ると正。
# ---------------------------------------------------------------------------
# (z, 横の半径, 前の奥行き, 後ろの奥行き, 中心の y)
TORSO = [
    (0.84, .108, .078, .082, .010),
    (0.90, .114, .082, .088, .010),
    (0.96, .106, .079, .081, .005),
    (1.02, .096, .074, .074, .000),
    (1.08, .103, .082, .078, .000),
    (1.16, .118, .092, .084, .000),
    (1.23, .126, .096, .086, .005),
    (1.29, .118, .088, .082, .015),
    (1.33, .088, .068, .070, .025),
    (1.36, .056, .052, .056, .030),
]


def torso_pt(th, z, off=0.0):
    rx, yf, yb, yc = interp_table(TORSO, z)
    c = math.cos(th)
    y = yc - (yf + off) * c if c > 0 else yc - (yb + off) * c
    return Vector(((rx + off) * math.sin(th), y, z))


TORSO_HINT = radial_hint((0, 0.01, 0), axis=(0, 0, 1))

# 脚 (z, 横の半径, 前後の半径, 中心の y)
LEG = [
    (0.05, .030, .032, .040),
    (0.15, .035, .038, .030),
    (0.30, .045, .049, .018),
    (0.42, .049, .053, .010),
    (0.52, .044, .047, .006),
    (0.62, .052, .055, .004),
    (0.75, .058, .062, .002),
    (0.86, .064, .068, .000),
    (0.93, .068, .072, .000),
]


def leg_pt(s, th, z, off=0.0):
    """θ は前が 0、外側へ回ると正。"""
    rx, ry, yc = interp_table(LEG, z)
    return Vector((s * (.077 + (rx + off) * math.sin(th)), yc - (ry + off) * math.cos(th), z))


def leg_hint(s):
    return lambda p: Vector((p.x - s * .077, p.y - interp_table(LEG, p.z)[2], 0)).normalized()


# 腕 (腕に沿った距離 x, 前後の半径, 上下の半径)。中心は y=.025, z=1.283。
ARM_T = [
    (0.06, .056, .058),
    (0.12, .050, .052),
    (0.20, .045, .046),
    (0.30, .040, .040),
    (0.335, .035, .036),
    (0.37, .038, .038),
    (0.45, .035, .034),
    (0.53, .029, .029),
    (0.56, .031, .030),
]
ARM_Y, ARM_Z = .025, 1.283


def arm_pt(s, ph, x, off=0.0):
    """φ は上が 0、前（-Y）へ回ると正。"""
    ry, rz = interp_table(ARM_T, x)
    return Vector((s * x, ARM_Y - (ry + off) * math.sin(ph), ARM_Z + (rz + off) * math.cos(ph)))


def arm_hint(p):
    return Vector((0, p.y - ARM_Y, p.z - ARM_Z)).normalized()


# 兜 (z, 横の半径, 前の奥行き, 後ろの奥行き, 中心の y)
HELM = [
    (1.415, .060, .074, .068, .014),
    (1.445, .071, .090, .082, .012),
    (1.490, .077, .097, .091, .012),
    (1.535, .079, .098, .095, .013),
    (1.575, .076, .093, .094, .015),
    (1.605, .066, .081, .085, .017),
    (1.625, .048, .060, .066, .018),
    (1.634, .030, .036, .040, .018),
    (1.642, .010, .012, .014, .018),
]
HELM_TOP = (0, .018, 1.636)
HEAD_HINT = radial_hint((0, .015, 1.52))


def helm_pt(th, z, off=0.0, keel=0.0):
    rx, yf, yb, yc = interp_table(HELM, z)
    c = math.cos(th)
    k = keel * max(0.0, 1 - abs(th) / 0.50)
    y = yc - (yf + off + k) * c if c > 0 else yc - (yb + off) * c
    return Vector(((rx + off) * math.sin(th), y, z))


SPINE_CHAIN = [(0.93, "J_Bip_C_Hips"), (0.99, "J_Bip_C_Spine"), (1.06, "J_Bip_C_Spine"),
               (1.11, "J_Bip_C_Chest"), (1.17, "J_Bip_C_Chest"), (1.22, "J_Bip_C_UpperChest"),
               (1.32, "J_Bip_C_UpperChest"), (1.37, "J_Bip_C_Neck")]


# ---------------------------------------------------------------------------
# 部品
# ---------------------------------------------------------------------------
def build_body():
    """①鎧の下の服（関節は薄い服だけにして、跳ぶ・回るときに邪魔しない）。"""
    n = 28
    zs = [0.84, 0.87, 0.90, 0.93, 0.96, 0.99, 1.02, 1.05, 1.08, 1.12, 1.16, 1.20, 1.23, 1.26, 1.29, 1.31,
          1.33, 1.35, 1.36]
    rings = [[torso_pt(2 * math.pi * k / n, z) for k in range(n)] for z in zs]
    v, f = loft(rings)
    add_part(v, f, SUIT, lambda p: chain(p.z, SPINE_CHAIN), stage=1)
    # 首（兜と喉当ての中）
    rings = [[Vector((.050 * math.sin(a), .028 - .050 * math.cos(a), z)) for a in (2 * math.pi * k / 12 for k in range(12))]
             for z in (1.33, 1.38, 1.44)]
    v, f = loft(rings)
    add_part(v, f, SUIT, lambda p: chain(p.z, [(1.34, "J_Bip_C_UpperChest"), (1.38, "J_Bip_C_Neck"),
                                               (1.41, "J_Bip_C_Neck"), (1.43, "J_Bip_C_Head")]), stage=1)
    # 脚
    for s in (1, -1):
        up, low, foot = side_name(s, "UpperLeg"), side_name(s, "LowerLeg"), side_name(s, "Foot")
        rings = [[leg_pt(s, 2 * math.pi * k / 16, z) for k in range(16)]
                 for z in (0.93, 0.86, 0.75, 0.66, 0.58, 0.54, 0.50, 0.46, 0.40, 0.30, 0.20, 0.13)]
        v, f = loft(rings)
        add_part(v, f, SUIT, lambda p, _u=up, _l=low, _f=foot: chain(-p.z, [(-0.57, _u), (-0.49, _l), (-0.17, _l),
                                                                             (-0.11, _f)]), stage=1)
    # 腕
    for s in (1, -1):
        sh, up, low, hand = (side_name(s, n_) for n_ in ("Shoulder", "UpperArm", "LowerArm", "Hand"))
        rings = [[arm_pt(s, 2 * math.pi * k / 16, x) for k in range(16)]
                 for x in (0.06, 0.10, 0.16, 0.24, 0.30, 0.335, 0.37, 0.45, 0.53, 0.56)]
        v, f = loft(rings)
        add_part(v, f, SUIT, lambda p, _s=sh, _u=up, _l=low, _h=hand: chain(abs(p.x), [
            (0.07, _s), (0.12, _u), (0.31, _u), (0.35, _l), (0.52, _l), (0.55, _h)]), stage=1)


def build_helmet():
    """顔の見えない細身のフルフェイスの兜。前に稜線、面頬と眉庇の板の間に細い目のすき間。"""
    head = rigid("J_Bip_C_Head")
    n = 32
    zs = [1.415, 1.425, 1.44, 1.46, 1.48, 1.50, 1.52, 1.54, 1.56, 1.58, 1.595, 1.607, 1.618, 1.628]
    rings = [[helm_pt(2 * math.pi * k / n - math.pi, z, keel=.014 * (1 - smoothstep(1.58, 1.625, z))) for k in range(n)]
             for z in zs]
    v, f = loft(rings, cap_start=True, end_point=HELM_TOP)
    add_part(v, f, ARMOR, head, stage=1)

    # 面頬（顔の下半分を覆う板。上の縁は横へ下がり、下はあごへすぼまる）
    def visor(u, t):
        th = u * math.radians(64)
        zt = 1.521 + 0.010 * u * u
        zb = 1.408 + 0.034 * u * u
        return helm_pt(th, lerp(zt, zb, t), .005, keel=.022)
    plate(visor, 16, 6, .004, ARMOR, head, HEAD_HINT)

    # 眉庇（目の上の板。下の縁は横へ上がる）
    def brow(u, t):
        th = u * math.radians(76)
        zt = 1.612 - 0.022 * u * u
        zb = 1.546 + 0.014 * u * u
        return helm_pt(th, lerp(zt, zb, t), .005, keel=.017)
    plate(brow, 16, 4, .004, ARMOR, head, HEAD_HINT)

    # 目のすき間（暗い細い帯）
    def slit_surf(u, t):
        return helm_pt(u * math.radians(76), 1.5335 + 0.012 * u * u + (t - .5) * .01, .004, keel=.018)
    inlay(slit_surf, [(lerp(-.70, .70, i / 20), .5) for i in range(21)], .008, .0015, VOID, head, HEAD_HINT,
          stage=2, taper=False)
    # 縁取り（面頬の上・眉庇の下）
    edge_trim(visor, 0.0, head, HEAD_HINT, u0=-.9, u1=.9, width=.006)
    edge_trim(brow, 1.0, head, HEAD_HINT, u0=-.9, u1=.9, width=.006)



def build_hood():
    """後頭部から垂らしたフード。頭の後ろ半分を覆い、尾は背中へ垂れて揺れる。"""
    head = rigid("J_Bip_C_Head")

    def cap(u, t):
        z = lerp(1.646, 1.418, t)
        tw = math.radians(lerp(124, 100, smoothstep(0, 1, t)))
        th = math.pi + u * tw
        fold = .0035 * t * math.sin(u * 7 * math.pi)   # 下へ行くほど深くなるひだ
        return helm_pt(th, min(z, 1.642), .014 + .006 * t + fold)
    plate(cap, 22, 10, .006, CLOTH_OUT, head, HEAD_HINT, stage=1)

    def tail(u, t):
        z = 1.48 - t * (0.50 + 0.05 * (1 - u * u))   # 裾は真ん中が長い
        rx, yf, yb, yc = interp_table(TORSO, z)
        y_body = max(yc + yb, .098)
        w = lerp(.082, .052, t)
        fold = .006 * math.sin(u * 2.5 * math.pi) * (0.3 + 0.7 * t)
        z -= rag(u, 3.1) * smoothstep(.82, 1.0, t)
        return Vector((u * w, y_body + .022 + .012 * t - .022 * u * u + fold, z))

    hood_chain = [(-1.47, "J_Bip_C_Head"), (-1.43, "J_Sec_Hood_1"), (-1.33, "J_Sec_Hood_1"),
                  (-1.25, "J_Sec_Hood_2"), (-1.17, "J_Sec_Hood_2"), (-1.08, "J_Sec_Hood_3")]
    plate(tail, 12, 14, .006, CLOTH_OUT, lambda p: chain(-p.z, hood_chain),
          lambda p: Vector((0, 1, 0)), stage=1)
    edge_trim(tail, 0.78, lambda p: chain(-p.z, hood_chain), lambda p: Vector((0, 1, 0)), u0=-.92, u1=.92,
              width=.006)


def rag(u, seed):
    """裂けた裾の、列ごとの長さの差（m）。不ぞろいな舌の形になる。"""
    return (.040 * abs(math.sin(u * 7.3 + seed)) ** 3 + .016 * abs(math.sin(u * 17.9 + seed * 2.3))
            + .010 * abs(math.sin(u * 31.7 + seed * 4.1)))


def build_cloth():
    """腰から下の3枚の長い垂れ布（前1枚・後ろ2枚）。それぞれ揺れ用の3本の骨に付く。"""
    def panel_weight(key):
        cx, cy, z0, z1 = CLOTH_PANELS[key]
        L = z0 - z1
        pts = [(0.0, "J_Bip_C_Hips"), (0.05, f"J_Sec_Cloth{key}_1"), (1 / 6, f"J_Sec_Cloth{key}_1"),
               (0.5, f"J_Sec_Cloth{key}_2"), (5 / 6, f"J_Sec_Cloth{key}_3")]
        return lambda p: chain((z0 - p.z) / L, pts)

    def front(u, t):
        z = .968 - t * (0.50 + 0.08 * (1 - u * u))
        fold = .006 * math.sin(u * 3 * math.pi) * (0.2 + 0.8 * t)
        z -= rag(u, 0.7) * smoothstep(.84, 1.0, t)
        return Vector((u * lerp(.080, .056, t), -.098 - .030 * t + .018 * u * u - fold, z))
    plate(front, 14, 16, .006, CLOTH_OUT, panel_weight("F"), lambda p: Vector((0, -1, 0)), stage=1)
    edge_trim(front, .80, panel_weight("F"), lambda p: Vector((0, -1, 0)), u0=-.92, u1=.92, width=.007)

    for key, s in (("L", 1), ("R", -1)):
        def back(u, t, _s=s):
            z = .968 - t * (0.70 - 0.09 * (_s * u))   # 内側が長く、外側へ上がる裾
            fold = .007 * math.sin(u * 3 * math.pi + _s) * (0.2 + 0.8 * t)
            z -= rag(u, 1.9 + _s) * smoothstep(.86, 1.0, t)
            return Vector((_s * .070 + u * lerp(.072, .062, t), .100 + .040 * t - .016 * u * u + fold, z))
        plate(back, 14, 18, .006, CLOTH_OUT, panel_weight(key), lambda p: Vector((0, 1, 0)), stage=1)
        edge_trim(back, .82, panel_weight(key), lambda p: Vector((0, 1, 0)), u0=-.92, u1=.92, width=.007)


def build_torso_armor():
    """胸・腹・背の板。胸は2枚、腹は3枚を重ね、下の縁を曲線にする。"""
    def breast(u, t, off=0.0):
        th = u * math.radians(86)
        zt = 1.316 + 0.024 * u * u
        zb = 1.100 + 0.080 * u * u
        keel = .016 * max(0.0, 1 - abs(u) * 4)
        return torso_pt(th, lerp(zt, zb, t), .020 + off + keel)
    w_breast = lambda p: chain(p.z, [(1.12, "J_Bip_C_Chest"), (1.22, "J_Bip_C_UpperChest")])
    plate(breast, 20, 10, .005, ARMOR, w_breast, TORSO_HINT)

    def collar(u, t):
        th = u * math.radians(70)
        zt = 1.338 + 0.010 * u * u
        zb = 1.262 + 0.036 * u * u
        keel = .016 * max(0.0, 1 - abs(u) * 4)
        return torso_pt(th, lerp(zt, zb, t), .030 + keel)
    plate(collar, 16, 5, .005, ARMOR, rigid("J_Bip_C_UpperChest"), TORSO_HINT)
    edge_trim(collar, 1.0, rigid("J_Bip_C_UpperChest"), TORSO_HINT)

    for k, bone_w in enumerate((lambda p: {"J_Bip_C_Chest": 1.0},
                                lambda p: {"J_Bip_C_Chest": .5, "J_Bip_C_Spine": .5},
                                lambda p: {"J_Bip_C_Spine": 1.0})):
        def ab(u, t, _k=k):
            th = u * math.radians(80 - 5 * _k)
            zt = 1.122 - 0.044 * _k + 0.020 * u * u
            zb = 1.058 - 0.044 * _k + 0.028 * u * u
            return torso_pt(th, lerp(zt, zb, t), .013 - .002 * _k)
        plate(ab, 14, 3, .004, ARMOR, bone_w, TORSO_HINT)

    def back(u, t):
        th = math.pi + u * math.radians(96)
        zt = 1.322
        zb = 1.080 + 0.050 * u * u
        return torso_pt(th, lerp(zt, zb, t), .013)
    plate(back, 18, 8, .005, ARMOR, w_breast, TORSO_HINT)
    for s in (1, -1):
        inlay(back, bezier((s * .22, .85), (s * .30, .45), (s * .55, .40), (s * .78, .12)), .007, .0025, TRIM,
              w_breast, TORSO_HINT)
        inlay(back, bezier((s * .30, .80), (s * .38, .55), (s * .55, .52), (s * .68, .30), n=12), .0045, .0025, GLOW,
              w_breast, TORSO_HINT)

    # ベルト
    def belt(u, t):
        return torso_pt(u * math.pi, lerp(.992, .946, t), .007)
    plate(belt, 32, 2, .004, LEATHER, rigid("J_Bip_C_Hips"), TORSO_HINT)

    def buckle(u, t):
        th = u * math.radians(13)
        zt = 1.000 - .010 * u * u
        zb = .936 + .012 * u * u
        return torso_pt(th, lerp(zt, zb, t), .012 + .004 * (1 - u * u))
    plate(buckle, 8, 4, .004, TRIM, rigid("J_Bip_C_Hips"), TORSO_HINT)
    inlay(buckle, [(0, lerp(.18, .82, i / 8)) for i in range(9)], .004, .002, GLOW, rigid("J_Bip_C_Hips"), TORSO_HINT)

    # 腰の横の板（2枚重ね）
    for s in (1, -1):
        for k, (zt, zb, off) in enumerate(((.978, .890, .018), (.928, .840, .011))):
            def fauld(u, t, _s=s, _zt=zt, _zb=zb, _off=off):
                th = _s * (math.radians(92) + u * math.radians(40))
                return torso_pt(th, lerp(_zt, _zb - .02 * (1 - u * u), t), _off + .008 * t)
            w = rigid("J_Bip_C_Hips") if k == 0 else (lambda p, _s=s: {"J_Bip_C_Hips": .5, side_name(_s, "UpperLeg"): .5})
            plate(fauld, 10, 4, .004, ARMOR, w, TORSO_HINT)
            edge_trim(fauld, 1.0, w, TORSO_HINT, width=.006)

    # 胸の紋様（左右の渦の曲線は金、真ん中の稜線と内側の曲線は紫に光る）
    inlay(breast, [(0, lerp(.10, .78, i / 20)) for i in range(21)], .006, .0025, GLOW, w_breast, TORSO_HINT)
    for s in (1, -1):
        # 下の真ん中から、上の外（肩）へ流れ上がる曲線
        a = bezier((s * .05, .84), (s * .34, .72), (s * .28, .30), (s * .74, .12))
        inlay(breast, a, .007, .0025, TRIM, w_breast, TORSO_HINT)
        g = bezier((s * .07, .64), (s * .24, .56), (s * .20, .32), (s * .50, .20), n=14)
        inlay(breast, g, .0045, .0025, GLOW, w_breast, TORSO_HINT)

    # 喉当て（内の1枚）と、首の横と後ろを囲む立ち襟（前は V に下がる）
    def gorget(u, t):
        a = u * math.pi
        r = lerp(.060, .054, t)
        return Vector((r * math.sin(a), .028 - r * math.cos(a), lerp(1.350, 1.428, t)))
    plate(gorget, 24, 3, .004, ARMOR, rigid("J_Bip_C_Neck"), radial_hint((0, .028, 0), axis=(0, 0, 1)))

    def high_collar(u, t):
        a = u * math.pi
        z1 = 1.352 + .046 * (1 - math.cos(a)) / 2
        r = lerp(.098, .074, t) + .006 * (1 - math.cos(a)) / 2 * t
        return Vector((r * math.sin(a), .030 - r * math.cos(a), lerp(1.296, z1, t)))
    w_col = lambda p: {"J_Bip_C_UpperChest": .6, "J_Bip_C_Neck": .4}
    plate(high_collar, 28, 4, .004, ARMOR, w_col, radial_hint((0, .030, 0), axis=(0, 0, 1)))
    edge_trim(high_collar, 1.0, w_col, radial_hint((0, .030, 0), axis=(0, 0, 1)), n=40)


def build_arm_armor():
    for s in (1, -1):
        sh, up, low, hand = (side_name(s, n_) for n_ in ("Shoulder", "UpperArm", "LowerArm", "Hand"))
        w_pauldron = lambda p, _u=up, _s=sh: {_u: 0.6, _s: 0.4}

        # 肩当て（3枚重ね。控えめに張り出し、外の縁を曲線にする）
        lames = []
        for k, (x0, x1, off, pmax) in enumerate(((.080, .212, .030, 84), (.170, .255, .024, 78),
                                                 (.222, .296, .018, 72))):
            def lame(u, t, _x0=x0, _x1=x1, _off=off, _pm=pmax):
                ph = u * math.radians(_pm)
                x = lerp(_x0 + .018 * u * u, _x1 - .030 * u * u, t)
                return arm_pt(s, ph, x, _off + .006 * math.sin(math.pi * t))
            lames.append(lame)
            plate(lame, 14, 5, .004, ARMOR, w_pauldron, arm_hint)
            edge_trim(lame, 1.0, w_pauldron, arm_hint, u0=-.92, u1=.92)
        # 肩当ての紋様
        inlay(lames[0], bezier((-.70, .25), (-.20, .05), (.25, .55), (.75, .62)), .006, .0025, TRIM, w_pauldron,
              arm_hint)
        inlay(lames[0], bezier((-.55, .55), (-.15, .40), (.15, .78), (.50, .85), n=14), .0045, .0025, GLOW,
              w_pauldron, arm_hint)

        # 上腕の板
        def rere(u, t):
            ph = u * math.radians(108)
            return arm_pt(s, ph, lerp(.200, .300 + .014 * (1 - u * u), t), .010)
        plate(rere, 14, 4, .004, ARMOR, rigid(up), arm_hint)

        # 肘当て（後ろ向きの木の葉の形の小さな板）
        def couter(u, t):
            ph = -math.radians(90) + u * math.radians(62)
            x = lerp(.298 + .026 * u * u, .378 - .026 * u * u, t)
            bulge = .010 * (1 - u * u) * math.sin(math.pi * t)
            return arm_pt(s, ph, x, .012 + bulge)
        w_c = lambda p, _u=up, _l=low: chain(abs(p.x), [(.32, _u), (.35, _l)])
        plate(couter, 10, 6, .004, ARMOR, w_c, arm_hint)

        # 籠手（前腕。肘側は上が長くとがり、手首側は少し開く）
        def vamb(u, t):
            ph = u * math.radians(150)
            x0 = .368 - .028 * math.cos(ph)
            x = lerp(x0, .532, t)
            return arm_pt(s, ph, x, .009 + .008 * smoothstep(.70, 1.0, t))
        plate(vamb, 18, 7, .004, ARMOR, rigid(low), arm_hint)

        def vamb_top(u, t):
            ph = u * math.radians(140)
            return arm_pt(s, ph, lerp(.352 - .022 * math.cos(ph), .392, t), .015 + .004 * t)
        plate(vamb_top, 16, 2, .004, ARMOR, rigid(low), arm_hint)
        edge_trim(vamb_top, 1.0, rigid(low), arm_hint, u0=-.95, u1=.95, width=.005)
        edge_trim(vamb, 1.0, rigid(low), arm_hint, u0=-.95, u1=.95)
        inlay(vamb, bezier((-.30, .12), (.25, .32), (-.15, .62), (.30, .90)), .006, .0025, TRIM, rigid(low), arm_hint)
        inlay(vamb, [(0.0, lerp(.18, .86, i / 16)) for i in range(17)], .0045, .0025, GLOW, rigid(low), arm_hint)

        # 手首の口
        def cuff(u, t):
            return arm_pt(s, u * math.radians(170), lerp(.528, .574, t), .015 + .004 * t)
        plate(cuff, 18, 2, .004, ARMOR, rigid(hand), arm_hint)

        # 拳（右手は柄の向きに合わせて回す）と、甲の板
        fist = Vector((s * .610, .012, 1.272))
        rot = Matrix.Rotation(-SWORD_ANGLE, 3, "Z") if s < 0 else Matrix.Identity(3)
        axes = tuple(rot @ Vector(a) for a in ((1, 0, 0), (0, 1, 0), (0, 0, 1)))
        g = axes[1] * -1            # 握っているものの向き（右手は柄、左手は前）
        e_up, e_out = axes[2], axes[0] * s
        v, f = box(fist - e_out * .010, (.058, .084, .060), axes)
        add_part(v, f, SUIT, rigid(hand), stage=1, bevel=.012)
        # 柄を巻いて握る指（4本 × 3節。人差し指が鍔の側）
        for off_g in (.031, .011, -.009, -.029):
            for k, (phi, ln) in enumerate(((22, .024), (78, .022), (132, .020))):
                a = math.radians(phi)
                rad = e_up * math.cos(a) + e_out * math.sin(a)
                tng = -e_up * math.sin(a) + e_out * math.cos(a)
                c = fist + e_out * .004 + g * off_g + rad * .034
                v, f = box(c, (ln, .018, .016), (tng, g, rad))
                add_part(v, f, ARMOR if k < 2 else SUIT, rigid(hand), stage=1, bevel=.004)
        for k, (dx, w_) in enumerate(((-.018, .050), (.020, .044))):
            v, f = box(fist + rot @ Vector((s * dx, 0, .036 - .004 * k)), (w_, .090, .010), axes)
            add_part(v, f, ARMOR, rigid(hand), stage=2, bevel=.004)
        v, f = box(fist + rot @ Vector((-s * .012, -.048, .016)), (.046, .024, .026), axes)
        add_part(v, f, ARMOR, rigid(hand), stage=2, bevel=.006)


def build_leg_armor():
    for s in (1, -1):
        up, low, foot, toe = (side_name(s, n_) for n_ in ("UpperLeg", "LowerLeg", "Foot", "ToeBase"))
        hint = leg_hint(s)

        # 腿当て（前と外を覆う。下の縁は膝の前へとがる）
        def cuisse(u, t):
            th = math.radians(42) + u * math.radians(56)
            zt = .842 + .030 * u
            zb = .600 + .060 * (th / math.radians(100)) ** 2
            return leg_pt(s, th, lerp(zt, zb, t), .008)
        plate(cuisse, 14, 8, .004, ARMOR, rigid(up), hint)

        # 腿の上の重ねの板
        def cuisse_top(u, t):
            th = math.radians(42) + u * math.radians(60)
            return leg_pt(s, th, lerp(.884 + .030 * u, .812 + .024 * u - .012 * (1 - u * u), t), .015)
        plate(cuisse_top, 14, 3, .004, ARMOR, rigid(up), hint)
        edge_trim(cuisse_top, 1.0, rigid(up), hint, u0=-.95, u1=.95, width=.005)

        # 膝当て（木の葉の形の薄い板）
        def poleyn(u, t):
            th = u * math.radians(70)
            zt = .604 - .036 * u * u
            zb = .468 + .034 * u * u
            bulge = .005 * math.sin(math.pi * t) + .010 * max(0.0, 1 - abs(u) * 3)
            return leg_pt(s, th, lerp(zt, zb, t), .013 + bulge)
        plate(poleyn, 12, 6, .004, ARMOR, rigid(low), hint)
        inlay(poleyn, bezier((-.55, .35), (-.15, .15), (.15, .85), (.55, .65), n=12), .005, .002, TRIM, rigid(low),
              hint)

        # すね当て（前に稜線。上の縁は横へ下がる）
        def greave(u, t):
            th = u * math.radians(125)
            zt = .470 - .040 * u * u
            zb = .150 + .020 * u * u
            keel = .008 * max(0.0, 1 - abs(u) * 4)
            return leg_pt(s, th, lerp(zt, zb, t), .009 + keel)
        plate(greave, 18, 8, .004, ARMOR, rigid(low), hint)
        edge_trim(greave, 0.0, rigid(low), hint, u0=-.95, u1=.95, width=.006)

        # 足首の重ねの板（2枚）
        for k, (zt, zb, off) in enumerate(((.182, .146, .011), (.156, .120, .016))):
            def ankle(u, t, _zt=zt, _zb=zb, _off=off):
                th = u * math.radians(150)
                return leg_pt(s, th, lerp(_zt - .010 * u * u, _zb + .006 * u * u, t), _off)
            w = rigid(low) if k == 0 else (lambda p, _l=low, _f=foot: {_l: .5, _f: .5})
            plate(ankle, 18, 2, .004, ARMOR, w, hint)

        # 鉄靴（とがったつま先。断面は角ばった七角形）
        prof = [(.080, .036, .140, .002), (.040, .044, .150, .000), (-.020, .045, .100, .000),
                (-.080, .040, .072, .000), (-.130, .032, .050, .002), (-.168, .016, .030, .006)]
        rings = []
        for y, hw, zt, zb in prof:
            cx = s * .077
            rings.append([Vector((cx - hw, y, zb)), Vector((cx + hw, y, zb)), Vector((cx + hw * 1.04, y, lerp(zb, zt, .55))),
                          Vector((cx + hw * .62, y, zt)), Vector((cx, y, zt + .010)), Vector((cx - hw * .62, y, zt)),
                          Vector((cx - hw * 1.04, y, lerp(zb, zt, .55)))])
        v, f = loft(rings, cap_start=True, end_point=(s * .077, -.196, .016))
        add_part(v, f, ARMOR, lambda p, _f=foot, _t=toe: {_f: smoothstep(-0.10, -0.04, p.y),
                                                          _t: 1 - smoothstep(-0.10, -0.04, p.y)}, stage=1)
        # 甲の板（2枚）
        for k, (y0, y1) in enumerate(((-.010, -.070), (-.060, -.120))):
            def lame(u, t, _y0=y0, _y1=y1):
                y = lerp(_y0, _y1, t)
                hw = interp_table([(p[0] * -1, p[1]) for p in prof[::-1]], -y)[0] * 1.06
                zt = interp_table([(p[0] * -1, p[2]) for p in prof[::-1]], -y)[0]
                a = u * math.radians(70)
                return Vector((s * .077 + hw * math.sin(a), y, zt - .02 + (hw * .55 + .02) * math.cos(a) + .006))
            w = (lambda p, _f=foot: {_f: 1.0}) if k == 0 else (lambda p, _t=toe: {_t: 1.0})
            plate(lame, 10, 3, .004, ARMOR, w, lambda p, _s=s: Vector((p.x - _s * .077, 0, 1)).normalized())


def build_sword():
    """ゆるく反った細身の片手剣。刀身は腕と同じくらい（0.6 m）。刀身の両面に紫の発光ラインを1本。"""
    d, e, f = sword_axes()
    R = rigid("J_Sec_Sword")
    fist = FIST_R

    def C(sd):
        """刀身の中心線。根元から先へ、峰の側（-e）へゆるく反る。"""
        k = max(0.0, sd - .10)
        return fist + d * sd - e * (.085 * k * k)

    def D(sd):
        return (C(sd + .005) - C(sd - .005)).normalized()

    # 柄（六角の革巻き）と柄頭
    rings = [[fist + d * sd + e * (.0145 * math.cos(a)) + f * (.0135 * math.sin(a))
              for a in (2 * math.pi * k / 6 for k in range(6))] for sd in (-.094, .080)]
    v, f_ = loft(rings)
    add_part(v, f_, LEATHER, R, stage=1)
    pv = [fist + d * (-.094), fist + d * (-.122) + e * .010, fist + d * (-.142)]
    rings = [[p + e * (r * math.cos(a)) + f * (r * .8 * math.sin(a)) for a in (2 * math.pi * k / 6 for k in range(6))]
             for p, r in zip(pv[:2], (.016, .014))]
    v, f_ = loft(rings, cap_start=True, end_point=pv[2])
    add_part(v, f_, TRIM, R, stage=1)
    # 鍔（刃と峰の向きに伸び、両端が刀身の方へ反る薄い板）
    pts = [fist + d * (.088 + .016 * (x / .036) ** 2) + e * x for x in (lerp(-.036, .036, i / 12) for i in range(13))]
    ribbon(pts, [d] * len(pts), .026, .007, TRIM, R, stage=1, base=-.0035, taper=True)
    # 刀身（刃は薄く、峰は平ら）
    rings = []
    sds = [.096, .12, .16, .21, .26, .31, .36, .41, .46, .51, .55, .59, .62, .65, .67]
    for sd in sds:
        c = C(sd)
        dd = D(sd)
        ee = (e - dd * e.dot(dd)).normalized()
        ff = dd.cross(ee).normalized()
        hw = lerp(.0165, .0115, smoothstep(.1, .62, sd)) * (1 - .55 * smoothstep(.60, .67, sd))
        ht = .0042 * (1 - .4 * smoothstep(.5, .67, sd))
        rings.append([c + ee * hw, c + ff * ht - ee * hw * .1, c + ff * ht * .7 - ee * hw,
                      c - ff * ht * .7 - ee * hw, c - ff * ht - ee * hw * .1])
    tip = C(.705) + (e - D(.7) * e.dot(D(.7))).normalized() * .004
    v, f_ = loft(rings, cap_start=True, end_point=tip)
    add_part(v, f_, BLADE, R, stage=1)
    # 刀身の根元の金具（鍔の上で刀身を包む）
    rings = []
    for sd, sc in ((.094, 1.45), (.122, 1.30)):
        c = C(sd)
        rings.append([c + e * (.0175 * sc * math.cos(a_)) + f * (.0050 * sc * 1.6 * math.sin(a_))
                      for a_ in (2 * math.pi * k / 8 for k in range(8))])
    v, f_ = loft(rings)
    add_part(v, f_, TRIM, R, stage=2)
    # 刀身の発光ライン（峰寄り、両面）
    for side in (1, -1):
        pts, nrm = [], []
        for i in range(25):
            sd = lerp(.14, .60, i / 24)
            c = C(sd)
            dd = D(sd)
            ee = (e - dd * e.dot(dd)).normalized()
            ff = dd.cross(ee).normalized() * side
            ht = .0042 * (1 - .4 * smoothstep(.5, .67, sd))
            hw = lerp(.0165, .0115, smoothstep(.1, .62, sd))
            pts.append(c + ff * (ht * .78) - ee * hw * .45)
            nrm.append(ff)
        ribbon(pts, nrm, .0032, .0008, GLOW, R, stage=3)


def build_gear():
    """小物。腿のベルト、腰の後ろの小物入れ2つ、腰の後ろに横へ差した短剣。"""
    hips = rigid("J_Bip_C_Hips")
    for s in (1, -1):
        up = side_name(s, "UpperLeg")

        def strap(u, t, _s=s):
            return leg_pt(_s, u * math.pi, lerp(.716, .694, t), .012)
        plate(strap, 22, 1, .003, LEATHER, rigid(up), leg_hint(s))
        c = leg_pt(s, math.radians(100), .705, .016)
        n_ = Vector((c.x - s * .077, c.y, 0)).normalized()
        axes = (Vector((0, 0, 1)).cross(n_).normalized(), n_, Vector((0, 0, 1)))
        v, f = box(c, (.020, .006, .026), axes)
        add_part(v, f, TRIM, rigid(up), stage=3, bevel=.002)
        # 小物入れ（ふたは金の縁）
        th = s * math.radians(128)
        c = torso_pt(th, .925, .040)
        n_ = Vector((c.x, c.y - .01, 0)).normalized()
        axes = (Vector((0, 0, 1)).cross(n_).normalized(), n_, Vector((0, 0, 1)))
        v, f = box(c, (.058, .034, .056), axes)
        add_part(v, f, LEATHER, hips, stage=2, bevel=.008)
        v, f = box(c + n_ * .004 + Vector((0, 0, .020)), (.062, .036, .020), axes)
        add_part(v, f, LEATHER, hips, stage=2, bevel=.005)
        v, f = box(c + n_ * .020 + Vector((0, 0, .010)), (.014, .004, .016), axes)
        add_part(v, f, TRIM, hips, stage=3, bevel=.002)
    # 短剣（腰の後ろに横へ差す。柄は右）
    X, Y, Z = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
    c = Vector((.020, .118, .915))
    v, f = box(c, (.200, .022, .036), (X, Y, Z))
    add_part(v, f, LEATHER, hips, stage=2, bevel=.008)
    v, f = box(c + X * .108, (.024, .026, .040), (X, Y, Z))
    add_part(v, f, TRIM, hips, stage=3, bevel=.005)
    for dx in (-.040, .040):
        v, f = box(c + X * dx, (.010, .026, .040), (X, Y, Z))
        add_part(v, f, TRIM, hips, stage=3, bevel=.002)
    v, f = box(c - X * .108, (.012, .030, .050), (X, Y, Z))
    add_part(v, f, TRIM, hips, stage=2, bevel=.003)
    v, f = box(c - X * .150, (.075, .016, .018), (X, Y, Z))
    add_part(v, f, LEATHER, hips, stage=2, bevel=.005)
    v, f = box(c - X * .193, (.016, .020, .022), (X, Y, Z))
    add_part(v, f, TRIM, hips, stage=2, bevel=.005)


build_body()
build_gear()
build_helmet()
build_hood()
build_cloth()
build_sword()
build_leg_armor()
build_torso_armor()
build_arm_armor()

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
        bmesh.ops.bevel(bm, geom=list(bm.edges), offset=part["bevel"], segments=2, affect="EDGES",
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
bake_uv = me.uv_layers.new(name="UVMap")    # 焼き付けたテクスチャの UV（ゲームはこれを使う）
uv = me.uv_layers.new(name="Cells")         # 色のマスの UV（焼き付けの元）
for poly in me.polygons:
    zmin, zmax = part_zrange[face_part[poly.index]]
    for li in poly.loop_indices:
        z = all_verts[me.loops[li].vertex_index].z
        fr = 0.5 if zmax - zmin < 1e-6 else (z - zmin) / (zmax - zmin)
        uv.data[li].uv = cell_uv(face_cells[poly.index], 0.5, fr)
me.shade_smooth()
me.set_sharp_from_angle(angle=math.radians(35))

obj = bpy.data.objects.new(OUT_NAME, me)
bpy.context.scene.collection.objects.link(obj)
obj.parent = ARM
obj.matrix_parent_inverse = ARM.matrix_world.inverted()
groups = {}
for i, ws in enumerate(vert_weights):
    for bone, wt in ws.items():
        if wt <= 1e-4:
            continue
        groups.setdefault((bone, round(wt, 5)), []).append(i)
for (bone, wt), ids in groups.items():
    vg = obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
    vg.add(ids, wt, "REPLACE")
mod = obj.modifiers.new("Armature", "ARMATURE")
mod.object = ARM

tris = sum(len(p.vertices) - 2 for p in me.polygons)
print("STAGE", STAGE, "PARTS", len(PARTS), "VERTS", len(me.vertices), "TRIS", tris)

# 頭身（フードの上からあごまでを1頭とする）
zs_all = [v.z for v in all_verts]
total_h = max(zs_all)
head_h = total_h - 1.412
print(f"HEIGHT {total_h:.3f} HEAD {head_h:.3f} RATIO {total_h / head_h:.2f}")

# ---------------------------------------------------------------------------
# テクスチャの焼き付け（stage 4）
#   色のマス（Cells の UV）を元に、Cycles で細部を描き足して、焼き付け用の UV（UVMap）の 2048px に焼く。
#   - 色: 縁のすり減り・すき間の陰（AO）・彫りの紋様・細かい傷・布の織り目・革のしぼ・足元の汚れ
#   - 金属感となめらかさ（spec）・発光のマスク
#   - 法線: 板の縁の面取り・彫り・傷・織り目・金の飾りの粒・革のしぼ
# ---------------------------------------------------------------------------
BAKE_SIZE = 2048


def id_atlas():
    """部品の種類を色で表したマスの画像。R: 鎧 1・刃 0.5、G: 金、B: 布 1・革 0.5。"""
    arr = np.zeros((TEX_SIZE, TEX_SIZE, 4), dtype=np.float32)
    arr[..., 3] = 1.0
    ids = {ARMOR: (1, 0, 0), BLADE: (.5, 0, 0), TRIM: (0, 1, 0), CLOTH_OUT: (0, 0, 1), CLOTH_IN: (0, 0, 1),
           SUIT: (0, 0, 1), LEATHER: (0, 0, .5)}
    for cell, rgb in ids.items():
        cx, cy = cell % 4, cell // 4
        arr[cy * CELL_PX:(cy + 1) * CELL_PX, cx * CELL_PX:(cx + 1) * CELL_PX, :3] = rgb
    img = bpy.data.images.new(f"{OUT_NAME}_id", TEX_SIZE, TEX_SIZE, alpha=True)
    img.colorspace_settings.name = "Non-Color"
    img.pixels.foreach_set(arr.ravel())
    img.pack()
    return img


class NodeKit:
    """ノードを短く組むための道具。"""

    def __init__(self, tree):
        self.t = tree

    def n(self, kind, **props):
        nd = self.t.nodes.new(kind)
        for k, v in props.items():
            setattr(nd, k, v)
        return nd

    def link(self, src, dst):
        self.t.links.new(src, dst)

    def put(self, sock, val):
        if isinstance(val, bpy.types.NodeSocket):
            self.link(val, sock)
        else:
            sock.default_value = val

    def math(self, op, a, b=0.0, clamp=False):
        nd = self.n("ShaderNodeMath", operation=op, use_clamp=clamp)
        self.put(nd.inputs[0], a)
        self.put(nd.inputs[1], b)
        return nd.outputs[0]

    def mul(self, *xs):
        out = xs[0]
        for x in xs[1:]:
            out = self.math("MULTIPLY", out, x)
        return out

    def ramp(self, val, f0, f1, t0=0.0, t1=1.0):
        """f0→f1 を t0→t1 になめらかにつなぐ（範囲の外は端の値）。"""
        nd = self.n("ShaderNodeMapRange", interpolation_type="SMOOTHSTEP", clamp=True)
        self.put(nd.inputs["Value"], val)
        nd.inputs["From Min"].default_value = f0
        nd.inputs["From Max"].default_value = f1
        nd.inputs["To Min"].default_value = t0
        nd.inputs["To Max"].default_value = t1
        return nd.outputs["Result"]

    def gray(self, val):
        nd = self.n("ShaderNodeCombineColor")
        for i in range(3):
            self.put(nd.inputs[i], val)
        return nd.outputs[0]

    def mix(self, fac, a, b, blend="MIX"):
        nd = self.n("ShaderNodeMix", data_type="RGBA", blend_type=blend, clamp_result=True)
        self.put(nd.inputs[0], fac)
        self.put(nd.inputs[6], a if not isinstance(a, tuple) else (*a, 1.0))
        self.put(nd.inputs[7], b if not isinstance(b, tuple) else (*b, 1.0))
        return nd.outputs[2]

    def noise(self, vec, scale, detail=2.0, rough=0.5, distortion=0.0):
        nd = self.n("ShaderNodeTexNoise")
        self.link(vec, nd.inputs["Vector"])
        nd.inputs["Scale"].default_value = scale
        nd.inputs["Detail"].default_value = detail
        nd.inputs["Roughness"].default_value = rough
        nd.inputs["Distortion"].default_value = distortion
        return nd.outputs["Fac"]

    def scaled(self, vec, sx, sy, sz):
        nd = self.n("ShaderNodeMapping")
        self.link(vec, nd.inputs["Vector"])
        nd.inputs["Scale"].default_value = (sx, sy, sz)
        return nd.outputs["Vector"]

    def wave(self, vec, direction, scale, distortion=0.0, detail=0.0, detail_scale=1.0):
        nd = self.n("ShaderNodeTexWave", wave_type="BANDS", bands_direction=direction)
        self.link(vec, nd.inputs["Vector"])
        nd.inputs["Scale"].default_value = scale
        nd.inputs["Distortion"].default_value = distortion
        nd.inputs["Detail"].default_value = detail
        nd.inputs["Detail Scale"].default_value = detail_scale
        return nd.outputs["Fac"]

    def bump(self, normal, height, strength, distance, invert=False):
        nd = self.n("ShaderNodeBump", invert=invert)
        self.link(height, nd.inputs["Height"])
        nd.inputs["Strength"].default_value = strength
        nd.inputs["Distance"].default_value = distance
        self.link(normal, nd.inputs["Normal"])
        return nd.outputs["Normal"]


def bake_textures(obj, me, atlas):
    import time
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 6
    scene.render.bake.margin = 6
    # 焼き付け用の UV を広げる
    for o in bpy.data.objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    me.uv_layers.active = me.uv_layers["UVMap"]
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(55), island_margin=0.0015, area_weight=0.0,
                             correct_aspect=True, scale_to_bounds=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    me.uv_layers.active = me.uv_layers["UVMap"]

    m = bpy.data.materials.new("Bake")
    m.use_nodes = True
    t = m.node_tree
    for nd in list(t.nodes):
        t.nodes.remove(nd)
    K = NodeKit(t)
    out = K.n("ShaderNodeOutputMaterial")
    cells = K.n("ShaderNodeUVMap", uv_map="Cells").outputs["UV"]

    def atlas_tex(img):
        nd = K.n("ShaderNodeTexImage", interpolation="Closest")
        nd.image = img
        K.link(cells, nd.inputs["Vector"])
        return nd

    t_alb, t_spec = atlas_tex(atlas["albedo"]), atlas_tex(atlas["spec"])
    t_emit, t_id = atlas_tex(atlas["emission"]), atlas_tex(id_atlas())
    sep_id = K.n("ShaderNodeSeparateColor")
    K.link(t_id.outputs["Color"], sep_id.inputs[0])
    sep_sp = K.n("ShaderNodeSeparateColor")
    K.link(t_spec.outputs["Color"], sep_sp.inputs[0])
    sep_em = K.n("ShaderNodeSeparateColor")
    K.link(t_emit.outputs["Color"], sep_em.inputs[0])
    R, G, B = sep_id.outputs[0], sep_id.outputs[1], sep_id.outputs[2]
    armor = K.math("GREATER_THAN", R, .75)
    blade = K.mul(K.math("GREATER_THAN", R, .25), K.math("LESS_THAN", R, .75))
    trim = K.math("GREATER_THAN", G, .5)
    cloth = K.math("GREATER_THAN", B, .75)
    leather = K.mul(K.math("GREATER_THAN", B, .25), K.math("LESS_THAN", B, .75))
    glow = K.math("GREATER_THAN", sep_em.outputs[0], .5)
    metalish = K.math("MAXIMUM", K.math("MAXIMUM", armor, trim), blade)

    obj_co = K.n("ShaderNodeTexCoord").outputs["Object"]
    sxyz = K.n("ShaderNodeSeparateXYZ")
    K.link(obj_co, sxyz.inputs[0])
    z = sxyz.outputs["Z"]

    # 縁（面取りした法線と元の法線の差）と、すき間の陰
    bevel = K.n("ShaderNodeBevel", samples=8)
    bevel.inputs["Radius"].default_value = .0028
    geo = K.n("ShaderNodeNewGeometry")
    dot = K.n("ShaderNodeVectorMath", operation="DOT_PRODUCT")
    K.link(bevel.outputs["Normal"], dot.inputs[0])
    K.link(geo.outputs["Normal"], dot.inputs[1])
    edge = K.ramp(dot.outputs["Value"], .985, .90)
    ao_nd = K.n("ShaderNodeAmbientOcclusion", samples=12)
    ao_nd.inputs["Distance"].default_value = .045
    cavity = K.ramp(ao_nd.outputs["AO"], .15, 1.0, .42, 1.0)

    # 模様
    var = K.ramp(K.noise(obj_co, 5.0, 3.0), .30, .70, .86, 1.12)          # 大きなむら
    patch = K.ramp(K.noise(obj_co, 3.0, 2.0), .56, .66)                    # 傷の多いところ
    scr = K.mul(K.ramp(K.noise(K.scaled(obj_co, 90, 90, 7), 1.0, 2.0), .655, .70), patch)    # 縦の細かい傷
    scr2 = K.mul(K.ramp(K.noise(K.scaled(obj_co, 8, 140, 140), 1.0, 2.0), .67, .71), patch)  # 横の傷
    scratches = K.math("MAXIMUM", scr, scr2)
    eng_line = K.ramp(K.wave(obj_co, "DIAGONAL", 24.0, 4.5, 1.5, 1.2), .935, .985)   # 渦を巻く細い線
    eng_area = K.ramp(K.noise(obj_co, 4.5, 2.0), .60, .64)
    engrave = K.mul(eng_line, eng_area, K.math("SUBTRACT", 1.0, edge, clamp=True), armor)
    vor = K.n("ShaderNodeTexVoronoi", feature="F1")
    K.link(obj_co, vor.inputs["Vector"])
    vor.inputs["Scale"].default_value = 260.0
    beads = K.mul(K.ramp(vor.outputs["Distance"], .30, .16), trim)       # 金の飾りの粒
    weave = K.mul(K.wave(obj_co, "X", 420.0), K.wave(obj_co, "Z", 420.0))
    fuzz = K.noise(obj_co, 160.0, 3.0)
    grain = K.noise(obj_co, 220.0, 4.0, .7)                                # 革のしぼ
    dirt = K.mul(K.ramp(z, .30, 0.0), K.ramp(K.noise(obj_co, 7.0, 3.0), .40, .70), .65)

    # 色
    col = t_alb.outputs["Color"]
    col = K.mix(1.0, col, K.gray(var), "MULTIPLY")
    col = K.mix(K.mul(engrave, .55), col, (0.058, 0.058, 0.072))            # 彫りは少し明るい鋼
    col = K.mix(K.mul(scratches, metalish, .35), col, (0.095, 0.095, 0.115))  # 傷
    col = K.mix(K.mul(edge, armor, .9), col, (0.17, 0.17, 0.205))           # 鎧の縁のすり減り
    col = K.mix(K.mul(edge, trim, .8), col, (0.62, 0.48, 0.24))             # 金の縁は明るく
    col = K.mix(K.mul(beads, .45), col, (0.55, 0.43, 0.22))
    col = K.mix(K.mul(edge, blade, .9), col, (0.62, 0.64, 0.70))            # 刃先
    wv = K.ramp(K.math("ADD", K.mul(weave, .6), K.mul(fuzz, .4)), .2, .8, .82, 1.08)
    col = K.mix(cloth, col, K.mix(1.0, col, K.gray(wv), "MULTIPLY"))
    lv = K.ramp(grain, .3, .7, .85, 1.1)
    col = K.mix(leather, col, K.mix(1.0, col, K.gray(lv), "MULTIPLY"))
    col = K.mix(dirt, col, (0.035, 0.031, 0.027))
    col = K.mix(1.0, col, K.gray(cavity), "MULTIPLY")
    col = K.mix(glow, col, t_alb.outputs["Color"])                          # 発光の線はそのままの色

    # 金属感・なめらかさ・発光（R・G・B にまとめて焼く）
    metal = K.math("ADD", sep_sp.outputs[0], K.mul(edge, metalish, .30), clamp=True)
    metal = K.math("SUBTRACT", metal, K.mul(dirt, .5), clamp=True)
    smooth = K.math("ADD", t_spec.outputs["Alpha"], K.mul(edge, metalish, .18), clamp=True)
    smooth = K.math("SUBTRACT", smooth, K.mul(scratches, metalish, .22), clamp=True)
    smooth = K.math("SUBTRACT", smooth, K.mul(dirt, .3), clamp=True)
    msg = K.n("ShaderNodeCombineColor")
    K.link(metal, msg.inputs[0])
    K.link(smooth, msg.inputs[1])
    K.link(glow, msg.inputs[2])

    # 法線（面取り → 彫り → 傷 → 織り目 → 粒 → 革）
    nrm = bevel.outputs["Normal"]
    nrm = K.bump(nrm, engrave, .55, .0008, invert=True)
    nrm = K.bump(nrm, scratches, .22, .0003, invert=True)
    nrm = K.bump(nrm, K.mul(weave, cloth), .35, .0005)
    nrm = K.bump(nrm, beads, .45, .0006)
    nrm = K.bump(nrm, K.mul(grain, leather), .25, .0004)
    bsdf_n = K.n("ShaderNodeBsdfPrincipled")
    K.link(nrm, bsdf_n.inputs["Normal"])
    emis = K.n("ShaderNodeEmission")

    me.materials.clear()
    me.materials.append(m)

    def target(name, colorspace):
        img = bpy.data.images.new(f"{OUT_NAME}_{name}_bake", BAKE_SIZE, BAKE_SIZE, alpha=False)
        img.colorspace_settings.name = colorspace
        nd = K.n("ShaderNodeTexImage")
        nd.image = img
        for x in t.nodes:
            x.select = False
        nd.select = True
        t.nodes.active = nd
        return img

    results = {}
    for name, src, cs in (("albedo", col, "sRGB"), ("msg", msg.outputs[0], "Non-Color")):
        img = target(name, cs)
        K.link(src, emis.inputs["Color"])
        K.link(emis.outputs[0], out.inputs["Surface"])
        t0 = time.time()
        bpy.ops.object.bake(type="EMIT", use_clear=True, margin=6)
        print(f"BAKED {name} {time.time() - t0:.0f}s")
        results[name] = img
    img = target("normal", "Non-Color")
    K.link(bsdf_n.outputs[0], out.inputs["Surface"])
    t0 = time.time()
    bpy.ops.object.bake(type="NORMAL", normal_space="TANGENT", use_clear=True, margin=6)
    print(f"BAKED normal {time.time() - t0:.0f}s")
    results["normal"] = img

    # 保存（spec は r=メタリック・a=なめらかさ、発光のマスクは別の1枚）
    def pixels(img):
        a = np.empty(BAKE_SIZE * BAKE_SIZE * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        return a.reshape(BAKE_SIZE, BAKE_SIZE, 4)

    def save(arr, name, colorspace):
        im = bpy.data.images.new(f"{OUT_NAME}_{name}", BAKE_SIZE, BAKE_SIZE, alpha=True)
        im.colorspace_settings.name = colorspace
        im.pixels.foreach_set(arr.ravel())
        im.filepath_raw = os.path.join(OUT_DIR, f"{OUT_NAME}_{name}.png")
        im.file_format = "PNG"
        im.save()
        return im

    alb = pixels(results["albedo"])
    alb[..., 3] = 1.0
    msg_px = pixels(results["msg"])
    spec = np.zeros_like(msg_px)
    spec[..., 0] = msg_px[..., 0]
    spec[..., 3] = msg_px[..., 1]
    emi = np.zeros_like(msg_px)
    emi[..., 0] = emi[..., 1] = emi[..., 2] = msg_px[..., 2]
    emi[..., 3] = 1.0
    nrm_px = pixels(results["normal"])
    nrm_px[..., 3] = 1.0
    baked = {
        "albedo": save(alb, "albedo", "sRGB"),
        "spec": save(spec, "spec", "Non-Color"),
        "emission": save(emi, "emission", "Non-Color"),
        "normal": save(nrm_px, "normal", "Non-Color"),
    }
    me.uv_layers.remove(me.uv_layers["Cells"])
    me.materials.clear()
    return baked


# ---------------------------------------------------------------------------
# マテリアルと書き出し（ゲーム用: base color に albedo、specular IOR level に spec。
# エンジンは名前に specular を含むテクスチャを spec マップとして読む）
# ---------------------------------------------------------------------------
images = make_textures(save=False)
if STAGE >= 4:
    images = bake_textures(obj, me, images)
else:
    # 焼き付けないときは、色のマスの UV をそのまま使う
    me.uv_layers.remove(me.uv_layers["UVMap"])
    me.uv_layers["Cells"].name = "UVMap"
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
if "normal" in images:
    tex_n = nt.nodes.new("ShaderNodeTexImage")
    tex_n.image = images["normal"]
    nmap = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(tex_n.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
me.materials.clear()
me.materials.append(mat)

if STAGE >= 4:
    out_fbx = os.path.join(OUT_DIR, f"{OUT_NAME}.fbx")
    for o in bpy.data.objects:
        o.select_set(o in (ARM, obj))
    bpy.ops.export_scene.fbx(filepath=out_fbx, axis_forward="Y", axis_up="Z", add_leaf_bones=False,
                             primary_bone_axis="Y", secondary_bone_axis="X", bake_anim=False,
                             object_types={"EMPTY", "ARMATURE", "MESH"}, apply_unit_scale=True,
                             apply_scale_options="FBX_SCALE_NONE", use_armature_deform_only=False,
                             path_mode="STRIP", use_mesh_modifiers=False)
    print("EXPORTED", out_fbx)


# ---------------------------------------------------------------------------
# プレビュー（正面・側面・斜め）
# ---------------------------------------------------------------------------
def toon_material(images):
    """トゥーン寄りの陰影（明・中・暗の3段）＋控えめな金属の照り＋発光。プレビュー用。"""
    m = bpy.data.materials.new("KnightToon")
    m.use_nodes = True
    t = m.node_tree
    for nd in list(t.nodes):
        t.nodes.remove(nd)
    out = t.nodes.new("ShaderNodeOutputMaterial")
    ta = t.nodes.new("ShaderNodeTexImage")
    ta.image = images["albedo"]
    ts = t.nodes.new("ShaderNodeTexImage")
    ts.image = images["spec"]
    te = t.nodes.new("ShaderNodeTexImage")
    te.image = images["emission"]
    # 拡散の3段
    diff = t.nodes.new("ShaderNodeBsdfDiffuse")
    s2r = t.nodes.new("ShaderNodeShaderToRGB")
    t.links.new(diff.outputs[0], s2r.inputs[0])
    nmap = None
    if "normal" in images:
        tn = t.nodes.new("ShaderNodeTexImage")
        tn.image = images["normal"]
        nmap = t.nodes.new("ShaderNodeNormalMap")
        t.links.new(tn.outputs["Color"], nmap.inputs["Color"])
        t.links.new(nmap.outputs["Normal"], diff.inputs["Normal"])
    ramp = t.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = "CONSTANT"
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.0, (0.34, 0.34, 0.40, 1)
    els[1].position, els[1].color = 0.18, (0.66, 0.66, 0.70, 1)
    e3 = els.new(0.55)
    e3.color = (1.0, 1.0, 1.0, 1)
    t.links.new(s2r.outputs["Color"], ramp.inputs["Fac"])
    mul = t.nodes.new("ShaderNodeMix")
    mul.data_type = "RGBA"
    mul.blend_type = "MULTIPLY"
    mul.inputs["Factor"].default_value = 1.0
    t.links.new(ta.outputs["Color"], mul.inputs[6])
    t.links.new(ramp.outputs["Color"], mul.inputs[7])
    # 金属の照り（メタリック 0.6・ラフネス 0.5 の照りを、1段のはっきりした帯にする）
    gl = t.nodes.new("ShaderNodeBsdfPrincipled")
    gl.inputs["Base Color"].default_value = (1, 1, 1, 1)
    gl.inputs["Metallic"].default_value = 1.0
    gl.inputs["Roughness"].default_value = 0.5
    if nmap is not None:
        t.links.new(nmap.outputs["Normal"], gl.inputs["Normal"])
    sep = t.nodes.new("ShaderNodeSeparateColor")
    t.links.new(ts.outputs["Color"], sep.inputs[0])
    s2r2 = t.nodes.new("ShaderNodeShaderToRGB")
    t.links.new(gl.outputs[0], s2r2.inputs[0])
    ramp2 = t.nodes.new("ShaderNodeValToRGB")
    ramp2.color_ramp.interpolation = "CONSTANT"
    ramp2.color_ramp.elements[0].color = (0, 0, 0, 1)
    ramp2.color_ramp.elements[1].position = 0.88
    ramp2.color_ramp.elements[1].color = (1, 1, 1, 1)
    t.links.new(s2r2.outputs["Color"], ramp2.inputs["Fac"])
    tint = t.nodes.new("ShaderNodeMix")
    tint.data_type = "RGBA"
    tint.blend_type = "MIX"
    tint.inputs["Factor"].default_value = 0.5
    tint.inputs[6].default_value = (1, 1, 1, 1)
    t.links.new(ta.outputs["Color"], tint.inputs[7])
    spec_mul = t.nodes.new("ShaderNodeMix")
    spec_mul.data_type = "RGBA"
    spec_mul.blend_type = "MULTIPLY"
    spec_mul.inputs["Factor"].default_value = 1.0
    t.links.new(ramp2.outputs["Color"], spec_mul.inputs[6])
    t.links.new(tint.outputs[2], spec_mul.inputs[7])
    metal_amt = t.nodes.new("ShaderNodeMath")
    metal_amt.operation = "MULTIPLY"
    metal_amt.inputs[1].default_value = 0.40
    t.links.new(sep.outputs[0], metal_amt.inputs[0])
    add1 = t.nodes.new("ShaderNodeMix")
    add1.data_type = "RGBA"
    add1.blend_type = "ADD"
    t.links.new(metal_amt.outputs[0], add1.inputs["Factor"])
    t.links.new(mul.outputs[2], add1.inputs[6])
    t.links.new(spec_mul.outputs[2], add1.inputs[7])
    # 発光
    em_col = t.nodes.new("ShaderNodeMix")
    em_col.data_type = "RGBA"
    em_col.blend_type = "MULTIPLY"
    em_col.inputs["Factor"].default_value = 1.0
    em_col.inputs[7].default_value = (*GLOW_COLOR, 1)
    t.links.new(te.outputs["Color"], em_col.inputs[6])
    em_str = t.nodes.new("ShaderNodeValue")
    em_str.name = "GlowStrength"
    em_str.outputs[0].default_value = 1.2
    em_scale = t.nodes.new("ShaderNodeVectorMath")
    em_scale.operation = "SCALE"
    t.links.new(em_col.outputs[2], em_scale.inputs[0])
    t.links.new(em_str.outputs[0], em_scale.inputs["Scale"])
    add2 = t.nodes.new("ShaderNodeVectorMath")
    add2.operation = "ADD"
    t.links.new(add1.outputs[2], add2.inputs[0])
    t.links.new(em_scale.outputs[0], add2.inputs[1])
    emis = t.nodes.new("ShaderNodeEmission")
    t.links.new(add2.outputs[0], emis.inputs["Color"])
    t.links.new(emis.outputs[0], out.inputs["Surface"])
    return m


def outline_material():
    m = bpy.data.materials.new("KnightOutline")
    m.use_nodes = True
    t = m.node_tree
    for nd in list(t.nodes):
        t.nodes.remove(nd)
    out = t.nodes.new("ShaderNodeOutputMaterial")
    em = t.nodes.new("ShaderNodeEmission")
    em.inputs["Color"].default_value = (0.012, 0.010, 0.018, 1)
    t.links.new(em.outputs[0], out.inputs["Surface"])
    m.use_backface_culling = True
    return m


def clay_material(images, glow):
    m = bpy.data.materials.new("Clay")
    m.use_nodes = True
    t = m.node_tree
    b = t.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (0.50, 0.50, 0.53, 1)
    b.inputs["Roughness"].default_value = 0.6
    if glow:
        ta = t.nodes.new("ShaderNodeTexImage")
        ta.image = images["albedo"]
        t.links.new(ta.outputs["Color"], b.inputs["Base Color"])
        ts = t.nodes.new("ShaderNodeTexImage")
        ts.image = images["spec"]
        sep = t.nodes.new("ShaderNodeSeparateColor")
        t.links.new(ts.outputs["Color"], sep.inputs[0])
        t.links.new(sep.outputs[0], b.inputs["Metallic"])
        b.inputs["Roughness"].default_value = 0.5
        te = t.nodes.new("ShaderNodeTexImage")
        te.image = images["emission"]
        mix = t.nodes.new("ShaderNodeMix")
        mix.data_type = "RGBA"
        mix.blend_type = "MULTIPLY"
        mix.inputs["Factor"].default_value = 1.0
        mix.inputs[7].default_value = (*GLOW_COLOR, 1)
        t.links.new(te.outputs["Color"], mix.inputs[6])
        t.links.new(mix.outputs[2], b.inputs["Emission Color"])
        b.inputs["Emission Strength"].default_value = 1.3
    return m


def render_preview():
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x, scene.render.resolution_y = 420, 600
    scene.view_settings.view_transform = "Standard"
    scene.world = bpy.data.worlds.new("w")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes["Background"]
    bg.inputs[0].default_value = (0.62, 0.64, 0.68, 1)
    bg.inputs[1].default_value = 0.45 if STAGE < 4 else 0.8
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sun.data.energy = 4.5 if STAGE < 4 else 3.2
    sun.rotation_euler = (math.radians(48), 0, math.radians(-35))
    scene.collection.objects.link(sun)
    if STAGE >= 4:
        me.materials.clear()
        me.materials.append(toon_material(images))
        me.materials.append(outline_material())
        sol = obj.modifiers.new("Outline", "SOLIDIFY")
        sol.thickness = 0.0035
        sol.offset = 1.0
        sol.use_flip_normals = True
        sol.material_offset = 1
        sol.use_rim = False
    else:
        me.materials.clear()
        me.materials.append(clay_material(images, STAGE >= 3))
    # 頭身の目盛り（①のみ）
    marks = []
    if STAGE == 1:
        mm = bpy.data.materials.new("mark")
        mm.use_nodes = True
        mm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.8, 0.1, 0.1, 1)
        k = 0
        while k * head_h <= total_h + 1e-3:
            bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.45, k * head_h))
            c = bpy.context.active_object
            c.scale = (1.4, 0.002, 0.003)
            c.data.materials.append(mm)
            marks.append(c)
            k += 1
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    tiles = []
    # (名前, 向き, 正射影, 見る高さ, 写す幅)。最後の2つは上半身と脚のアップ。
    views = [("front", 0, True, 0.84, 1.95), ("side", 90, True, 0.84, 1.95), ("diag", 35, False, 0.84, 0),
             ("close", 30, True, 1.30, 0.80), ("legs", -40, True, 0.55, 1.10)]
    for vname, az, ortho, tz, scale in views:
        a = math.radians(az)
        tgt = Vector((0, 0, tz))
        cam.location = tgt + Vector((math.sin(a) * 4.0, -math.cos(a) * 4.0, 0.25 if not ortho else 0.12))
        cam.rotation_euler = (tgt - cam.location).to_track_quat("-Z", "Y").to_euler()
        if ortho:
            cam.data.type = "ORTHO"
            cam.data.ortho_scale = scale
        else:
            cam.data.type = "PERSP"
            cam.data.lens = 72
        p = os.path.join(PREVIEW_DIR, "_tile.png")
        scene.render.filepath = p
        bpy.ops.render.render(write_still=True)
        im = bpy.data.images.load(p)
        tiles.append(np.array(im.pixels[:]).reshape(im.size[1], im.size[0], 4))
        bpy.data.images.remove(im)
    full = np.concatenate(tiles, axis=1)
    img = bpy.data.images.new("sheet", full.shape[1], full.shape[0])
    img.pixels.foreach_set(full.astype(np.float32).ravel())
    img.filepath_raw = os.path.join(PREVIEW_DIR, f"stage{STAGE}.png")
    img.file_format = "PNG"
    img.save()
    os.remove(os.path.join(PREVIEW_DIR, "_tile.png"))
    print("PREVIEW", img.filepath_raw)


if PREVIEW_DIR:
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    render_preview()
