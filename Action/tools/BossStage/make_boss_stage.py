"""ボス戦の広間（月明かりの玉座の間）を、Meshy で作った部品と単純な形から組み立てる。

Blender をウィンドウなしで動かす:
    blender -b --factory-startup -P make_boss_stage.py

入力:
    MESHY_DIR/<部品>/<部品>.fbx と <部品>.png・_metallic.png・_roughness.png・_normal.png
    （Meshy の zip を展開し、ファイル名の頭を部品名にしたもの。リポジトリには入れない）
    Game/Assets/modelData/stage/boss/BossFloor_*.png（make_floor_texture.py で作る）
    Game/Assets/modelData/stage/boss/BossWall_*.png（make_wall_texture.py で作る）

出力（Game/Assets/modelData/stage/boss/）:
    BossHall.fbx       見た目（部品と床・壁・台座をまとめた1つのモデル）
    BossHallNoShadow.fbx  影を落とさない見た目（天井と炎）
    collision/BossHallCollision.fbx  当たり判定用の単純な形
    〇〇_albedo.png・_spec.png・_normal.png  部品のテクスチャ（起動時に DDS に変換される）
    preview/ に確かめ用の画像（git に入れない）

座標: ゲームの (x, y, z) メートルで考え、Blender には (x, -z, y) で置く。
      書き出した FBX はゲームで enModelUpAxisZ で読むと (100x, 100z, -100y) になり、元のゲーム座標に戻る。
      ゲームの +Z が玉座の方、-Z が入口の扉の方。
"""
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ACTION = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT_DIR = os.path.join(REPO_ACTION, "Game", "Assets", "modelData", "stage", "boss")
COL_DIR = os.path.join(OUT_DIR, "collision")
PREVIEW_DIR = os.path.join(HERE, "preview")
MESHY_DIR = os.environ.get("BOSS_MESHY_DIR", r"D:\03_data\BossStageMeshy")

# ---- 広間の大きさ（ゲームの m） ----
HALL_HALF_WIDTH = 14.0      # 壁の内側までの X
HALL_HALF_LENGTH = 24.0     # 壁の内側までの Z
WALL_HEIGHT = 18.0         # アーチの上（約 17.8m）まで隠れる高さ
WALL_THICKNESS = 1.0
FLOOR_TEXTURE_METERS = 8.0  # 床のテクスチャ1枚（タイル 4×4 枚）を何 m 四方に貼るか（タイル1枚 2m）

# ---- 部品: (目標の大きさ m, 合わせる向き, テクスチャの1辺, 三角形を減らす割合) ----
# 合わせる向き: "height" は高さ、"width" は幅（Blender の X）をその大きさにする。
PARTS = {
    "Throne":      (5.0, "height", 2048, 1.0),
    "Door":        (8.0, "height", 2048, 0.6),
    "Pillar":      (13.0, "height", 2048, 0.35),
    "Window":      (11.0, "height", 2048, 0.35),
    "Arch":        (10.0, "width",  2048, 1.0),
    "Candelabrum": (3.0, "height", 1024, 0.27),
    "Statue":      (3.8, "height", 2048, 0.7),
    "Banner":      (6.5, "height", 2048, 1.0),
    "Rubble":      (2.5, "width",  1024, 1.0),
    "Sconce":      (1.8, "height", 1024, 1.0),
}
# 正面（ランタンが出ている方）が -Y を向いていない部品を、読み込むときに Z まわりに回す（度）
PART_ROTATION = {
    "Sconce": 90.0,   # Meshy の出力はランタンが -X へ伸びている
}

# ---- 置き方 ----
# 見本の画像（太い柱が通り道の両側に迫り、柱の上の方に垂れ幕、柱の前に燭台、奥に大きな窓と玉座）に合わせる
PILLAR_X = 9.0                               # 柱の真ん中。柱の内側の面は約 ±7.8m
PILLAR_ZS = [-18.0, -10.0, -2.0, 6.0, 14.0]
WINDOW_ZS = [-14.0, -6.0, 2.0, 10.0, 18.0]   # 柱と柱の間から見える
WINDOW_BASE_Y = 1.5
BACK_WINDOW_SCALE = 15.0 / 11.0              # 玉座の後ろの壁の大きな窓（高さ 15m）
BACK_WINDOW_BASE_Y = 3.0                     # 下の方は玉座に隠れる
DAIS_HALF_WIDTH = 7.5
DAIS_FRONT_Z = 14.0
DAIS_STEPS = 6
DAIS_STEP_HEIGHT = 0.3
DAIS_STEP_DEPTH = 0.8
DAIS_STEP_SHRINK = 0.3                       # 1段上がるごとに左右を縮める（m）
THRONE_Z = 21.0
ARCH_ZS = [-14.0, -6.0, 2.0, 10.0]          # 同じ列の柱と柱の真ん中
# 燭台: 柱ごとに1本、柱の広間側の前に置く。台座の上にも玉座の手前に小さめを2本。
# ゲームの灯り（BossStage.cpp の CANDLE_LIGHTS）もこの置き場所に合わせる。
CANDELABRUM_X = 6.8
DAIS_CANDELABRA = [(-2.6, 18.7), (2.6, 18.7)]   # (x, z)。いちばん上の段（z が 18 から奥）の手前
DAIS_CANDELABRUM_SCALE = 0.6
STATUE_X = 4.5                               # 玉座の左右（台座の上）
STATUE_Z = 20.5
EMBLEM_CENTER_Z = 3.0                        # 床の紋章の真ん中（戦う場所）
EMBLEM_DIAMETER = 11.0
BANNER_BASE_Y = 4.5                          # 柱の垂れ幕の下の端の高さ（上の端は約 11m で、柱の飾りの下）
BANNER_GAP = 0.05                            # 柱の表から離す距離（m）。布が柱にめり込まないように
WALL_BANNER_X = 7.0                          # 玉座の後ろの壁の垂れ幕（大きな窓の左右）
WALL_BANNER_SCALE = 9.0 / 6.5                # 壁の垂れ幕は柱のものより大きく（高さ 9m）
WALL_BANNER_BASE_Y = 6.0
# がれき: (x, z, 向き°, 大きさの倍率)。隅と、柱と壁の間の足元に置く。通り道（x が ±7.8 の内側）には小さいものだけ
RUBBLE = [
    (-12.0, -22.0, 0, 1.2), (12.3, -21.5, 140, 1.0),
    (-11.8, -6.5, 30, 0.9), (11.8, 2.5, 250, 1.0),
    (-11.6, 17.5, 200, 1.0), (7.0, -14.5, 60, 0.7),
]
# 壁の灯り: (x, 壁)。"front" は入口の壁、"back" は玉座の後ろの壁。横の壁は柱の真後ろになって隠れるので付けない。
# ゲームの灯り（BossStage.cpp の SCONCE_LIGHTS）も、書き出すときに出る位置に合わせる。
SCONCES = [(-5.0, "front"), (5.0, "front"), (-10.0, "front"), (10.0, "front"), (-10.5, "back"), (10.5, "back")]
SCONCE_BASE_Y = 2.6

# ---- 天井（壁の上から、とがったアーチの形の丸天井をかける） ----
VAULT_RISE = 16.0            # 壁の上（18m）から天井のいちばん高いところまで。広間の半分の幅（14m）より高いので、てっぺんがとがる
VAULT_SEGMENTS = 16          # 片側の弧を何本の直線で作るか
VAULT_Z_STEP = 4.0           # 奥行きの分け方（m）
RIB_WIDTH = 0.8              # 天井の骨（リブ）の幅
RIB_DEPTH = 0.45             # 天井の面から下へ出る量
RIB_ZS = PILLAR_ZS + ARCH_ZS  # 柱の列と、アーチの真ん中にリブを渡す

# ---- 床の当たり判定 ----
# 大きな三角形1枚で床を作ると、物理エンジンの計算がずれて、継ぎ目でぶつかったりすり抜けたりする。細かいマスに分ける。
FLOOR_COLLISION_CELL = 2.0

# ---- 通り道の両側の紫の光る帯（見本の床の文字の帯） ----
# エンジンに「自分で光る」仕組みがないので、明るい紫の模様を、すぐ近くの紫のライト（BossStage.cpp の RUNE_LIGHT）で照らして光って見せる。
RUNE_X = 5.6                 # 帯の真ん中
RUNE_WIDTH = 0.4
RUNE_Z_START = -23.0
RUNE_Z_END = DAIS_FRONT_Z - 0.2
RUNE_TEXTURE_METERS = 4.0    # 帯の模様1枚の長さ
RUNE_LIFT = 0.02

# ---- 炎（燭台と壁の灯りのろうそくの先に立てる） ----
# ろうそくの先は、Meshy のテクスチャで白い（ろう）ところの、いちばん高い点として見つける。
WAX_MIN = 0.55               # ろうの色: R・G・B がどれもこれより明るい（sRGB）
WAX_MAX_SPREAD = 0.18        # ろうの色: いちばん明るい色といちばん暗い色の差がこれより小さい（白っぽい）
CANDELABRUM_WICK_RADIUS = 0.10   # 同じろうそくとみなす横の距離（m）。ろうそくの太さより広く、隣のろうそくとの間より狭く
SCONCE_WICK_RADIUS = 0.05
# ろうそくを探す範囲（部品の足元が原点の m。z より下、y が ymin〜ymax）。壁の灯りは、ランタンの屋根の白い飾りの先・ランタンの前の柱・壁の板を外す
WICK_AREA = {
    "Sconce": (1.35, -0.45, 0.0),
}
WICK_MIN_POINTS = 6          # これより少ない点の白いところは、ろうそくでないとして捨てる
FLAME_HEIGHT = 0.24          # 炎の高さ（大きさの倍率 1 のとき、m）
FLAME_WIDTH = 0.10
FLAT_NORMAL_SRGB = 0.735     # sRGB として読まれたあとに 0.5（傾きなし）になる値
FLAME_LIGHT_BELOW = 0.12     # 炎を照らすライトを、炎の段の根元のどれだけ下に置くか（BossStage.cpp の FLAME_LIGHT）
FLAME_TIER_GAP = 0.2         # 根元の高さの差がこれより小さい炎を、同じ段とみなす（m）

# ---- 絨毯 ----
CARPET_HALF_WIDTH = 1.5
CARPET_Z_START = -22.5       # 扉の前
CARPET_LIFT = 0.015
CARPET_TEXTURE_METERS = 6.0  # 模様1枚の長さ
CARPET_EMBLEM_GAP = 0.3      # 紋章の円の縁から空ける距離
CARPET_END_Z = 19.9          # 玉座の前で止める

PART_METALLIC_SCALE = 0.6   # Meshy の金属度に掛ける
PART_SMOOTH_MAX = 0.6       # Meshy のつや（1 - 粗さ）の上限


def log(*a):
    print("[boss]", *a, flush=True)


def to_blender(x, y, z):
    return Vector((x, -z, y))


# ---------------------------------------------------------------------------
# テクスチャ
# ---------------------------------------------------------------------------
def load_px(path, colorspace="Non-Color", size=None):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = colorspace
    if size and tuple(im.size) != (size, size):
        im.scale(size, size)
    a = np.empty(im.size[0] * im.size[1] * 4, dtype=np.float32)
    im.pixels.foreach_get(a)
    w, h = im.size
    bpy.data.images.remove(im)
    return a.reshape(h, w, 4)


def save_png(arr, path, colorspace):
    h, w = arr.shape[:2]
    im = bpy.data.images.new(os.path.basename(path), w, h, alpha=True)
    im.colorspace_settings.name = colorspace
    im.pixels.foreach_set(arr.astype(np.float32).ravel())
    im.filepath_raw = path
    im.file_format = "PNG"
    im.save()
    bpy.data.images.remove(im)


def make_material(name, albedo, spec, normal=None):
    """エンジンが読む形のマテリアル。色 → Base Color、スペキュラ → Specular IOR Level、法線 → Normal Map。"""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]

    def tex(path, colorspace):
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = bpy.data.images.load(path, check_existing=True)
        n.image.colorspace_settings.name = colorspace
        return n

    nt.links.new(tex(albedo, "sRGB").outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(tex(spec, "Non-Color").outputs["Color"], bsdf.inputs["Specular IOR Level"])
    if normal:
        nm = nt.nodes.new("ShaderNodeNormalMap")
        nt.links.new(tex(normal, "Non-Color").outputs["Color"], nm.inputs["Color"])
        nt.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def part_textures(name, size):
    src = os.path.join(MESHY_DIR, name, name)
    alb = load_px(src + ".png", "sRGB", size)
    met = load_px(src + "_metallic.png", "Non-Color", size)
    rough = load_px(src + "_roughness.png", "Non-Color", size)
    nrm = load_px(src + "_normal.png", "Non-Color", size)
    spec = np.zeros_like(met)
    # 空の映り込みで水色に光りすぎないよう、金属度とつやを弱める
    spec[..., 0] = met[..., 0] * PART_METALLIC_SCALE
    spec[..., 3] = np.minimum(1.0 - rough[..., 0], PART_SMOOTH_MAX)
    paths = {k: os.path.join(OUT_DIR, f"{name}_{k}.png") for k in ("albedo", "spec", "normal")}
    save_png(alb, paths["albedo"], "sRGB")
    save_png(spec, paths["spec"], "Non-Color")
    nrm[..., 3] = 1.0
    save_png(nrm, paths["normal"], "Non-Color")
    return make_material(name, paths["albedo"], paths["spec"], paths["normal"])


# ---------------------------------------------------------------------------
# 部品
# ---------------------------------------------------------------------------
def load_part(name):
    target, fit, tex_size, ratio = PARTS[name]
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(MESHY_DIR, name, name + ".fbx"))
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == "MESH"]
    for o in new:
        if o.type != "MESH":
            bpy.data.objects.remove(o)
    obj = meshes[0]
    obj.name = name
    bpy.context.view_layer.update()
    # 変形を焼き込む
    obj.data.transform(obj.matrix_world)
    obj.matrix_world = Matrix.Identity(4)
    if name in PART_ROTATION:
        obj.data.transform(Matrix.Rotation(math.radians(PART_ROTATION[name]), 4, "Z"))
    co = np.array([v.co[:] for v in obj.data.vertices])
    mn, mx = co.min(0), co.max(0)
    k = target / ((mx[2] - mn[2]) if fit == "height" else (mx[0] - mn[0]))
    # 足元の真ん中を原点にして、高さをそろえる
    t = Matrix.Scale(k, 4) @ Matrix.Translation(Vector((-(mn[0] + mx[0]) / 2, -(mn[1] + mx[1]) / 2, -mn[2])))
    obj.data.transform(t)
    if ratio < 1.0:
        mod = obj.modifiers.new("dec", "DECIMATE")
        mod.ratio = ratio
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.data.materials.clear()
    obj.data.materials.append(part_textures(name, tex_size))
    size = (mx - mn) * k
    log(name, "size m", np.round(size, 2), "tris", sum(len(p.vertices) - 2 for p in obj.data.polygons))
    obj.hide_render = True
    obj.hide_set(True)
    return obj, size


def place(src, x, y, z, rot_deg, placed, scale=1.0):
    o = src.copy()
    o.data = src.data.copy()
    bpy.context.scene.collection.objects.link(o)
    o.hide_render = False
    o.hide_set(False)
    o.matrix_world = Matrix.Translation(to_blender(x, y, z)) @ Matrix.Rotation(math.radians(rot_deg), 4, "Z") @ Matrix.Scale(scale, 4)
    placed.append(o)
    return o


# ---------------------------------------------------------------------------
# 単純な形（床・壁・台座）
# ---------------------------------------------------------------------------
def box(name, gmin, gmax, mat, uv_meters, objs):
    """ゲーム座標の最小・最大の角で箱を作る。UV は面の向きごとに、uv_meters m で 1 になるよう平らに貼る。"""
    bm = bmesh.new()
    a = to_blender(*gmin); b = to_blender(*gmax)
    lo = Vector((min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)))
    hi = Vector((max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)))
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co = Vector((lo.x if v.co.x < 0 else hi.x, lo.y if v.co.y < 0 else hi.y, lo.z if v.co.z < 0 else hi.z))
    uv = bm.loops.layers.uv.new("UVMap")
    for f in bm.faces:
        n = f.normal
        for l in f.loops:
            p = l.vert.co
            if abs(n.z) > 0.5:
                u, v = p.x, p.y
            elif abs(n.x) > 0.5:
                u, v = p.y, p.z
            else:
                u, v = p.x, p.z
            l[uv].uv = (u / uv_meters, v / uv_meters)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(o)
    me.materials.append(mat)
    objs.append(o)
    return o


def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    os.makedirs(OUT_DIR, exist_ok=True)
    os.makedirs(COL_DIR, exist_ok=True)
    os.makedirs(PREVIEW_DIR, exist_ok=True)

    floor_mat = make_material(
        "BossFloor",
        os.path.join(OUT_DIR, "BossFloor_albedo.png"),
        os.path.join(OUT_DIR, "BossFloor_spec.png"),
        os.path.join(OUT_DIR, "BossFloor_normal.png"))
    wall_mat = make_material(
        "BossWall",
        os.path.join(OUT_DIR, "BossWall_albedo.png"),
        os.path.join(OUT_DIR, "BossWall_spec.png"),
        os.path.join(OUT_DIR, "BossWall_normal.png"))

    visual, col = [], []
    W, L, T = HALL_HALF_WIDTH, HALL_HALF_LENGTH, WALL_THICKNESS

    # 床（下に 0.5m の厚み）
    box("Floor", (-W - T, -0.5, -L - T), (W + T, 0.0, L + T), floor_mat, FLOOR_TEXTURE_METERS, visual)
    # 壁（左右・奥・入口）
    box("WallL", (-W - T, 0, -L - T), (-W, WALL_HEIGHT, L + T), wall_mat, 4.0, visual)
    box("WallR", (W, 0, -L - T), (W + T, WALL_HEIGHT, L + T), wall_mat, 4.0, visual)
    box("WallBack", (-W, 0, L), (W, WALL_HEIGHT, L + T), wall_mat, 4.0, visual)
    box("WallFront", (-W, 0, -L - T), (W, WALL_HEIGHT, -L), wall_mat, 4.0, visual)
    # 玉座の台座（下の段ほど広く、手前に出る）
    for i in range(DAIS_STEPS):
        top = DAIS_STEP_HEIGHT * (i + 1)
        z0 = DAIS_FRONT_Z + DAIS_STEP_DEPTH * i
        hw = DAIS_HALF_WIDTH - DAIS_STEP_SHRINK * i
        box(f"Dais{i}", (-hw, 0, z0), (hw, top, L), floor_mat, FLOOR_TEXTURE_METERS, visual)
    dais_top = DAIS_STEP_HEIGHT * DAIS_STEPS

    # 当たり判定: 壁・台座は見た目と同じ箱を使う。床は細かいマスに分けた面にする
    for o in list(visual):
        if o.name == "Floor":
            continue
        c = o.copy(); c.data = o.data.copy()
        bpy.context.scene.collection.objects.link(c)
        col.append(c)
    floor_grid(-W - T, W + T, -L - T, L + T, FLOOR_COLLISION_CELL, wall_mat, col)

    # Meshy の部品
    parts = {n: load_part(n) for n in PARTS}
    # 柱（左右に並べる。正面を広間の真ん中へ向ける）
    p_obj, p_size = parts["Pillar"]
    for z in PILLAR_ZS:
        for side in (-1, 1):
            place(p_obj, side * PILLAR_X, 0, z, 90 * side * -1, visual)
            hx, hz = p_size[0] / 2, p_size[1] / 2
            box("PillarCol", (side * PILLAR_X - hx, 0, z - hz), (side * PILLAR_X + hx, PARTS["Pillar"][0], z + hz), wall_mat, 4.0, col)
    # 窓（壁の内側に、正面を広間へ向けて付ける）
    w_obj, w_size = parts["Window"]
    for z in WINDOW_ZS:
        for side in (-1, 1):
            x = side * (W - w_size[1] / 2)
            place(w_obj, x, WINDOW_BASE_Y, z, 90 * side * -1, visual)
    # 玉座の後ろの大きな窓（奥の壁の内側に、入口の方へ向けて）
    place(w_obj, 0, BACK_WINDOW_BASE_Y, L - w_size[1] * BACK_WINDOW_SCALE / 2, 180, visual, BACK_WINDOW_SCALE)
    # 扉（入口の壁の内側）
    d_obj, d_size = parts["Door"]
    place(d_obj, 0, 0, -L + d_size[1] / 2, 0, visual)
    # 玉座（台座の上で、入口の方を向く）
    t_obj, t_size = parts["Throne"]
    place(t_obj, 0, dais_top, THRONE_Z, 180, visual)
    box("ThroneCol", (-t_size[0] / 2, dais_top, THRONE_Z - t_size[1] / 2),
        (t_size[0] / 2, dais_top + t_size[2], THRONE_Z + t_size[1] / 2), wall_mat, 4.0, col)
    # アーチ（同じ列の柱の上に載せる。正面を広間の真ん中へ向ける）
    a_obj, _ = parts["Arch"]
    pillar_height = PARTS["Pillar"][0]
    for z in ARCH_ZS:
        for side in (-1, 1):
            place(a_obj, side * PILLAR_X, pillar_height, z, 90 * side * -1, visual)
    # 燭台（台座の手前と入口の左右）
    c_obj, c_size = parts["Candelabrum"]
    # (x, y, z, 向き°, 大きさの倍率)。柱の前のものは広間の真ん中へ、台座の上のものは入口の方へ向ける
    candelabra = [(side * CANDELABRUM_X, 0, z, 90 * side * -1, 1.0) for z in PILLAR_ZS for side in (-1, 1)]
    candelabra += [(x, dais_top, z, 180, DAIS_CANDELABRUM_SCALE) for x, z in DAIS_CANDELABRA]
    c_wicks = find_wicks("Candelabrum", c_obj, CANDELABRUM_WICK_RADIUS)
    flames = []   # (置いた行列, ろうそくの先の位置, 大きさの倍率)
    for x, y, z, rot, k in candelabra:
        o = place(c_obj, x, y, z, rot, visual, k)
        flames.append((o.matrix_world.copy(), c_wicks, k))
        r = max(c_size[0], c_size[1]) / 2 * k
        box("CandelabrumCol", (x - r, y, z - r), (x + r, y + c_size[2] * k, z + r), wall_mat, 4.0, col)
    # 騎士の石像（玉座の左右、台座の上で入口の方を向く）
    s_obj, s_size = parts["Statue"]
    for side in (-1, 1):
        x = side * STATUE_X
        place(s_obj, x, dais_top, STATUE_Z, 180, visual)
        box("StatueCol", (x - s_size[0] / 2, dais_top, STATUE_Z - s_size[1] / 2),
            (x + s_size[0] / 2, dais_top + s_size[2], STATUE_Z + s_size[1] / 2), wall_mat, 4.0, col)
    # 垂れ幕（柱の広間側の面に、柱と同じ向きで掛ける。玉座の後ろの壁にも大きめを2本）
    b_obj, b_size = parts["Banner"]
    for z in PILLAR_ZS:
        for side in (-1, 1):
            x = side * (PILLAR_X - p_size[1] / 2 - b_size[1] / 2 - BANNER_GAP)
            place(b_obj, x, BANNER_BASE_Y, z, 90 * side * -1, visual)
    for side in (-1, 1):
        z = L - b_size[1] * WALL_BANNER_SCALE / 2 - BANNER_GAP
        place(b_obj, side * WALL_BANNER_X, WALL_BANNER_BASE_Y, z, 180, visual, WALL_BANNER_SCALE)
    # がれき（低い山なので、足元の小さめの箱だけ当たり判定にする）
    r_obj, r_size = parts["Rubble"]
    for x, z, rot, k in RUBBLE:
        place(r_obj, x, 0, z, rot, visual, k)
        hr = 0.35 * min(r_size[0], r_size[1]) * k
        box("RubbleCol", (x - hr, 0, z - hr), (x + hr, 0.6 * r_size[2] * k, z + hr), wall_mat, 4.0, col)
    # 壁の灯り（壁に背中を付け、ランタンを広間の方へ向ける）
    sc_obj, sc_size = parts["Sconce"]
    sc_wicks = find_wicks("Sconce", sc_obj, SCONCE_WICK_RADIUS)
    for x, wall in SCONCES:
        if wall == "front":
            z, rot = -L + sc_size[1] / 2, 0
        else:
            z, rot = L - sc_size[1] / 2, 180
        o = place(sc_obj, x, SCONCE_BASE_Y, z, rot, visual)
        flames.append((o.matrix_world.copy(), sc_wicks, 1.0))
    # 床の紋章（円の外は透明にして、床の上に少し浮かせて置く）
    emblem_disc(visual)
    # 通り道の両側の紫の光る帯
    rune_strips(visual)
    # 入口から玉座までの絨毯（紋章のところは空ける）
    carpet(dais_top, visual)

    for n, (o, _) in parts.items():
        bpy.data.objects.remove(o)

    # 天井と炎は影を落とさないよう、別のモデルにする（天井が影を落とすと、月の光が広間に入らなくなる）
    ceiling = []
    build_ceiling(wall_mat, floor_mat, ceiling)
    build_flames(flames, ceiling)

    export(visual, os.path.join(OUT_DIR, "BossHall.fbx"))
    export(ceiling, os.path.join(OUT_DIR, "BossHallNoShadow.fbx"))
    # 当たり判定用は、エンジンがフォルダーの別の画像を拾わないよう、専用のフォルダーに1色の画像と一緒に置く
    col_mat = flat_textures_in("BossHallCollision", COL_DIR)
    for o in col:
        o.data.materials.clear()
        o.data.materials.append(col_mat)
    export(col, os.path.join(COL_DIR, "BossHallCollision.fbx"))
    tris = sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in visual + ceiling)
    log("visual objects", len(visual) + len(ceiling), "tris", tris)
    for o in col:
        o.hide_render = True
    for o in ceiling:
        o.visible_shadow = False   # 確かめ用の画像でも、ゲームと同じく天井は影を落とさない
    render_previews(visual)


EMBLEM_RADIUS_IN_IMAGE = 0.492   # Meshy の画像で、円の外の金の輪までの半径（画像の幅に対する割合。画像で測った）
EMBLEM_LIFT = 0.03               # 床から浮かせる高さ（m）。遠くから見ても床とちらつかないように


def emblem_disc(objs):
    """床の紋章。Meshy の画像の円の外を透明にし、金の部分だけ少し金属にする。"""
    src = load_px(os.path.join(MESHY_DIR, "Emblem.png"), "sRGB")
    h, w = src.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    r = np.hypot((xx + 0.5) / w - 0.5, (yy + 0.5) / h - 0.5)
    alb = src.copy()
    alb[..., 3] = (r < EMBLEM_RADIUS_IN_IMAGE).astype(np.float32)
    # 金: 赤が青よりはっきり強いところ
    gold = np.clip((src[..., 0] - src[..., 2] - 0.12) * 4.0, 0, 1)
    spec = np.zeros_like(src)
    spec[..., 0] = gold * 0.6
    spec[..., 3] = 0.55 + 0.1 * gold
    nrm = np.ones_like(src); nrm[..., 0] = 0.5; nrm[..., 1] = 0.5
    paths = {k: os.path.join(OUT_DIR, f"Emblem_{k}.png") for k in ("albedo", "spec", "normal")}
    save_png(alb, paths["albedo"], "sRGB")
    save_png(spec, paths["spec"], "Non-Color")
    save_png(nrm, paths["normal"], "Non-Color")
    mat = make_material("Emblem", paths["albedo"], paths["spec"], paths["normal"])

    half = EMBLEM_DIAMETER / 2
    c = to_blender(0, EMBLEM_LIFT, EMBLEM_CENTER_Z)
    bm = bmesh.new()
    vs = [bm.verts.new(c + Vector((dx, dy, 0))) for dx, dy in ((-half, -half), (half, -half), (half, half), (-half, half))]
    f = bm.faces.new(vs)
    uv = bm.loops.layers.uv.new("UVMap")
    # 画像の上（王冠）がゲームの +Z（玉座の方）を向くように、画像を 180° 回して貼る
    for l, t in zip(f.loops, ((1, 1), (0, 1), (0, 0), (1, 0))):
        l[uv].uv = t
    me = bpy.data.meshes.new("Emblem")
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("Emblem", me)
    bpy.context.scene.collection.objects.link(o)
    me.materials.append(mat)
    objs.append(o)


def new_object(name, bm, mat, objs):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(o)
    me.materials.append(mat)
    objs.append(o)
    return o


def face_toward(bm, faces, target_of):
    """面の表が target_of(面) の点の方を向くよう、逆向きの面を裏返す（このエンジンは面の裏を描かない）。"""
    bm.normal_update()
    flip = [f for f in faces if f.normal.dot(target_of(f) - f.calc_center_median()) < 0]
    if flip:
        bmesh.ops.reverse_faces(bm, faces=flip)


def floor_grid(x0, x1, z0, z1, cell, mat, objs):
    """床の当たり判定。上の面だけを cell m のマスに分けて作る。"""
    bm = bmesh.new()
    nx = int(round((x1 - x0) / cell)); nz = int(round((z1 - z0) / cell))
    vs = [[bm.verts.new(to_blender(x0 + (x1 - x0) * i / nx, 0.0, z0 + (z1 - z0) * j / nz)) for j in range(nz + 1)] for i in range(nx + 1)]
    faces = [bm.faces.new((vs[i][j], vs[i + 1][j], vs[i + 1][j + 1], vs[i][j + 1])) for i in range(nx) for j in range(nz)]
    face_toward(bm, faces, lambda f: f.calc_center_median() + Vector((0, 0, 1)))
    new_object("FloorCol", bm, mat, objs)


def vault_profile():
    """天井の切り口（ゲームの x, y）。左の壁の上から、てっぺんを通って右の壁の上まで。
    左半分は、中心 (xc, 壁の高さ)・半径 R の円の弧（右半分はその鏡写し）。"""
    w, h = HALL_HALF_WIDTH, VAULT_RISE
    xc = (h * h - w * w) / (2 * w)     # (w + xc)^2 = xc^2 + h^2 から
    r = w + xc
    top = math.atan2(h, -xc)
    left = []
    for i in range(VAULT_SEGMENTS + 1):
        a = math.pi + (top - math.pi) * i / VAULT_SEGMENTS
        left.append((xc + r * math.cos(a), WALL_HEIGHT + r * math.sin(a), (xc, WALL_HEIGHT)))
    right = [(-x, y, (-cx, cy)) for x, y, (cx, cy) in reversed(left[:-1])]
    return left + right   # (x, y, 弧の中心)


def build_ceiling(wall_mat, rib_mat, objs):
    W, L, T = HALL_HALF_WIDTH, HALL_HALF_LENGTH, WALL_THICKNESS
    prof = vault_profile()
    arc = [0.0]
    for a, b in zip(prof, prof[1:]):
        arc.append(arc[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    apex_y = max(p[1] for p in prof)

    # 丸天井の面（下から見える向き）
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    nz = int(math.ceil(2 * (L + T) / VAULT_Z_STEP))
    zs = [-L - T + 2 * (L + T) * j / nz for j in range(nz + 1)]
    grid = [[bm.verts.new(to_blender(x, y, z)) for z in zs] for x, y, _ in prof]
    faces = []
    for i in range(len(prof) - 1):
        for j in range(nz):
            f = bm.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
            for l, (ii, jj) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                l[uv].uv = (arc[ii] / 4.0, zs[jj] / 4.0)
            faces.append(f)
    face_toward(bm, faces, lambda f: to_blender(0, WALL_HEIGHT, -f.calc_center_median().y))
    new_object("Vault", bm, wall_mat, objs)

    # 入口と奥の壁の上の続き（天井の切り口の形にふさぐ）
    for z_wall, inward in ((-L, 1), (L, -1)):
        bm = bmesh.new()
        uv = bm.loops.layers.uv.new("UVMap")
        f = bm.faces.new([bm.verts.new(to_blender(x, y, z_wall)) for x, y, _ in prof])
        for l in f.loops:
            p = l.vert.co
            l[uv].uv = (p.x / 4.0, p.z / 4.0)
        face_toward(bm, [f], lambda f: f.calc_center_median() + to_blender(0, 0, inward) - to_blender(0, 0, 0))
        bmesh.ops.triangulate(bm, faces=[f])
        new_object("Gable", bm, wall_mat, objs)

    # 天井の骨（リブ）: 柱の列とアーチの真ん中を横切るもの、てっぺんを縦に通るもの
    for zc in RIB_ZS:
        bm = bmesh.new()
        uv = bm.loops.layers.uv.new("UVMap")
        rails = []
        for x, y, (cx, cy) in prof:
            n = Vector((cx - x, cy - y)).normalized()   # 弧の中心の方 = 広間の内側
            ix, iy = x + n.x * RIB_DEPTH, y + n.y * RIB_DEPTH
            rails.append([bm.verts.new(to_blender(px, py, zc + dz)) for px, py, dz in
                          ((x, y, -RIB_WIDTH / 2), (ix, iy, -RIB_WIDTH / 2), (ix, iy, RIB_WIDTH / 2), (x, y, RIB_WIDTH / 2))])
        for i in range(len(rails) - 1):
            for k in range(4):
                f = bm.faces.new((rails[i][k], rails[i + 1][k], rails[i + 1][(k + 1) % 4], rails[i][(k + 1) % 4]))
                for l, s in zip(f.loops, (arc[i], arc[i + 1], arc[i + 1], arc[i])):
                    l[uv].uv = (s / FLOOR_TEXTURE_METERS, 0.08 + k * 0.02)   # 床の模様の金の目地（v が 0.25 ごと）を避けて、大理石の中を使う
        bm.faces.new(rails[0]); bm.faces.new(list(reversed(rails[-1])))
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        new_object("Rib", bm, rib_mat, objs)
    box("RidgeRib", (-RIB_WIDTH / 2, apex_y - RIB_DEPTH, -L), (RIB_WIDTH / 2, apex_y, L), rib_mat, FLOOR_TEXTURE_METERS, objs)


def find_wicks(name, obj, radius):
    """部品のろうそくの先（部品の足元が原点の、Blender の座標）を見つける。"""
    alb = load_px(os.path.join(MESHY_DIR, name, name + ".png"), "sRGB", 512)
    me = obj.data
    uvl = me.uv_layers.active.data
    pts = []
    for li, loop in enumerate(me.loops):
        u, v = uvl[li].uv
        c = alb[int((v % 1.0) * 511), int((u % 1.0) * 511), :3]
        if c.min() > WAX_MIN and c.max() - c.min() < WAX_MAX_SPREAD:
            p = me.vertices[loop.vertex_index].co
            zmax, ymin, ymax = WICK_AREA.get(name, (1e9, -1e9, 1e9))
            if p.z < zmax and ymin < p.y < ymax:
                pts.append(p.copy())
    pts.sort(key=lambda p: -p.z)
    clusters = []
    for p in pts:
        for c in clusters:
            if (p.xy - c[0].xy).length < radius:
                c.append(p)
                break
        else:
            clusters.append([p])
    wicks = []
    for c in clusters:
        if len(c) < WICK_MIN_POINTS:
            continue
        top = c[0].z
        near = [p for p in c if p.z > top - 0.02]   # 上の面の点の真ん中を芯にする
        wicks.append(Vector((sum(p.x for p in near) / len(near), sum(p.y for p in near) / len(near), top)))
    # 高いろうそくのすぐ下にある白いところ（ろうそくの側面や、垂れたろう）は捨てる
    wicks.sort(key=lambda w: -w.z)
    kept = []
    for w in wicks:
        if all((w.xy - k.xy).length >= radius * 1.6 for k in kept):
            kept.append(w)
    wicks = kept
    for w in wicks:
        log(name, "wick", tuple(round(x, 3) for x in w))
    log(name, "wicks", len(wicks), "top m", round(max(w.z for w in wicks), 3) if wicks else None,
        "center", tuple(round(sum(w[i] for w in wicks) / max(len(wicks), 1), 3) for i in range(2)))
    return wicks


def flame_texture(path_prefix):
    """炎の形（しずくの形）と色。外は透明（アルファが 0.2 より小さいところはエンジンが描かない）。"""
    w, h = 64, 128
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    v = (yy + 0.5) / h                       # 0 が下（Blender の画像は下の行から）
    u = (xx + 0.5) / w * 2.0 - 1.0
    half = 0.9 * np.sin(np.pi * np.clip(v, 0, 1) ** 0.6) * (1.0 - v) ** 0.35
    inside = np.abs(u) < half
    core = np.clip(1.0 - np.hypot(u / 0.45, (v - 0.3) / 0.35), 0, 1)
    # 芯を白にすると、ライトが弱く当たる炎で夜空の青緑が混ざって緑っぽく見えるので、芯も黄色寄りにする
    outer = np.array([1.0, 0.42, 0.08]); inner = np.array([1.0, 0.78, 0.35])
    alb = np.zeros((h, w, 4), np.float32)
    alb[..., :3] = outer + (inner - outer) * core[..., None]
    alb[..., 3] = inside.astype(np.float32)
    spec = np.zeros((h, w, 4), np.float32)
    # エンジンは法線マップも sRGB として DDS にするので、読むときに 0.735 が 0.5 に戻る。0.5 のままだと法線が斜めに傾き、
    # 炎の板が横向きの面として照らされる（下のライトが当たらず、夜空の青緑を拾う）。
    nrm = np.ones((h, w, 4), np.float32); nrm[..., 0] = FLAT_NORMAL_SRGB; nrm[..., 1] = FLAT_NORMAL_SRGB
    paths = {k: f"{path_prefix}_{k}.png" for k in ("albedo", "spec", "normal")}
    save_png(alb, paths["albedo"], "sRGB")
    save_png(spec, paths["spec"], "Non-Color")
    save_png(nrm, paths["normal"], "Non-Color")
    return paths


def build_flames(flames, objs):
    """ろうそくの先に、3枚の板を 60° ずつ回して組んだ炎を立てる（どこから見ても炎に見える）。
    板の法線は全部真下に向け、すぐ下のライトだけで照らす。真上に向けると、上からの青い月の光と夜空の映り込みを拾って、炎が緑っぽくなる。"""
    paths = flame_texture(os.path.join(OUT_DIR, "Flame"))
    mat = make_material("Flame", paths["albedo"], paths["spec"], paths["normal"])
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    lights = []
    for mw, wicks, k in flames:
        for wick in wicks:
            base = mw @ wick
            hgt, wid = FLAME_HEIGHT * k, FLAME_WIDTH * k
            for i in range(3):
                a = math.radians(60 * i)
                d = Vector((math.cos(a), math.sin(a), 0)) * wid / 2
                offs = (-d, d, d + Vector((0, 0, hgt)), -d + Vector((0, 0, hgt)))
                for order in ((0, 1, 2, 3), (1, 0, 3, 2)):   # 表と裏（エンジンは面の裏を描かない）。同じ頂点で2面は作れないので頂点も分ける
                    vs = [bm.verts.new(base + off) for off in offs]
                    f = bm.faces.new([vs[j] for j in order])
                    for l, j in zip(f.loops, order):
                        l[uv].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[j]
        # 炎の段（高さがそろった炎の組）ごとに、その段の真ん中の、根元のすぐ下に1つ。1つで全部の段を照らすと、遠い段に届かない
        for tier in flame_tiers(wicks):
            c = mw @ Vector((sum(w.x for w in tier) / len(tier), sum(w.y for w in tier) / len(tier), min(w.z for w in tier)))
            lights.append(c - Vector((0, 0, FLAME_LIGHT_BELOW * k)))
    o = new_object("Flames", bm, mat, objs)
    me = o.data
    me.normals_split_custom_set([(0.0, 0.0, -1.0)] * len(me.loops))
    for p in lights:
        # ゲームの cm（x, 高さ, z）。BossStage.cpp の炎のライトはこの位置に合わせる
        log("FLAME LIGHT cm", round(p.x * 100), round(p.z * 100), round(-p.y * 100))


def flame_tiers(wicks):
    """ろうそくの先を、高さが FLAME_TIER_GAP 以内のものどうしの段に分ける。"""
    tiers = []
    for w in sorted(wicks, key=lambda w: w.z):
        if tiers and w.z - tiers[-1][-1].z < FLAME_TIER_GAP:
            tiers[-1].append(w)
        else:
            tiers.append([w])
    return tiers


def carpet_texture(path_prefix):
    """深い紫のビロードに、左右の金の縁と、真ん中に並ぶ小さな金の飾り。縦の端がつながる。"""
    w, h = 512, 1024                   # 幅 3m・長さ 6m
    rng = np.random.default_rng(3)
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    u = (xx + 0.5) / w; v = (yy + 0.5) / h
    # 地: 深い紫（月の光と自動露出で明るく見えるので、暗めにする）と、少し暗い菱形の織り模様
    dx = np.abs(((u * 6) % 1.0) - 0.5); dy = np.abs(((v * 12) % 1.0) - 0.5)
    diamond = (np.abs(dx + dy - 0.5) < 0.04).astype(np.float32)
    noise = rng.normal(0, 1, (h, w)).astype(np.float32)
    noise = (noise + np.roll(noise, 1, 0) + np.roll(noise, 1, 1) + np.roll(noise, -1, 0)) * 0.25
    base = np.array([0.06, 0.015, 0.085]) * (1.0 + 0.08 * noise[..., None]) * (1.0 - 0.35 * diamond[..., None])
    # 金: 左右の縁の2本線と、真ん中の飾り（1m ごとの菱形）
    edge = np.minimum(u, 1 - u)
    gold = ((edge > 0.04) & (edge < 0.07)) | ((edge > 0.10) & (edge < 0.115))
    cv = np.abs(((v * 6) % 1.0) - 0.5) * 6.0      # 1m ごと
    cu = np.abs(u - 0.5) * 6.0 / 1.0
    gold |= (np.abs(cu + cv - 0.6) < 0.08) & (cu + cv < 1.0)
    gold = gold.astype(np.float32)
    gold_col = np.array([0.77, 0.64, 0.42])
    alb = np.ones((h, w, 4), np.float32)
    alb[..., :3] = base * (1 - gold[..., None]) + gold_col * gold[..., None]
    spec = np.zeros((h, w, 4), np.float32)
    spec[..., 0] = gold * 0.8
    spec[..., 3] = 0.15 + 0.4 * gold          # 布はつやを消す
    nrm = np.ones((h, w, 4), np.float32); nrm[..., 0] = 0.5; nrm[..., 1] = 0.5
    paths = {k: f"{path_prefix}_{k}.png" for k in ("albedo", "spec", "normal")}
    save_png(alb, paths["albedo"], "sRGB")
    save_png(spec, paths["spec"], "Non-Color")
    save_png(nrm, paths["normal"], "Non-Color")
    return paths


def carpet(dais_top, objs):
    """入口の扉の前から玉座の前まで。床の紋章のところは空け、台座の段は踏み面と蹴上げに沿わせる。"""
    paths = carpet_texture(os.path.join(OUT_DIR, "Carpet"))
    mat = make_material("Carpet", paths["albedo"], paths["spec"], paths["normal"])
    e0 = EMBLEM_CENTER_Z - EMBLEM_DIAMETER / 2 - CARPET_EMBLEM_GAP
    e1 = EMBLEM_CENTER_Z + EMBLEM_DIAMETER / 2 + CARPET_EMBLEM_GAP
    # 道筋: (y, z) の点を順につなぐ。段では、蹴上げ（縦）と踏み面（横）を交互に
    path = [[(0.0, CARPET_Z_START), (0.0, e0)], [(0.0, e1), (0.0, DAIS_FRONT_Z)]]
    for i in range(DAIS_STEPS):
        z = DAIS_FRONT_Z + DAIS_STEP_DEPTH * i
        path[-1].append((DAIS_STEP_HEIGHT * (i + 1), z))
        z_next = DAIS_FRONT_Z + DAIS_STEP_DEPTH * (i + 1) if i < DAIS_STEPS - 1 else CARPET_END_Z
        path[-1].append((DAIS_STEP_HEIGHT * (i + 1), z_next))
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    length = 0.0
    for run in path:
        for (y0, z0), (y1, z1) in zip(run, run[1:]):
            riser = abs(z1 - z0) < 1e-6
            # 踏み面は上に、蹴上げは入口の方（-Z）に少しずらして、下の面とちらつかないようにする
            oy, oz = (0.0, -CARPET_LIFT) if riser else (CARPET_LIFT, 0.0)
            seg = math.hypot(y1 - y0, z1 - z0)
            corners = [(-CARPET_HALF_WIDTH, y0, z0, 0, length), (CARPET_HALF_WIDTH, y0, z0, 1, length),
                       (CARPET_HALF_WIDTH, y1, z1, 1, length + seg), (-CARPET_HALF_WIDTH, y1, z1, 0, length + seg)]
            f = bm.faces.new([bm.verts.new(to_blender(x, y + oy, z + oz)) for x, y, z, _, _ in corners])
            for l, (_, _, _, cu, cl) in zip(f.loops, corners):
                l[uv].uv = (cu, cl / CARPET_TEXTURE_METERS)
            up = to_blender(0, 0, -1) if riser else to_blender(0, 1, 0)
            face_toward(bm, [f], lambda f, up=up: f.calc_center_median() + up)
            length += seg
    new_object("Carpet", bm, mat, objs)


def rune_texture(path_prefix):
    """紫の光る帯の模様。細長い帯に、縁の線と、ルーン文字のような短い線の組み合わせを並べる。"""
    w, h = 64, 1024                    # 幅 0.4m・長さ 4m
    rng = np.random.default_rng(5)
    img = np.zeros((h, w), np.float32)
    # 縁の線
    img[:, 3:6] = 1.0; img[:, w - 6:w - 3] = 1.0
    # 文字: 1文字 = 縦 64 ピクセル。3×4 の点を結ぶ線を 3〜4 本
    cell = 64
    for c in range(h // cell):
        y0 = c * cell + 10; x0 = 14
        pts = [(x0 + i * 18, y0 + j * 14) for i in range(3) for j in range(4)]
        for _ in range(rng.integers(3, 5)):
            a, b = rng.choice(len(pts), 2, replace=False)
            (ax, ay), (bx, by) = pts[a], pts[b]
            for t in np.linspace(0, 1, 60):
                px, py = int(ax + (bx - ax) * t), int(ay + (by - ay) * t)
                img[max(py - 2, 0):py + 2, max(px - 2, 0):px + 2] = 1.0
    dark = np.array([0.03, 0.02, 0.06]); glow = np.array([0.75, 0.45, 1.0])
    alb = np.ones((h, w, 4), np.float32)
    alb[..., :3] = dark + (glow - dark) * img[..., None]
    spec = np.zeros((h, w, 4), np.float32); spec[..., 3] = 0.6
    nrm = np.ones((h, w, 4), np.float32); nrm[..., 0] = 0.5; nrm[..., 1] = 0.5
    paths = {k: f"{path_prefix}_{k}.png" for k in ("albedo", "spec", "normal")}
    save_png(alb, paths["albedo"], "sRGB")
    save_png(spec, paths["spec"], "Non-Color")
    save_png(nrm, paths["normal"], "Non-Color")
    return paths


def rune_strips(objs):
    paths = rune_texture(os.path.join(OUT_DIR, "Rune"))
    mat = make_material("Rune", paths["albedo"], paths["spec"], paths["normal"])
    for side in (-1, 1):
        x0, x1 = side * RUNE_X - RUNE_WIDTH / 2, side * RUNE_X + RUNE_WIDTH / 2
        bm = bmesh.new()
        uv = bm.loops.layers.uv.new("UVMap")
        corners = ((x0, RUNE_Z_START, 0), (x1, RUNE_Z_START, 1), (x1, RUNE_Z_END, 1), (x0, RUNE_Z_END, 0))
        f = bm.faces.new([bm.verts.new(to_blender(x, RUNE_LIFT, z)) for x, z, _ in corners])
        for l, (x, z, u) in zip(f.loops, corners):
            l[uv].uv = (u, z / RUNE_TEXTURE_METERS)
        face_toward(bm, [f], lambda f: f.calc_center_median() + Vector((0, 0, 1)))
        new_object("Rune", bm, mat, objs)


def flat_textures_in(name, folder):
    n = 4
    alb = np.ones((n, n, 4), np.float32); alb[..., :3] = 0.5
    p = os.path.join(folder, f"{name}_albedo.png")
    save_png(alb, p, "sRGB")
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    t = mat.node_tree.nodes.new("ShaderNodeTexImage")
    t.image = bpy.data.images.load(p)
    mat.node_tree.links.new(t.outputs["Color"], mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"])
    return mat


def export(objects, path):
    for o in bpy.context.scene.objects:
        o.select_set(o in objects)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={"MESH"},
        apply_unit_scale=False, apply_scale_options="FBX_SCALE_NONE", global_scale=1.0,
        axis_forward="Y", axis_up="Z", bake_space_transform=False, use_mesh_modifiers=True,
        mesh_smooth_type="OFF", add_leaf_bones=False, bake_anim=False,
        path_mode="STRIP", embed_textures=False)
    log("EXPORTED", path)


# ---------------------------------------------------------------------------
# 確かめ用の画像
# ---------------------------------------------------------------------------
def render_previews(visual):
    sc = bpy.context.scene
    try:
        sc.render.engine = "BLENDER_EEVEE"
    except TypeError:
        sc.render.engine = "BLENDER_EEVEE_NEXT"
    sc.render.resolution_x, sc.render.resolution_y = 1280, 720
    w = bpy.data.worlds.new("night"); sc.world = w; w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.02, 0.03, 0.07, 1)
    w.node_tree.nodes["Background"].inputs[1].default_value = float(os.environ.get("BOSS_PREVIEW_WORLD", "1.0"))
    sun = bpy.data.objects.new("moon", bpy.data.lights.new("moon", "SUN"))
    sc.collection.objects.link(sun)
    sun.data.energy = 2.5
    sun.data.color = (0.6, 0.7, 1.0)
    sun.rotation_euler = (math.radians(35), math.radians(25), math.radians(0))
    for side in (-1, 1):
        pl = bpy.data.objects.new("candle", bpy.data.lights.new("candle", "POINT"))
        sc.collection.objects.link(pl)
        pl.data.energy = 1500; pl.data.color = (1.0, 0.6, 0.3)
        pl.location = to_blender(side * 5.0, 2.5, 12.0)
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    sc.collection.objects.link(cam); sc.camera = cam
    cam.data.lens = 20
    for name, pos, target in [
        ("entrance", (0, 2.0, -21), (0, 3.0, 20)),
        ("high", (-12, 10, -22), (2, 0, 8)),
        ("throne", (3, 2.0, 6), (0, 3.5, 21)),
        ("door", (3, 3.0, -12), (-2, 3.0, -24)),
        ("ceiling", (0, 3.0, -20), (0, 26.0, 4)),
        ("candle", (-4.6, 2.9, -9.0), (-6.8, 2.5, -10.0)),
        ("sconce", (-4.0, 4.2, -21.6), (-5.0, 4.0, -23.4)),
        ("carpet", (0, 3.0, 8.0), (0, 0.5, 18.0)),
    ]:
        cam.location = to_blender(*pos)
        cam.rotation_euler = (to_blender(*target) - cam.location).to_track_quat("-Z", "Y").to_euler()
        sc.render.filepath = os.path.join(PREVIEW_DIR, f"hall_{name}.png")
        bpy.ops.render.render(write_still=True)
        log("PREVIEW", sc.render.filepath)


build()
