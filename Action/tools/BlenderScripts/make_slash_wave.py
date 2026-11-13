# 黒騎士が飛ばす斬撃（三日月形の薄い板）のモデルと色の画像を作る。
#
# 使い方（Blender 5.2 で確認）:
#   blender -b --python make_slash_wave.py -- <出力フォルダ>
#
# 書き出すもの
#   <出力フォルダ>/SlashWave.fbx   三日月形の板。水平に寝ていて、ふくらんだ側が前（-Y。ゲームでは +Z）
#   <出力フォルダ>/SlashWave.png   色。ふくらんだ縁がいちばん明るい暗めの赤で、内側と両端へ向けて暗くなる
#
# ゲームは FBX を読むとき、FBX にテクスチャの指定がなければ、同じフォルダの画像を使う。
# 斜めの斬撃にするのは、ゲームの側で進む向きのまわりに回して置く（SlashBullet）。
import bpy
import sys
import os
import math

argv = sys.argv[sys.argv.index("--") + 1:]
OUT_DIR = os.path.abspath(argv[0])

WIDTH = 1.6          # 三日月の端から端まで（m）
DEPTH = 0.45         # 真ん中のふくらみ（m）
THICK_MID = 0.22     # 真ん中の太さ（m）。両端へ向けて細くなる
PLATE = 0.02         # 板の厚さ（m）
SEGMENTS = 32

# 色（0〜255）。縁 → 内側、端は内側の色へ寄せる
EDGE = (194, 52, 40)
BASE = (163, 32, 28)       # #a3201c
INNER = (90, 15, 14)

bpy.ops.wm.read_factory_settings(use_empty=True)

# 外側の縁（ふくらんだ側）と内側の縁を、両端で1点に集まるように作る
verts, faces, uvs = [], [], []
for i in range(SEGMENTS + 1):
    s = i / SEGMENTS
    x = (s - 0.5) * WIDTH
    bulge = 1.0 - (2 * s - 1) ** 2               # 端で 0、真ん中で 1
    y_outer = -DEPTH * bulge                     # 前（-Y）へふくらむ
    y_inner = y_outer + THICK_MID * math.sqrt(max(bulge, 0.0))
    verts.append((x, y_outer + DEPTH * 0.5, 0.0))
    verts.append((x, y_inner + DEPTH * 0.5, 0.0))
for i in range(SEGMENTS):
    a, b, c, d = 2 * i, 2 * i + 1, 2 * i + 3, 2 * i + 2
    faces.append((a, d, c, b))

mesh = bpy.data.meshes.new("SlashWave")
mesh.from_pydata(verts, [], faces)
uv = mesh.uv_layers.new(name="UVMap")
for poly in mesh.polygons:
    for li in poly.loop_indices:
        vi = mesh.loops[li].vertex_index
        uv.data[li].uv = ((vi // 2) / SEGMENTS, 0.0 if vi % 2 == 0 else 1.0)
obj = bpy.data.objects.new("SlashWave", mesh)
bpy.context.scene.collection.objects.link(obj)

# 厚みを付けて閉じた形にする（裏から見ても消えないように）
sol = obj.modifiers.new("Solidify", "SOLIDIFY")
sol.thickness = PLATE
sol.offset = 0.0
bpy.context.view_layer.objects.active = obj
obj.select_set(True)
bpy.ops.object.modifier_apply(modifier="Solidify")

# 色の画像
W, H = 64, 16
img = bpy.data.images.new("SlashWave", W, H)
px = []
for yy in range(H):
    v = yy / (H - 1)                      # 0 = 外側の縁、1 = 内側
    for xx in range(W):
        u = xx / (W - 1)
        tip = abs(2 * u - 1) ** 3         # 端ほど 1
        if v < 0.35:
            c = [EDGE[k] + (BASE[k] - EDGE[k]) * (v / 0.35) for k in range(3)]
        else:
            c = [BASE[k] + (INNER[k] - BASE[k]) * ((v - 0.35) / 0.65) for k in range(3)]
        c = [c[k] + (INNER[k] - c[k]) * tip for k in range(3)]
        px += [c[0] / 255, c[1] / 255, c[2] / 255, 1.0]
img.pixels = px
os.makedirs(OUT_DIR, exist_ok=True)
img.filepath_raw = os.path.join(OUT_DIR, "SlashWave.png")
img.file_format = "PNG"
img.save()

mat = bpy.data.materials.new("SlashWave")
mesh.materials.append(mat)

bpy.ops.export_scene.fbx(
    filepath=os.path.join(OUT_DIR, "SlashWave.fbx"), use_selection=True, object_types={"MESH"},
    axis_forward="Y", axis_up="Z", apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
    bake_anim=False, path_mode="STRIP")
print("EXPORTED", OUT_DIR)
