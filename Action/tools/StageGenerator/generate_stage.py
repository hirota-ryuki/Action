# -*- coding: utf-8 -*-
"""
草原のステージを作るスクリプト（Blender 5.2 で動かす）。

    blender.exe --background --factory-startup --python generate_stage.py -- [--no-preview]

やること:
  1. field_settings.json を読む（乱数の種が同じなら、同じステージができる）
  2. ノイズから地形の高さを決め、区画ごとに地形のメッシュ（近い用・遠い用）を作る
  3. 坂の角度・高さ・道から地面の色を決め、区画ごとに 1 枚のテクスチャに塗る
  4. MegaKit の部品を大きさをそろえて書き出す（葉・草は裏からも見えるよう裏返しの面を足す）
  5. 決まりに従って部品の置き場所を決め、配置 JSON に書き出す
  6. 確かめ用の画像を描き出す

座標:
  Blender は X が東、Y が北、Z が上で、単位は m。
  ゲームは Y が上で、単位は cm。地形と部品は Z が上のまま FBX に書き出し、ゲームでは
  enModelUpAxisZ で読む。エンジンはそのとき X 軸で -90 度回すので、
  Blender の (x, y, z) はゲームの (100x, 100z, -100y) になる。配置 JSON にはゲームの座標を書く。
"""
import bpy
import bmesh
import json
import math
import os
import shutil
import sys

import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ACTION_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "..", ".."))
GAME_DIR = os.path.join(ACTION_DIR, "Game")
FIELD_DIR = os.path.join(GAME_DIR, "Assets", "modelData", "stage", "field")
NATURE_DIR = os.path.join(GAME_DIR, "Assets", "modelData", "stage", "nature")
NATURE_TEX_DIR = os.path.join(NATURE_DIR, "Textures")
PROPS_JSON_PATH = os.path.join(GAME_DIR, "Assets", "data", "stage", "field_props.json")
PREVIEW_DIR = os.path.join(SCRIPT_DIR, "preview")
BLEND_PATH = os.path.join(SCRIPT_DIR, "preview", "field.blend")

MEGAKIT_DIR = r"D:\03_data\StylizedNatureMegaKit"
MEGAKIT_FBX_DIR = os.path.join(MEGAKIT_DIR, "FBX")
MEGAKIT_TEX_DIR = os.path.join(MEGAKIT_DIR, "Textures")

# ゲームの中でのパス（ModelRender::Init に渡す形）。
GAME_FIELD_PATH = "Assets/modelData/stage/field/"
GAME_NATURE_PATH = "Assets/modelData/stage/nature/"

M_TO_CM = 100.0


def log(msg):
    print("[generate_stage] " + msg, flush=True)


# ---------------------------------------------------------------------------
# ノイズ（numpy で配列のまま計算する。地形の頂点とテクスチャで同じ関数を使う）
# ---------------------------------------------------------------------------
def hash2(ix, iy, seed):
    """整数の格子点から 0〜1 の乱数を作る。"""
    h = (ix.astype(np.int64) * 374761393 + iy.astype(np.int64) * 668265263 + seed * 1442695041) & 0xFFFFFFFF
    h = (h ^ (h >> 13)) * 1274126177 & 0xFFFFFFFF
    h = h ^ (h >> 16)
    return (h & 0xFFFFFF).astype(np.float64) / float(0xFFFFFF)


def value_noise(x, y, seed):
    """-1〜1 の滑らかなノイズ。"""
    ix = np.floor(x)
    iy = np.floor(y)
    fx = x - ix
    fy = y - iy
    ux = fx * fx * (3.0 - 2.0 * fx)
    uy = fy * fy * (3.0 - 2.0 * fy)
    a = hash2(ix, iy, seed)
    b = hash2(ix + 1, iy, seed)
    c = hash2(ix, iy + 1, seed)
    d = hash2(ix + 1, iy + 1, seed)
    v = a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy
    return v * 2.0 - 1.0


def fbm(x, y, octaves, seed):
    total = np.zeros_like(x, dtype=np.float64)
    amp = 0.5
    freq = 1.0
    norm = 0.0
    for i in range(octaves):
        total += value_noise(x * freq, y * freq, seed + i * 101) * amp
        norm += amp
        amp *= 0.5
        freq *= 2.0
    return total / norm


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


# ---------------------------------------------------------------------------
# 地形
# ---------------------------------------------------------------------------
class Terrain:
    def __init__(self, settings):
        self.s = settings
        self.t = settings["terrain"]
        self.seed = settings["seed"]
        self.path = np.array(settings["path"]["points"], dtype=np.float64)
        self.path_width = settings["path"]["width"]
        self.path_edge_noise = settings["path"]["edgeNoise"]

    def path_distance(self, x, y):
        """道（折れ線）までの距離。"""
        best = np.full_like(x, 1e9, dtype=np.float64)
        for i in range(len(self.path) - 1):
            ax, ay = self.path[i]
            bx, by = self.path[i + 1]
            dx, dy = bx - ax, by - ay
            ll = dx * dx + dy * dy
            t = np.clip(((x - ax) * dx + (y - ay) * dy) / ll, 0.0, 1.0)
            px = ax + t * dx - x
            py = ay + t * dy - y
            best = np.minimum(best, np.sqrt(px * px + py * py))
        # 縁を少しぎざぎざにする。
        best = best + fbm(x / 3.0, y / 3.0, 2, self.seed + 900) * self.path_edge_noise
        return best

    def cliff_t(self, x, y):
        """崖の下が 0、崖の上が 1。"""
        t = self.t
        edge = t["cliffY"] + fbm(x / t["cliffWarpScale"], np.full_like(x, 3.7), 3, self.seed + 500) * t["cliffWarp"]
        return smoothstep(edge - t["cliffWidth"] * 0.5, edge + t["cliffWidth"] * 0.5, y)

    def height(self, x, y):
        t = self.t
        h = fbm(x / t["hillScale"], y / t["hillScale"], 4, self.seed) * t["hillHeight"]
        h += fbm(x / t["detailScale"], y / t["detailScale"], 2, self.seed + 77) * t["detailHeight"]
        # 崖。上の段は丘の高さに崖の高さを足す。
        h += self.cliff_t(x, y) * t["cliffHeight"]
        # 真ん中（プレイヤーが出てくるところ）は平らにして高さ 0 にそろえる。
        r = np.sqrt(x * x + y * y)
        k = smoothstep(t["flatRadius"], t["flatRadius"] + t["flatBlend"], r)
        h = h * k
        # 道は少しへこませる。
        pd = self.path_distance(x, y)
        h -= (1.0 - smoothstep(0.0, self.path_width + 0.8, pd)) * 0.12
        return h

    def slope(self, x, y, eps=0.25):
        """坂の傾き（tan）。"""
        hx = (self.height(x + eps, y) - self.height(x - eps, y)) / (2 * eps)
        hy = (self.height(x, y + eps) - self.height(x, y - eps)) / (2 * eps)
        return np.sqrt(hx * hx + hy * hy)


# ---------------------------------------------------------------------------
# Blender の操作
# ---------------------------------------------------------------------------
def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def deselect_all():
    for o in bpy.context.scene.objects:
        o.select_set(False)


def export_fbx(objects, path):
    """エンジンの FBX インポーターは単位の設定を見ず、ノードの変形まで含めた座標をそのまま使う。
    この設定で書き出すと、Blender の 1m がファイルの中で 100（cm）になる（tkm の大きさで確かめた）。"""
    deselect_all()
    for o in objects:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        object_types={'MESH'},
        apply_unit_scale=False,
        apply_scale_options='FBX_SCALE_NONE',
        global_scale=1.0,
        axis_forward='Y',
        axis_up='Z',
        bake_space_transform=False,
        use_mesh_modifiers=True,
        mesh_smooth_type='OFF',
        add_leaf_bones=False,
        bake_anim=False,
        path_mode='STRIP',
        embed_textures=False,
    )


def import_fbx_in_meters(path, expected_size_m):
    """書き出した FBX を確かめ用に読み戻す。単位の扱いで大きさが 10 の累乗倍ずれるので、
    わかっている大きさ（m）に合わせ直す。戻り値は読んだオブジェクト。"""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == 'MESH']
    if meshes and expected_size_m > 0:
        bpy.context.view_layer.update()
        actual = max(max(o.dimensions) for o in meshes)
        if actual > 0:
            k = 10.0 ** round(math.log10(expected_size_m / actual))
            if k != 1.0:
                for o in new:
                    if o.parent is None:
                        o.scale = [v * k for v in o.scale]
                        o.location = [v * k for v in o.location]
    return new


def make_image_material(name, image_path):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(image_path, check_existing=True)
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 1.0
    return mat


def save_png(pixels_rgb, size, path):
    """pixels_rgb: (size, size, 3) の sRGB 0〜1。行 0 が画像の下（Blender の UV と同じ向き）。"""
    img = bpy.data.images.new(os.path.basename(path), width=size, height=size, alpha=False)
    rgba = np.ones((size, size, 4), dtype=np.float32)
    rgba[:, :, :3] = np.clip(pixels_rgb, 0.0, 1.0)
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)


# ---------------------------------------------------------------------------
# 地形の区画
# ---------------------------------------------------------------------------
def chunk_origin(settings, cx, cy):
    """区画番号（0 始まり）から、区画の南西の角の Blender 座標。区画全体の真ん中が原点になる。"""
    size = settings["chunkSize"]
    x0 = (cx - settings["chunksX"] / 2.0) * size
    y0 = (cy - settings["chunksY"] / 2.0) * size
    return x0, y0


def build_grid_mesh(name, terrain, x0, y0, size, step, material):
    n = int(round(size / step))
    xs = x0 + np.arange(n + 1) * step
    ys = y0 + np.arange(n + 1) * step
    gx, gy = np.meshgrid(xs, ys)  # gy が行
    gz = terrain.height(gx, gy)

    verts = np.stack([gx, gy, gz], axis=-1).reshape(-1, 3)
    faces = []
    for j in range(n):
        for i in range(n):
            a = j * (n + 1) + i
            faces.append((a, a + 1, a + n + 2, a + n + 1))
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts.tolist(), [], faces)
    me.update()
    uv = me.uv_layers.new(name="UVMap")
    # 頂点の UV は区画の中での位置（0〜1）。
    loop_vi = np.zeros(len(me.loops), dtype=np.int64)
    me.loops.foreach_get("vertex_index", loop_vi)
    u = (verts[loop_vi, 0] - x0) / size
    v = (verts[loop_vi, 1] - y0) / size
    uv.data.foreach_set("uv", np.stack([u, v], axis=-1).ravel().astype(np.float32))
    me.materials.append(material)
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def paint_ground(settings, terrain, x0, y0):
    size = settings["chunkSize"]
    res = settings["textureSize"]
    c = {k: np.array(v, dtype=np.float64) for k, v in settings["colors"].items()}
    seed = settings["seed"]
    px = x0 + (np.arange(res) + 0.5) / res * size
    py = y0 + (np.arange(res) + 0.5) / res * size
    gx, gy = np.meshgrid(px, py)

    slope = terrain.slope(gx, gy)
    pdist = terrain.path_distance(gx, gy)

    # 草：明るい・普通・暗いを、ゆるいノイズで混ぜる（手で塗ったようなむら）。
    n1 = fbm(gx / 14.0, gy / 14.0, 3, seed + 300) * 0.5 + 0.5
    n2 = fbm(gx / 4.0, gy / 4.0, 2, seed + 310) * 0.5 + 0.5
    w_light = smoothstep(0.45, 0.75, n1)[..., None]
    col = c["grassMid"] * (1 - w_light) + c["grassLight"] * w_light
    w_dark = (smoothstep(0.55, 0.8, n2) * 0.45)[..., None]
    col = col * (1 - w_dark) + c["grassDark"] * w_dark

    # 道（土）。
    w_path = (1.0 - smoothstep(terrain.path_width * 0.6, terrain.path_width, pdist))[..., None]
    dirt = c["dirt"] * (0.92 + 0.08 * n2[..., None])
    col = col * (1 - w_path) + dirt * w_path

    # 崖の斜面（岩肌）。上の方を明るく、下を暗くして段を描いたように見せる。
    w_rock = smoothstep(0.9, 1.4, slope)[..., None]
    bands = (np.sin(terrain.height(gx, gy) * 2.2 + n2 * 2.0) * 0.5 + 0.5)[..., None]
    rock = c["rockDark"] * (1 - bands) + c["rock"] * bands
    col = col * (1 - w_rock) + rock * w_rock

    # ごく細かいむら。
    fine = fbm(gx * 1.5, gy * 1.5, 1, seed + 320)[..., None] * 0.025
    return np.clip(col + fine, 0, 1)


def build_chunks(settings, terrain):
    os.makedirs(FIELD_DIR, exist_ok=True)
    chunks = []
    size = settings["chunkSize"]
    for cy in range(settings["chunksY"]):
        for cx in range(settings["chunksX"]):
            x0, y0 = chunk_origin(settings, cx, cy)
            name = "chunk_%d_%d" % (cx, cy)
            tex_path = os.path.join(FIELD_DIR, name + ".png")
            log("区画 %s: テクスチャを塗る" % name)
            save_png(paint_ground(settings, terrain, x0, y0), settings["textureSize"], tex_path)
            mat = make_image_material("Ground_" + name, tex_path)

            near = build_grid_mesh(name, terrain, x0, y0, size, settings["gridStep"], mat)
            export_fbx([near], os.path.join(FIELD_DIR, name + ".fbx"))
            far = build_grid_mesh(name + "_far", terrain, x0, y0, size, settings["farGridStep"], mat)
            export_fbx([far], os.path.join(FIELD_DIR, name + "_far.fbx"))
            far.hide_render = True

            ccx, ccy = x0 + size / 2, y0 + size / 2
            cz = float(terrain.height(np.array([ccx]), np.array([ccy]))[0])
            chunks.append({
                "x": cx, "y": cy,
                "model": GAME_FIELD_PATH + name + ".fbx",
                "far": GAME_FIELD_PATH + name + "_far.fbx",
                "center": to_game(ccx, ccy, cz),
                "tris": len(near.data.polygons) * 2,
            })
    return chunks


def to_game(x, y, z):
    return [round(x * M_TO_CM, 1), round(z * M_TO_CM, 1), round(-y * M_TO_CM, 1)]


# ---------------------------------------------------------------------------
# 部品（MegaKit）の書き出し
# ---------------------------------------------------------------------------
def material_image_paths(mat):
    paths = []
    if mat and mat.use_nodes:
        for nd in mat.node_tree.nodes:
            if nd.type == 'TEX_IMAGE' and nd.image:
                paths.append(nd.image.filepath)
    return paths


def base_color_of(mat):
    if mat and mat.use_nodes:
        bsdf = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        if bsdf:
            return list(bsdf.inputs["Base Color"].default_value)[:3]
    return [0.5, 0.5, 0.5]


_made_this_run = set()


def tint_image(src, color_srgb, dst):
    """白黒の画像に色を掛ける。アルファはそのまま。"""
    img = bpy.data.images.load(src)
    w, h = img.size
    px = np.zeros(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    px = px.reshape(-1, 4)
    px[:, :3] *= np.array(color_srgb, dtype=np.float32)
    out = bpy.data.images.new(os.path.basename(dst), width=w, height=h, alpha=True)
    out.pixels.foreach_set(px.ravel())
    out.filepath_raw = dst
    out.file_format = 'PNG'
    out.save()
    bpy.data.images.remove(out)
    bpy.data.images.remove(img)


def export_nature_model(model, settings):
    """MegaKit の FBX を読み、大きさをそろえて書き出す。戻り値は三角形の数と大きさ（m）。"""
    clear_scene()
    name = model["name"]
    bpy.ops.import_scene.fbx(filepath=os.path.join(MEGAKIT_FBX_DIR, name + ".fbx"))
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    deselect_all()
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    # 親（空のノード）があれば外して、変形をメッシュに焼き込む。
    if obj.parent:
        mw = obj.matrix_world.copy()
        obj.parent = None
        obj.matrix_world = mw
    s = model.get("baseScale", 1.0)
    obj.scale = [v * s for v in obj.scale]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    os.makedirs(NATURE_TEX_DIR, exist_ok=True)
    tints = model.get("tint", {})
    solids = model.get("solid", {})
    for slot in obj.material_slots:
        mat = slot.material
        if mat is None:
            continue
        base_name = mat.name.split(".")[0]
        # 白黒の画像に色を掛けて、新しいテクスチャにする（有料版のシェーダーが色を付けている葉など）。
        if base_name in tints:
            t = tints[base_name]
            png = os.path.join(NATURE_TEX_DIR, t["name"] + ".png")
            if not os.path.exists(png) or png not in _made_this_run:
                tint_image(os.path.join(MEGAKIT_TEX_DIR, t["image"]), t["color"], png)
                _made_this_run.add(png)
            slot.material = make_image_material(base_name, png)
            continue
        imgs = material_image_paths(mat)
        # テクスチャのないマテリアルは、色1色の小さな画像を作って貼る（エンジンが別の画像を拾わないように）。
        if not imgs or base_name in solids:
            png = os.path.join(NATURE_TEX_DIR, "Solid_%s_%s.png" % (name, base_name))
            if base_name in solids:
                srgb = np.array(solids[base_name], dtype=np.float64)
            else:
                col = np.array(base_color_of(mat), dtype=np.float64)
                srgb = np.where(col <= 0.0031308, col * 12.92, 1.055 * np.power(col, 1 / 2.4) - 0.055)
            save_png(np.tile(srgb, (4, 4, 1)), 4, png)
            slot.material = make_image_material(base_name, png)
            continue
        for p in imgs:
            src = bpy.path.abspath(p)
            if not os.path.exists(src):
                src = os.path.join(MEGAKIT_TEX_DIR, os.path.basename(p))
            if os.path.exists(src):
                shutil.copy2(src, os.path.join(NATURE_TEX_DIR, os.path.basename(src)))
            else:
                log("警告: テクスチャが見つからない %s (%s)" % (p, name))

    # 葉・草の面は1枚なので、裏返しの面を足して裏からも見えるようにする。
    foliage = set(model.get("foliage", []))
    if foliage:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        idx = {i for i, slot in enumerate(obj.material_slots) if slot.material and slot.material.name.split(".")[0] in foliage}
        src_faces = [f for f in bm.faces if f.material_index in idx]
        if src_faces:
            ret = bmesh.ops.duplicate(bm, geom=src_faces)
            new_faces = [g for g in ret["geom"] if isinstance(g, bmesh.types.BMFace)]
            bmesh.ops.reverse_faces(bm, faces=new_faces)
        bm.to_mesh(obj.data)
        bm.free()

    os.makedirs(NATURE_DIR, exist_ok=True)
    export_fbx([obj], os.path.join(NATURE_DIR, name + ".fbx"))
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    co = np.array([v.co[:] for v in obj.data.vertices])
    size = (co.max(axis=0) - co.min(axis=0)).tolist()
    return tris, size


# ---------------------------------------------------------------------------
# 置き場所を決める
# ---------------------------------------------------------------------------
class SpatialHash:
    def __init__(self, cell):
        self.cell = cell
        self.grid = {}

    def _key(self, x, y):
        return (int(math.floor(x / self.cell)), int(math.floor(y / self.cell)))

    def add(self, x, y, r):
        self.grid.setdefault(self._key(x, y), []).append((x, y, r))

    def near(self, x, y, r):
        """(x, y) から r と相手の半径を足した距離より近いものがあれば True。"""
        kx, ky = self._key(x, y)
        reach = int(math.ceil((r + 3.0) / self.cell))
        for i in range(kx - reach, kx + reach + 1):
            for j in range(ky - reach, ky + reach + 1):
                for (ox, oy, orad) in self.grid.get((i, j), ()):
                    if (ox - x) ** 2 + (oy - y) ** 2 < (r + orad) ** 2:
                        return True
        return False


def place_props(settings, terrain):
    rng = np.random.default_rng(settings["seed"])
    size = settings["chunkSize"]
    blockers = SpatialHash(4.0)  # 木・岩など、他の物を寄せ付けないもの
    placed = []  # (model, cx, cy, x, y, z, rot_deg, scale, tilt_x, tilt_y)
    for rule_no, rule in enumerate(settings["rules"]):
        own = SpatialHash(max(rule["minDist"], 0.5))
        for cy in range(settings["chunksY"]):
            for cx in range(settings["chunksX"]):
                x0, y0 = chunk_origin(settings, cx, cy)
                want = rule["count"]
                # 候補を多めに作り、条件で絞る。
                n = max(want * 12, 4000)  # 崖や道のように狭い場所でも候補が足りるように
                xs = x0 + rng.random(n) * size
                ys = y0 + rng.random(n) * size
                ok = np.ones(n, dtype=bool)
                ct = terrain.cliff_t(xs, ys)
                sl = terrain.slope(xs, ys)
                pd = terrain.path_distance(xs, ys)
                area = rule["area"]
                if area == "lowland":
                    ok &= ct < 0.05
                elif area == "plateau":
                    ok &= ct > 0.95
                elif area == "cliff":
                    ok &= (sl > 1.0) & (ct > 0.03) & (ct < 0.97)
                elif area == "path":
                    ok &= pd < terrain.path_width * 0.45
                if "slopeMax" in rule:
                    ok &= sl < rule["slopeMax"]
                if "pathAvoid" in rule:
                    ok &= pd > rule["pathAvoid"]
                if "spawnAvoid" in rule:
                    ok &= np.sqrt(xs * xs + ys * ys) > rule["spawnAvoid"]
                if rule.get("cluster", 0) > 0:
                    m = fbm(xs / 10.0, ys / 10.0, 2, settings["seed"] + 1000 + rule_no) * 0.5 + 0.5
                    ok &= m > rule["cluster"]
                got = 0
                for i in np.nonzero(ok)[0]:
                    if got >= want:
                        break
                    x, y = float(xs[i]), float(ys[i])
                    if own.near(x, y, rule["minDist"] * 0.5):
                        continue
                    if blockers.near(x, y, rule.get("block", 0.05)):
                        continue
                    own.add(x, y, rule["minDist"] * 0.5)
                    if rule.get("block", 0) > 0:
                        blockers.add(x, y, rule["block"])
                    model = rule["models"][int(rng.integers(len(rule["models"])))]
                    sc = float(rng.uniform(*rule["scale"]))
                    # 足もとの範囲でいちばん低い地面に置く（坂や崖で浮かないように）。
                    foot = rule.get("foot", 0.0) * sc
                    if foot > 0:
                        ang = np.linspace(0, 2 * math.pi, 8, endpoint=False)
                        fx = np.concatenate([[x], x + np.cos(ang) * foot, x + np.cos(ang) * foot * 0.5])
                        fy = np.concatenate([[y], y + np.sin(ang) * foot, y + np.sin(ang) * foot * 0.5])
                        ground = float(terrain.height(fx, fy).min())
                    else:
                        ground = float(terrain.height(np.array([x]), np.array([y]))[0])
                    z = ground - rule.get("sink", 0.0) * sc
                    rot = float(rng.uniform(0, 360))
                    tilt = rule.get("tilt", 0)
                    tx = float(rng.uniform(-tilt, tilt)) if tilt else 0.0
                    tz = float(rng.uniform(-tilt, tilt)) if tilt else 0.0
                    placed.append((model, cx, cy, x, y, z, rot, sc, tx, tz))
                    got += 1
                if got < want:
                    log("決まり %d (%s) 区画 %d,%d: %d / %d 個しか置けなかった" % (rule_no, rule["models"][0], cx, cy, got, want))
    return placed


# ---------------------------------------------------------------------------
# 配置 JSON
# ---------------------------------------------------------------------------
def write_props_json(settings, chunks, placed, model_info):
    kinds = settings["kinds"]
    models = []
    index = {}
    for m in settings["models"]:
        k = kinds[m["kind"]]
        index[m["name"]] = len(models)
        entry = {
            "name": m["name"],
            "path": GAME_NATURE_PATH + m["name"] + ".fbx",
            "kind": m["kind"],
            "collision": k["collision"],
            "shadow": k["shadow"],
            "drawDistance": round(k["drawDistance"] * M_TO_CM, 1),
            "tris": model_info[m["name"]][0],
        }
        if "trunkRadius" in m:
            entry["trunkRadius"] = round(m["trunkRadius"] * M_TO_CM, 1)
            entry["trunkHeight"] = round(min(model_info[m["name"]][1][2], 4.0) * M_TO_CM, 1)
        models.append(entry)

    # 区画ごと・部品ごとにまとめる。1つの置き方は [x, y, z, 向き（度）, 大きさ, 傾きX, 傾きZ]（ゲームの座標）。
    groups = {}
    for (model, cx, cy, x, y, z, rot, sc, tx, tz) in placed:
        groups.setdefault((cx, cy, index[model]), []).append(
            to_game(x, y, z) + [round(rot, 1), round(sc, 3), round(tx, 1), round(tz, 1)])
    props = [{"chunk": [cx, cy], "model": mi, "items": items} for (cx, cy, mi), items in sorted(groups.items())]

    data = {
        "_comment": "generate_stage.py が書き出したファイル。手で直さず、field_settings.json を変えて作り直す。座標はゲームの単位（cm、Y が上）。items は [x, y, z, 向き（Y 軸まわりの度）, 大きさ, 傾き X, 傾き Z]。",
        "version": 1,
        "chunkSize": round(settings["chunkSize"] * M_TO_CM, 1),
        "chunks": chunks,
        "models": models,
        "props": props,
    }
    os.makedirs(os.path.dirname(PROPS_JSON_PATH), exist_ok=True)
    with open(PROPS_JSON_PATH, "w", encoding="utf-8", newline="\n") as f:
        # 1つの置き方を1行にして、差分を見やすく・ファイルを小さくする。
        f.write("{\n")
        keys = list(data.keys())
        for ki, key in enumerate(keys):
            end = "," if ki < len(keys) - 1 else ""
            val = data[key]
            if key == "props":
                f.write('  "props": [\n')
                for pi, p in enumerate(val):
                    f.write('    { "chunk": %s, "model": %d, "items": [\n' % (json.dumps(p["chunk"]), p["model"]))
                    for ii, it in enumerate(p["items"]):
                        f.write("      " + json.dumps(it) + ("," if ii < len(p["items"]) - 1 else "") + "\n")
                    f.write("    ]}" + ("," if pi < len(val) - 1 else "") + "\n")
                f.write("  ]" + end + "\n")
            elif isinstance(val, list):
                f.write('  "%s": [\n' % key)
                for vi, v in enumerate(val):
                    f.write("    " + json.dumps(v, ensure_ascii=False) + ("," if vi < len(val) - 1 else "") + "\n")
                f.write("  ]" + end + "\n")
            else:
                f.write('  "%s": %s%s\n' % (key, json.dumps(val, ensure_ascii=False), end))
        f.write("}\n")


# ---------------------------------------------------------------------------
# 確かめ用の画像
# ---------------------------------------------------------------------------
def build_preview_scene(settings, terrain, placed, model_info):
    clear_scene()
    scene = bpy.context.scene
    # 地形（近い用）を読み戻す。
    for cy in range(settings["chunksY"]):
        for cx in range(settings["chunksX"]):
            import_fbx_in_meters(os.path.join(FIELD_DIR, "chunk_%d_%d.fbx" % (cx, cy)), settings["chunkSize"])
    # 部品は1つずつ読み、コレクションのインスタンスとして並べる（軽い）。
    proto = {}
    lib = bpy.data.collections.new("Prototypes")
    for m in settings["models"]:
        new = import_fbx_in_meters(os.path.join(NATURE_DIR, m["name"] + ".fbx"), max(model_info[m["name"]][1]))
        col = bpy.data.collections.new("P_" + m["name"])
        lib.children.link(col)
        for o in new:
            for c in o.users_collection:
                c.objects.unlink(o)
            col.objects.link(o)
        proto[m["name"]] = col
    for (model, cx, cy, x, y, z, rot, sc, tx, tz) in placed:
        inst = bpy.data.objects.new("I_" + model, None)
        inst.instance_type = 'COLLECTION'
        inst.instance_collection = proto[model]
        inst.location = (x, y, z)
        inst.rotation_euler = (math.radians(tx), math.radians(tz), math.radians(rot))
        inst.scale = (sc, sc, sc)
        scene.collection.objects.link(inst)

    # 空と太陽。
    world = bpy.data.worlds.new("Sky")
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    bg.inputs["Color"].default_value = (0.55, 0.75, 1.0, 1.0)
    bg.inputs["Strength"].default_value = 0.9
    scene.world = world
    sun_data = bpy.data.lights.new("Sun", 'SUN')
    sun_data.energy = 3.5
    sun = bpy.data.objects.new("Sun", sun_data)
    sun.rotation_euler = (math.radians(50), 0, math.radians(35))
    scene.collection.objects.link(sun)

    # 葉の透明な部分を抜く。
    for mat in bpy.data.materials:
        if not mat.use_nodes:
            continue
        nt = mat.node_tree
        bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        tex = next((n for n in nt.nodes if n.type == 'TEX_IMAGE'), None)
        if bsdf and tex and not bsdf.inputs["Alpha"].is_linked:
            nt.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
        if hasattr(mat, "surface_render_method"):
            mat.surface_render_method = 'DITHERED'

    scene.render.resolution_x = settings["preview"]["width"]
    scene.render.resolution_y = settings["preview"]["height"]
    scene.render.image_settings.file_format = 'PNG'
    try:
        scene.render.engine = 'BLENDER_EEVEE'
    except TypeError:
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.view_settings.view_transform = 'Standard'


def render_view(name, location, target, lens=24.0, ortho_scale=None):
    scene = bpy.context.scene
    cam_data = bpy.data.cameras.new(name)
    if ortho_scale:
        cam_data.type = 'ORTHO'
        cam_data.ortho_scale = ortho_scale
    else:
        cam_data.lens = lens
    cam_data.clip_end = 1000
    cam = bpy.data.objects.new(name, cam_data)
    scene.collection.objects.link(cam)
    cam.location = location
    d = np.array(target) - np.array(location)
    from mathutils import Vector
    cam.rotation_euler = Vector(d.tolist()).to_track_quat('-Z', 'Y').to_euler()
    scene.camera = cam
    scene.render.filepath = os.path.join(PREVIEW_DIR, name + ".png")
    try:
        bpy.ops.render.render(write_still=True)
    except RuntimeError as e:
        log("EEVEE で描けなかったので Workbench で描く: %s" % e)
        scene.render.engine = 'BLENDER_WORKBENCH'
        scene.display.shading.color_type = 'TEXTURE'
        bpy.ops.render.render(write_still=True)
    log("画像: " + scene.render.filepath)


# ---------------------------------------------------------------------------
def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    with open(os.path.join(SCRIPT_DIR, "field_settings.json"), encoding="utf-8") as f:
        settings = json.load(f)
    terrain = Terrain(settings)

    # 前に作ったものを消してから作る（使わなくなった部品やテクスチャを残さない）。
    for d in (FIELD_DIR, NATURE_DIR):
        if os.path.isdir(d):
            shutil.rmtree(d)

    log("部品を書き出す")
    model_info = {}
    for m in settings["models"]:
        model_info[m["name"]] = export_nature_model(m, settings)
        log("  %s: %d 三角形, 大きさ %s m" % (m["name"], model_info[m["name"]][0], [round(v, 2) for v in model_info[m["name"]][1]]))

    log("地形を作る")
    clear_scene()
    chunks = build_chunks(settings, terrain)

    log("部品を置く")
    placed = place_props(settings, terrain)
    write_props_json(settings, chunks, placed, model_info)
    counts = {}
    for p in placed:
        counts[p[0]] = counts.get(p[0], 0) + 1
    for k, v in counts.items():
        log("  %s: %d 個" % (k, v))
    log("配置 JSON: " + PROPS_JSON_PATH)

    if "--no-preview" not in argv:
        os.makedirs(PREVIEW_DIR, exist_ok=True)
        build_preview_scene(settings, terrain, placed, model_info)
        bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)
        half = settings["chunkSize"] * settings["chunksX"] / 2
        z0 = float(terrain.height(np.array([0.0]), np.array([-8.0]))[0])
        render_view("preview_ground", (0.0, -8.0, z0 + 1.7), (0.0, 30.0, z0 + 3.0), lens=20.0)
        render_view("preview_high", (-30.0, -55.0, 22.0), (5.0, 15.0, 2.0), lens=22.0)
        render_view("preview_top", (0.0, 0.0, 120.0), (0.0, 0.0, 0.0), ortho_scale=half * 2.0)
    log("おわり")


main()
