# アイテムのモデル（FBX）を斜め上から撮って、UI に出すアイコン（背景が透明な正方形の PNG）を作る。
#
# 使い方（Blender 5.1 で確認）:
#   blender -b --factory-startup --python make_item_icon.py -- <アイテムの .fbx> <出力の .png> [大きさ（ピクセル、既定 256）]
#
# - FBX と同じフォルダにある色の画像（Meshy なら名前の後ろに何も付かない PNG）を、そのまま色として貼る。
# - カメラは正投影で、モデル全体が少し余白を残して収まるように合わせる。
# - できた PNG は texconv で DDS にしてゲームで使う（Assets/sprite/item/）。
import bpy
import math
import os
import sys

from mathutils import Vector


def main():
    argv = sys.argv[sys.argv.index("--") + 1:]
    fbx_path = os.path.abspath(argv[0])
    out_path = os.path.abspath(argv[1])
    size = int(argv[2]) if len(argv) > 2 else 256

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=fbx_path)
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']

    # 色の画像を貼り直す（FBX の中のパスが別のPCを指していても、隣にある PNG を使う）。
    base = os.path.splitext(os.path.basename(fbx_path))[0]
    folder = os.path.dirname(fbx_path)
    color_png = None
    for name in os.listdir(folder):
        stem, ext = os.path.splitext(name)
        if ext.lower() == ".png" and not stem.endswith(("_metallic", "_roughness", "_normal")):
            color_png = os.path.join(folder, name)
            break
    if color_png is not None:
        image = bpy.data.images.load(color_png)
        for obj in meshes:
            for slot in obj.material_slots:
                mat = slot.material
                mat.use_nodes = True
                nodes = mat.node_tree.nodes
                bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
                tex = nodes.new("ShaderNodeTexImage")
                tex.image = image
                mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
                bsdf.inputs["Metallic"].default_value = 0.0
                bsdf.inputs["Roughness"].default_value = 0.4

    # モデル全体の箱。
    corners = [obj.matrix_world @ Vector(c) for obj in meshes for c in obj.bound_box]
    lo = Vector((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)))
    hi = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
    center = (lo + hi) * 0.5
    extent = max(hi - lo)

    scene = bpy.context.scene

    # カメラ: 正面から少し右・少し上（Blender は Z が上、前は -Y）。
    cam_data = bpy.data.cameras.new("IconCamera")
    cam_data.type = 'ORTHO'
    cam_data.ortho_scale = extent * 1.15
    cam = bpy.data.objects.new("IconCamera", cam_data)
    scene.collection.objects.link(cam)
    direction = Vector((math.sin(math.radians(20.0)), -1.0, math.tan(math.radians(15.0)))).normalized()
    cam.location = center + direction * extent * 3.0
    cam.rotation_euler = (center - cam.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera = cam

    # 光: 斜め前上からの太陽と、明るめの環境光。
    sun_data = bpy.data.lights.new("IconSun", 'SUN')
    sun_data.energy = 3.0
    sun = bpy.data.objects.new("IconSun", sun_data)
    scene.collection.objects.link(sun)
    sun.rotation_euler = (math.radians(50.0), 0.0, math.radians(30.0))
    world = bpy.data.worlds.new("IconWorld")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (1.0, 1.0, 1.0, 1.0)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.8
    scene.world = world

    scene.render.engine = 'BLENDER_EEVEE'
    scene.render.film_transparent = True
    scene.render.resolution_x = size
    scene.render.resolution_y = size
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.view_settings.view_transform = 'Standard'
    scene.render.filepath = out_path
    bpy.ops.render.render(write_still=True)
    print("wrote", out_path)


main()
