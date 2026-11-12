"""ボス戦の広間（月明かりの玉座の間）を、Meshy で作った部品と単純な形から組み立てる。

Blender をウィンドウなしで動かす:
    blender -b --factory-startup -P make_boss_stage.py

入力:
    MESHY_DIR/<部品>/<部品>.fbx と <部品>.png・_metallic.png・_roughness.png・_normal.png
    （Meshy の zip を展開し、ファイル名の頭を部品名にしたもの。リポジトリには入れない）
    Game/Assets/modelData/stage/boss/BossFloor_*.png（make_floor_texture.py で作る）

出力（Game/Assets/modelData/stage/boss/）:
    BossHall.fbx       見た目（部品と床・壁・台座をまとめた1つのモデル）
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
WALL_HEIGHT = 12.0
WALL_THICKNESS = 1.0
FLOOR_TEXTURE_METERS = 8.0  # 床のテクスチャ1枚（タイル 4×4 枚）を何 m 四方に貼るか（タイル1枚 2m）

# ---- 部品: (目標の大きさ m, 合わせる向き, テクスチャの1辺, 三角形を減らす割合) ----
# 合わせる向き: "height" は高さ、"width" は幅（Blender の X）をその大きさにする。
PARTS = {
    "Throne":      (4.0, "height", 2048, 1.0),
    "Door":        (8.0, "height", 2048, 0.6),
    "Pillar":      (9.0, "height", 2048, 0.35),
    "Window":      (7.5, "height", 2048, 0.35),
    "Arch":        (9.4, "width",  2048, 1.0),
    "Candelabrum": (2.6, "height", 1024, 0.27),
    "Statue":      (3.2, "height", 2048, 0.7),
}

# ---- 置き方 ----
PILLAR_X = 11.0
PILLAR_ZS = [-18.0, -10.0, -2.0, 6.0, 14.0]
WINDOW_ZS = [-14.0, -6.0, 2.0, 10.0, 18.0]
WINDOW_BASE_Y = 1.0
DAIS_HALF_WIDTH = 6.0
DAIS_FRONT_Z = 14.0
DAIS_STEPS = 4
DAIS_STEP_HEIGHT = 0.3
DAIS_STEP_DEPTH = 1.0
THRONE_Z = 21.0
ARCH_ZS = [-14.0, -6.0, 2.0, 10.0]          # 同じ列の柱と柱の真ん中
CANDELABRA = [(-4.5, 12.5), (4.5, 12.5), (-4.0, -21.0), (4.0, -21.0)]   # (x, z)。台座の手前と入口の左右
STATUE_X = 3.2                               # 玉座の左右（台座の上）
EMBLEM_CENTER_Z = 2.0                        # 床の紋章の真ん中（戦う場所）
EMBLEM_DIAMETER = 8.0

WALL_COLOR = (0.055, 0.060, 0.075)   # 壁の色（sRGB）。灰色がかった青の暗い石
WALL_SMOOTH = 0.35
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


def flat_textures(name, color_srgb, smooth):
    """1色のテクスチャ（法線は平ら）。"""
    n = 16
    alb = np.ones((n, n, 4), np.float32); alb[..., :3] = color_srgb
    spec = np.zeros((n, n, 4), np.float32); spec[..., 3] = smooth
    nrm = np.ones((n, n, 4), np.float32); nrm[..., 0] = 0.5; nrm[..., 1] = 0.5
    paths = {k: os.path.join(OUT_DIR, f"{name}_{k}.png") for k in ("albedo", "spec", "normal")}
    save_png(alb, paths["albedo"], "sRGB")
    save_png(spec, paths["spec"], "Non-Color")
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


def place(src, x, y, z, rot_deg, placed):
    o = src.copy()
    o.data = src.data.copy()
    bpy.context.scene.collection.objects.link(o)
    o.hide_render = False
    o.hide_set(False)
    o.matrix_world = Matrix.Translation(to_blender(x, y, z)) @ Matrix.Rotation(math.radians(rot_deg), 4, "Z")
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
    wall_mat = flat_textures("BossWall", WALL_COLOR, WALL_SMOOTH)

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
        hw = DAIS_HALF_WIDTH - 0.5 * i
        box(f"Dais{i}", (-hw, 0, z0), (hw, top, L), floor_mat, FLOOR_TEXTURE_METERS, visual)
    dais_top = DAIS_STEP_HEIGHT * DAIS_STEPS

    # 当たり判定: 床・壁・台座は見た目と同じ箱を使う
    for o in list(visual):
        c = o.copy(); c.data = o.data.copy()
        bpy.context.scene.collection.objects.link(c)
        col.append(c)

    # Meshy の部品
    parts = {n: load_part(n) for n in PARTS}
    # 柱（左右に並べる。正面を広間の真ん中へ向ける）
    p_obj, p_size = parts["Pillar"]
    for z in PILLAR_ZS:
        for side in (-1, 1):
            place(p_obj, side * PILLAR_X, 0, z, 90 * side * -1, visual)
            hx, hz = p_size[0] / 2, p_size[1] / 2
            box("PillarCol", (side * PILLAR_X - hx, 0, z - hz), (side * PILLAR_X + hx, 9.0, z + hz), wall_mat, 4.0, col)
    # 窓（壁の内側に、正面を広間へ向けて付ける）
    w_obj, w_size = parts["Window"]
    for z in WINDOW_ZS:
        for side in (-1, 1):
            x = side * (W - w_size[1] / 2)
            place(w_obj, x, WINDOW_BASE_Y, z, 90 * side * -1, visual)
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
    for x, z in CANDELABRA:
        place(c_obj, x, 0, z, 180 if z > 0 else 0, visual)
        r = max(c_size[0], c_size[1]) / 2
        box("CandelabrumCol", (x - r, 0, z - r), (x + r, c_size[2], z + r), wall_mat, 4.0, col)
    # 騎士の石像（玉座の左右、台座の上で入口の方を向く）
    s_obj, s_size = parts["Statue"]
    for side in (-1, 1):
        x = side * STATUE_X
        place(s_obj, x, dais_top, THRONE_Z, 180, visual)
        box("StatueCol", (x - s_size[0] / 2, dais_top, THRONE_Z - s_size[1] / 2),
            (x + s_size[0] / 2, dais_top + s_size[2], THRONE_Z + s_size[1] / 2), wall_mat, 4.0, col)
    # 床の紋章（円の外は透明にして、床の上に少し浮かせて置く）
    emblem_disc(visual)

    for n, (o, _) in parts.items():
        bpy.data.objects.remove(o)

    export(visual, os.path.join(OUT_DIR, "BossHall.fbx"))
    # 当たり判定用は、エンジンがフォルダーの別の画像を拾わないよう、専用のフォルダーに1色の画像と一緒に置く
    col_mat = flat_textures_in("BossHallCollision", COL_DIR)
    for o in col:
        o.data.materials.clear()
        o.data.materials.append(col_mat)
    export(col, os.path.join(COL_DIR, "BossHallCollision.fbx"))
    tris = sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in visual)
    log("visual objects", len(visual), "tris", tris)
    for o in col:
        o.hide_render = True
    render_previews(visual)


EMBLEM_RADIUS_IN_IMAGE = 0.492   # Meshy の画像で、円の外の金の輪までの半径（画像の幅に対する割合。画像で測った）
EMBLEM_LIFT = 0.01               # 床から浮かせる高さ（m）。床とちらつかないように


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
    w.node_tree.nodes["Background"].inputs[1].default_value = 1.0
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
    ]:
        cam.location = to_blender(*pos)
        cam.rotation_euler = (to_blender(*target) - cam.location).to_track_quat("-Z", "Y").to_euler()
        sc.render.filepath = os.path.join(PREVIEW_DIR, f"hall_{name}.png")
        bpy.ops.render.render(write_still=True)
        log("PREVIEW", sc.render.filepath)


build()
