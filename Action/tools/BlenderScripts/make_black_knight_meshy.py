# 黒騎士（敵）のモデルを、Meshy で作ったメッシュに骨を入れて FBX に書き出す。
#
# 使い方（Blender 5.1 で確認）:
#   blender -b --python make_black_knight_meshy.py -- <PoseRig.fbx> <Meshy の .fbx> <出力フォルダ> [--preview <フォルダ>]
#
#   PoseRig.fbx   BlackKnightSource/PoseRig.fbx。SK_Player の骨格に揺れ用の骨（J_Sec_Hood・J_Sec_Cloth*）と
#                 剣の骨（J_Sec_Sword）を足したものと、剣のメッシュ（前の自作の黒騎士の剣）が入っている。
#   Meshy の .fbx  Meshy で作った体のメッシュ（T ポーズ、手は空）。同じフォルダに、名前の後ろに
#                 なし・_metallic・_roughness を付けた3枚の PNG（色・メタリック・ラフネス）があること。
#   --preview     正面・斜め・背面と、骨を曲げた確かめ用のポーズを <フォルダ>/meshy_*.png に書く。
#
# - Meshy のメッシュは身長 1.621 m（SK_Player と同じ）に縮め、足の裏を z=0、左右の中心を x=0 にそろえる。前は -Y。
# - 骨は、関節の位置を Meshy の体に合わせて動かす（JOINTS）。腕・脚・背骨は、骨の向きも次の関節へ向け直す。
#   モーション（retarget_black_knight.py）は、既存のモーションを、骨の世界での向きがそろうようにこの骨格へ移す。
# - 重みは Blender の自動の重み付け（ボーンヒート）で付け、手・頭・剣を固く付け直す。
#   VRoid の揺れ用の骨（髪・スカート・胸）とフードの骨・指の骨は、重みを付けない（親の骨に付いて動くだけ）。
# - マテリアルは2つ。BlackKnight（Meshy の色・メタリック・ラフネスから albedo と spec を作る）と、
#   BlackKnightSword（剣。BlackKnightSource の画像を写す）。spec は r=メタリック、a=なめらかさ（1-ラフネス）。
# - ゲームのエンジンには発光がない。紫の光は Meshy の色の画像に描かれているので、そのまま色で代わりにする。
#   プレビュー用に、色の画像の紫のところを発光のマスク（BlackKnight_emission.png）として書く（ゲームは使わない）。
import bpy
import math
import os
import shutil
import sys

import numpy as np
from mathutils import Matrix, Vector

argv = sys.argv[sys.argv.index("--") + 1:]
RIG_FBX, MESHY_FBX, OUT_DIR = (os.path.abspath(p) for p in argv[:3])
PREVIEW_DIR = os.path.abspath(argv[argv.index("--preview") + 1]) if "--preview" in argv else None
OUT_NAME = "BlackKnight"
HEIGHT = 1.621

# ---------------------------------------------------------------------------
# Meshy の体に合わせた関節の位置（ワールド、m）。左（+X）だけ書き、右は x を反転する。
# 目盛りを付けて描いた正面・側面の画像から読み取った値。
# ---------------------------------------------------------------------------
JOINTS_C = {
    "Hips": (0, -.060, .950),
    "Spine": (0, -.065, 1.010),
    "Chest": (0, -.070, 1.110),
    "UpperChest": (0, -.065, 1.210),
    "Neck": (0, -.050, 1.370),
    "Head": (0, -.050, 1.430),
}
JOINTS_L = {
    "Shoulder": (.025, -.050, 1.330),
    "UpperArm": (.140, -.040, 1.300),
    "LowerArm": (.390, -.030, 1.275),
    "Hand": (.620, -.030, 1.250),
    "UpperLeg": (.095, -.060, .900),
    "LowerLeg": (.145, -.040, .480),
    "Foot": (.170, -.020, .130),
    "ToeBase": (.175, -.120, .040),
}
# 揺れ用の骨。ClothF は前の長い垂れ布、ClothL・ClothR はマントの左右の半分（腰から裾まで）
CLOTH = {
    "F": [(0, -.170, .980), (0, -.170, .730), (0, -.170, .490), (0, -.170, .250)],
    "L": [(.130, .080, .980), (.140, .110, .720), (.150, .140, .460), (.160, .170, .200)],
}
CLOTH["R"] = [(-x, y, z) for x, y, z in CLOTH["L"]]
FINGER_SCALE = 1.15   # Meshy の手は SK_Player より大きいので、指の骨の間隔を広げる

# 骨の向けたい先（骨の名前 -> 向ける先の骨の名前）
AIM = {
    "J_Bip_C_Hips": "J_Bip_C_Spine", "J_Bip_C_Spine": "J_Bip_C_Chest", "J_Bip_C_Chest": "J_Bip_C_UpperChest",
    "J_Bip_C_UpperChest": "J_Bip_C_Neck", "J_Bip_C_Neck": "J_Bip_C_Head",
}
for s in "LR":
    AIM.update({f"J_Bip_{s}_Shoulder": f"J_Bip_{s}_UpperArm", f"J_Bip_{s}_UpperArm": f"J_Bip_{s}_LowerArm",
                f"J_Bip_{s}_LowerArm": f"J_Bip_{s}_Hand", f"J_Bip_{s}_UpperLeg": f"J_Bip_{s}_LowerLeg",
                f"J_Bip_{s}_LowerLeg": f"J_Bip_{s}_Foot", f"J_Bip_{s}_Foot": f"J_Bip_{s}_ToeBase"})


def targets():
    t = {f"J_Bip_C_{k}": Vector(v) for k, v in JOINTS_C.items()}
    for k, (x, y, z) in JOINTS_L.items():
        t[f"J_Bip_L_{k}"] = Vector((x, y, z))
        t[f"J_Bip_R_{k}"] = Vector((-x, y, z))
    ends = {}
    for key, pts in CLOTH.items():
        for k in range(3):
            n = f"J_Sec_Cloth{key}_{k + 1}"
            t[n] = Vector(pts[k])
            ends[n] = Vector(pts[k + 1])
    return t, ends


def is_finger(name):
    return any(k in name for k in ("Thumb", "Index", "Middle", "Ring", "Little"))


# 重みを付ける骨
def deforms(name):
    if name.startswith("J_Bip_"):
        return not is_finger(name)
    return name.startswith("J_Sec_Cloth")


# ---------------------------------------------------------------------------
# 骨格を読み込み、関節を Meshy の体に合わせる
# ---------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=RIG_FBX)
ARM = next(o for o in bpy.data.objects if o.type == "ARMATURE")
SWORD = next(o for o in bpy.data.objects if o.type == "MESH")
INV = ARM.matrix_world.inverted()
OLD_SWORD_HEAD = ARM.matrix_world @ ARM.data.bones["J_Sec_Sword"].head_local


def refit():
    tgt, ends = targets()
    ARM.select_set(True)
    bpy.context.view_layer.objects.active = ARM
    bpy.ops.object.mode_set(mode="EDIT")
    eb = ARM.data.edit_bones
    old = {b.name: (b.head.copy(), b.tail.copy(), b.z_axis.copy()) for b in eb}
    for b in eb:
        b.use_connect = False

    def depth(b):
        d = 0
        while b.parent:
            b, d = b.parent, d + 1
        return d
    order = sorted([b.name for b in eb], key=lambda n: depth(eb[n]))
    newh, rot = {}, {}
    for name in order:
        b = eb[name]
        p = b.parent.name if b.parent else None
        oh, ot, oz = old[name]
        if name in tgt:
            nh = INV @ tgt[name]
        elif p:
            sc = FINGER_SCALE if is_finger(name) else 1.0
            nh = newh[p] + rot[p] @ ((oh - old[p][0]) * sc)
        else:
            nh = oh.copy()
        od = ot - oh
        aim_to = None
        if name in AIM:
            aim_to = INV @ tgt[AIM[name]]
        elif name in ends:
            aim_to = INV @ ends[name]
        if aim_to is not None:
            R = od.normalized().rotation_difference((aim_to - nh).normalized()).to_matrix()
            tail = nh + (aim_to - nh)
        else:
            # 指・つま先・手は親と同じだけ回す。それ以外（剣・フード・VRoid の揺れ用の骨）は向きを変えない
            inherit = is_finger(name) or name.endswith(("_Hand", "_ToeBase"))
            R = rot[p] if (inherit and p) else Matrix.Identity(3)
            tail = nh + R @ od
        b.head = nh
        b.tail = tail
        b.align_roll(R @ oz)
        newh[name], rot[name] = nh, R
    bpy.ops.object.mode_set(mode="OBJECT")


refit()
NEW_SWORD_HEAD = ARM.matrix_world @ ARM.data.bones["J_Sec_Sword"].head_local
for b in ARM.data.bones:
    b.use_deform = deforms(b.name)

# 剣のメッシュを、剣の骨が動いた分だけ動かす（向きは変えない）
delta = SWORD.matrix_world.inverted().to_3x3() @ (NEW_SWORD_HEAD - OLD_SWORD_HEAD)
for v in SWORD.data.vertices:
    v.co += delta

# ---------------------------------------------------------------------------
# Meshy のメッシュ
# ---------------------------------------------------------------------------
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=MESHY_FBX)
new_objs = [o for o in bpy.data.objects if o not in before]
BODY = next(o for o in new_objs if o.type == "MESH")
for o in new_objs:
    if o is not BODY:
        bpy.data.objects.remove(o, do_unlink=True)
BODY.parent = None
mw = BODY.matrix_world.copy()
ws = [mw @ v.co for v in BODY.data.vertices]
zmin = min(p.z for p in ws)
zmax = max(p.z for p in ws)
cx = (min(p.x for p in ws) + max(p.x for p in ws)) / 2
SC = HEIGHT / (zmax - zmin)
BODY.matrix_world = Matrix.Identity(4)
for v, p in zip(BODY.data.vertices, ws):
    v.co = Vector(((p.x - cx) * SC, p.y * SC, (p.z - zmin) * SC))
BODY.data.update()
BODY.name = BODY.data.name = OUT_NAME
print("BODY tris", sum(len(p.vertices) - 2 for p in BODY.data.polygons), "scale", round(SC, 4))

# 自動の重み付け（ボーンヒート）
for o in bpy.data.objects:
    o.select_set(o in (BODY, ARM))
bpy.context.view_layer.objects.active = ARM
bpy.ops.object.parent_set(type="ARMATURE_AUTO")
print("weights:", len(BODY.vertex_groups), "groups")


def set_weights(vi, ws_):
    for vg in BODY.vertex_groups:
        vg.remove([vi])
    for n, w in ws_.items():
        if w > 1e-4:
            g = BODY.vertex_groups.get(n) or BODY.vertex_groups.new(name=n)
            g.add([vi], w, "REPLACE")


def smooth01(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


# 手は手の骨に固く付ける（指は曲げない）。手首の前後 3cm で前腕となめらかにつなぐ
hand_x = JOINTS_L["Hand"][0]
head_z = JOINTS_C["Head"][2]
for v in BODY.data.vertices:
    p = v.co
    if abs(p.x) > hand_x - 0.03 and p.z > 1.10:
        s = "L" if p.x > 0 else "R"
        t = smooth01(hand_x - 0.03, hand_x + 0.01, abs(p.x))
        set_weights(v.index, {f"J_Bip_{s}_Hand": t, f"J_Bip_{s}_LowerArm": 1 - t})
    elif p.z > head_z + 0.01 and abs(p.x) < 0.13:
        set_weights(v.index, {"J_Bip_C_Head": 1.0})

# 1つの頂点に付ける骨を4本までにして、合計を1にする
bpy.ops.object.select_all(action="DESELECT")
BODY.select_set(True)
bpy.context.view_layer.objects.active = BODY
bpy.ops.object.vertex_group_limit_total(group_select_mode="ALL", limit=4)
bpy.ops.object.vertex_group_normalize_all(group_select_mode="ALL", lock_active=False)

# ---------------------------------------------------------------------------
# テクスチャとマテリアル
# ---------------------------------------------------------------------------
os.makedirs(OUT_DIR, exist_ok=True)
stem = os.path.splitext(MESHY_FBX)[0]


def load(path, colorspace):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = colorspace
    return im


def px(im):
    a = np.empty(im.size[0] * im.size[1] * 4, dtype=np.float32)
    im.pixels.foreach_get(a)
    return a.reshape(im.size[1], im.size[0], 4)


def save(arr, name, colorspace):
    h, w = arr.shape[:2]
    im = bpy.data.images.new(f"{OUT_NAME}_{name}", w, h, alpha=True)
    im.colorspace_settings.name = colorspace
    im.pixels.foreach_set(arr.astype(np.float32).ravel())
    im.filepath_raw = os.path.join(OUT_DIR, f"{OUT_NAME}_{name}.png")
    im.file_format = "PNG"
    im.save()
    return im


albedo_path = os.path.join(OUT_DIR, f"{OUT_NAME}_albedo.png")
shutil.copyfile(stem + ".png", albedo_path)
alb = load(albedo_path, "sRGB")
met = px(load(stem + "_metallic.png", "Non-Color"))
rough = px(load(stem + "_roughness.png", "Non-Color"))
spec = np.zeros_like(met)
spec[..., 0] = met[..., 0]
spec[..., 3] = 1.0 - rough[..., 0]
spec_im = save(spec, "spec", "Non-Color")
# 発光のマスク（プレビュー用）: 色の画像で、青が赤・緑よりはっきり強いところ（紫の線）
a = px(load(stem + ".png", "Non-Color"))
r_, g_, b_ = a[..., 0], a[..., 1], a[..., 2]
mask = np.clip((b_ - np.maximum(r_ * 1.15, g_ * 1.45) - 0.06) * 6.0, 0, 1) * (b_ > 0.30)
emi = np.zeros_like(a)
emi[..., 0] = emi[..., 1] = emi[..., 2] = mask
emi[..., 3] = 1.0
save(emi, "emission", "Non-Color")
print("glow mask coverage", round(float(mask.mean()), 4))

mat = bpy.data.materials.new(OUT_NAME)
mat.use_nodes = True
nt = mat.node_tree
bsdf = nt.nodes["Principled BSDF"]
ta = nt.nodes.new("ShaderNodeTexImage")
ta.image = alb
nt.links.new(ta.outputs["Color"], bsdf.inputs["Base Color"])
ts = nt.nodes.new("ShaderNodeTexImage")
ts.image = spec_im
nt.links.new(ts.outputs["Color"], bsdf.inputs["Specular IOR Level"])
BODY.data.materials.clear()
BODY.data.materials.append(mat)

# 剣のマテリアルの画像を写す
src_dir = os.path.dirname(RIG_FBX)
for kind in ("albedo", "spec", "normal"):
    shutil.copyfile(os.path.join(src_dir, f"BlackKnightSword_{kind}.png"),
                    os.path.join(OUT_DIR, f"BlackKnightSword_{kind}.png"))
shutil.copyfile(os.path.join(src_dir, "BlackKnightSword_emission.png"),
                os.path.join(OUT_DIR, "BlackKnightSword_emission.png"))
smat = SWORD.data.materials[0]
for n in smat.node_tree.nodes:
    if n.type == "TEX_IMAGE" and n.image:
        n.image.filepath = os.path.join(OUT_DIR, os.path.basename(n.image.filepath))
        n.image.reload()

# 剣を体に合わせて1つのメッシュにする
for o in bpy.data.objects:
    o.select_set(o in (BODY, SWORD))
bpy.context.view_layer.objects.active = BODY
bpy.ops.object.join()
print("materials", [m.name for m in BODY.data.materials])
print("total tris", sum(len(p.vertices) - 2 for p in BODY.data.polygons))

out_fbx = os.path.join(OUT_DIR, f"{OUT_NAME}.fbx")
for o in bpy.data.objects:
    o.select_set(o in (ARM, BODY))
bpy.ops.export_scene.fbx(filepath=out_fbx, axis_forward="Y", axis_up="Z", add_leaf_bones=False,
                         primary_bone_axis="Y", secondary_bone_axis="X", bake_anim=False,
                         object_types={"ARMATURE", "MESH"}, apply_unit_scale=True,
                         apply_scale_options="FBX_SCALE_NONE", use_armature_deform_only=False,
                         path_mode="STRIP", use_mesh_modifiers=False)
print("EXPORTED", out_fbx)


# ---------------------------------------------------------------------------
# プレビュー
# ---------------------------------------------------------------------------
def render_previews():
    sc = bpy.context.scene
    try:
        sc.render.engine = "BLENDER_EEVEE"
    except TypeError:
        sc.render.engine = "BLENDER_EEVEE_NEXT"
    sc.render.resolution_x, sc.render.resolution_y = 600, 900
    sc.view_settings.view_transform = "Standard"
    w = bpy.data.worlds.new("w")
    sc.world = w
    w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.62, 0.64, 0.68, 1)
    w.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(48), 0, math.radians(-35))
    sc.collection.objects.link(sun)
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    sc.collection.objects.link(cam)
    sc.camera = cam
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 2.0
    tgt = Vector((0, 0, 0.85))

    def shot(name, ang):
        a = math.radians(ang)
        cam.location = tgt + Vector((math.sin(a) * 6, -math.cos(a) * 6, 0.4))
        cam.rotation_euler = (tgt - cam.location).to_track_quat("-Z", "Y").to_euler()
        sc.render.filepath = os.path.join(PREVIEW_DIR, f"meshy_{name}.png")
        bpy.ops.render.render(write_still=True)

    for name, ang in (("front", 0), ("diag", 35), ("back", 180)):
        shot(name, ang)
    # 確かめ用のポーズ: 左の太ももを上げて膝を曲げ、右腕を前へ、胴をひねる
    pb = ARM.pose.bones

    def rot(n, axis, deg):
        b = pb[n]
        b.rotation_mode = "XYZ"
        r = list(b.rotation_euler)
        r["XYZ".index(axis)] += math.radians(deg)
        b.rotation_euler = r
    rot("J_Bip_L_UpperLeg", "X", -70)
    rot("J_Bip_L_LowerLeg", "X", 90)
    rot("J_Bip_R_UpperLeg", "X", 25)
    rot("J_Bip_R_UpperArm", "Z", 60)
    rot("J_Bip_R_LowerArm", "Z", 60)
    rot("J_Bip_L_UpperArm", "Z", -70)
    rot("J_Bip_C_Spine", "Y", 25)
    rot("J_Bip_C_Head", "Y", -30)
    rot("J_Sec_ClothL_1", "X", 30)
    rot("J_Sec_ClothR_1", "X", 30)
    bpy.context.view_layer.update()
    for name, ang in (("pose_front", 0), ("pose_diag", 50), ("pose_side", 90)):
        shot(name, ang)


if PREVIEW_DIR:
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    render_previews()
print("done")
