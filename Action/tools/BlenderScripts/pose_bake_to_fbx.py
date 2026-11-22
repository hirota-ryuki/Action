# ゲームで焼き込んだ姿勢（PoseBaker が書く JSON）を、モデルの骨格のアニメーションにして FBX に書き出す。
#
# 使い方（Blender 5.1・5.2 で確認）:
#   blender -b --python pose_bake_to_fbx.py -- <骨格の FBX> <出力フォルダ> <JSON> [<JSON> ...]
#
#   骨格の FBX   クリップを付けるモデル（プレイヤーなら Game/Assets/modelData/player/Model/SK_Player.FBX）
#   出力フォルダ <出力フォルダ>/<JSON の clip>.fbx に書く
#   JSON         ゲームの PoseBaker が書いたもの。骨ごとに、バインドポーズ（bind）と、コマごとの
#                モデルの根元から見た骨の行列（frames）が入っている。行列はゲームの書き方（行ベクトル、4x4 を行の順に 16 個）
#
# - ゲームの座標（モデルの根元から見た座標）と Blender のアーマチュアの座標の対応は、
#   骨のバインドポーズの位置（ゲーム）と、骨の頭の位置（Blender）から、最小二乗で求める（向き・大きさ・位置）。
#   ゲームの FBX の読み込み方（軸の向き・単位）を知らなくても合う。合わない骨があれば、ずれを出す。
# - 骨ごとに「バインドポーズから今の姿勢への動き」（ゲームでスキンに掛ける行列）を、Blender の座標に直して、
#   その骨のポーズにする。位置・回転・大きさを全部キーにする（腕ごと前へ出す、などの位置の動きも残す）。
# - 書き出しは retarget_black_knight.py と同じ設定（ゲームがそのまま読める形）。
import bpy
import json
import os
import sys

import numpy as np
from mathutils import Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
TARGET_FBX = os.path.abspath(argv[0])
OUT_DIR = os.path.abspath(argv[1])
JSON_PATHS = [os.path.abspath(p) for p in argv[2:]]
FPS = 30

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=TARGET_FBX)
TGT = next(o for o in bpy.data.objects if o.type == "ARMATURE")
SCENE = bpy.context.scene
SCENE.render.fps = FPS
SCENE.render.fps_base = 1.0


def game_matrix(values):
    """ゲームの行列（行ベクトル、16 個）を、Blender の行列（列ベクトル）にする。"""
    rows = [values[0:4], values[4:8], values[8:12], values[12:16]]
    return Matrix(rows).transposed()


def fit_game_to_armature(bones):
    """ゲームのモデルの座標 → アーマチュアの座標 の行列（4x4）を、骨の位置から最小二乗で求める。"""
    src, dst = [], []
    for b in bones:
        bone = TGT.data.bones.get(b["name"])
        if bone is None:
            continue
        t = b["bind"][12:15]
        src.append([t[0], t[1], t[2], 1.0])
        dst.append(list(bone.head_local))
    a = np.array(src)
    x, *_ = np.linalg.lstsq(a, np.array(dst), rcond=None)
    err = np.abs(a @ x - np.array(dst)).max()
    m = Matrix.Identity(4)
    for r in range(3):
        for c in range(4):
            m[r][c] = x[c][r]
    print(f"FIT 骨 {len(src)} 本、位置のずれのいちばん大きいもの {err:.6f}（アーマチュアの単位）")
    return m


def bake(path):
    with open(path, encoding="utf-8") as fp:
        data = json.load(fp)
    name = data["clip"]
    bones = data["bones"]
    frames = data["frames"]
    to_arm = fit_game_to_armature(bones)
    to_game = to_arm.inverted()
    index = {b["name"]: i for i, b in enumerate(bones)}
    bind_inv = [game_matrix(b["bind"]).inverted() for b in bones]

    # 親から先に並べる（ポーズの基本の行列を、親の今の姿勢から求めるため）。
    order = []

    def visit(bone):
        order.append(bone)
        for child in bone.children:
            visit(child)

    for bone in TGT.data.bones:
        if bone.parent is None:
            visit(bone)

    TGT.animation_data_create()
    act = bpy.data.actions.new(name)
    TGT.animation_data.action = act
    for pb in TGT.pose.bones:
        pb.rotation_mode = "QUATERNION"

    max_err = 0.0
    prev_q = {}
    for f, pose in enumerate(frames):
        posed = {}
        for bone in order:
            rest = bone.matrix_local
            i = index.get(bone.name)
            if i is not None:
                # ゲームでスキンに掛ける動き（バインドポーズ → 今の姿勢）を、アーマチュアの座標に直す。
                deform = to_arm @ game_matrix(pose[i]) @ bind_inv[i] @ to_game
                target = deform @ rest
            elif bone.parent is not None:
                target = posed[bone.parent.name] @ bone.parent.matrix_local.inverted() @ rest
            else:
                target = rest.copy()
            posed[bone.name] = target
            if bone.parent is not None:
                parent_rest = bone.parent.matrix_local
                basis = (parent_rest.inverted() @ rest).inverted() @ posed[bone.parent.name].inverted() @ target
            else:
                basis = rest.inverted() @ target
            loc, rot, scale = basis.decompose()
            q0 = prev_q.get(bone.name)
            if q0 is not None and q0.dot(rot) < 0.0:
                rot = -rot
            prev_q[bone.name] = rot
            pb = TGT.pose.bones[bone.name]
            pb.location = loc
            pb.rotation_quaternion = rot
            pb.scale = scale
            pb.keyframe_insert("location", frame=f)
            pb.keyframe_insert("rotation_quaternion", frame=f)
            pb.keyframe_insert("scale", frame=f)
        # 確かめ: キーを打った姿勢の骨の頭の位置が、ゲームの姿勢と合っているか。
        SCENE.frame_set(f)
        for bone in order:
            if bone.name in index:
                got = TGT.pose.bones[bone.name].matrix.translation
                want = posed[bone.name].translation
                max_err = max(max_err, (got - want).length)
    act.use_fake_user = True
    SCENE.frame_start, SCENE.frame_end = 0, len(frames) - 1
    print(f"[{name}] {len(frames)} コマ。キーを打った姿勢とゲームの姿勢の、骨の位置のずれのいちばん大きいもの {max_err:.6f}")
    return name


def export(path):
    for o in bpy.data.objects:
        o.select_set(o == TGT)
    bpy.context.view_layer.objects.active = TGT
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={"ARMATURE"},
        axis_forward="Y", axis_up="Z", add_leaf_bones=False,
        primary_bone_axis="Y", secondary_bone_axis="X",
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", use_armature_deform_only=False,
        bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
        bake_anim_force_startend_keying=True, bake_anim_step=1.0, bake_anim_simplify_factor=0.0,
        path_mode="STRIP")
    print("EXPORTED", path)


os.makedirs(OUT_DIR, exist_ok=True)
for json_path in JSON_PATHS:
    clip = bake(json_path)
    export(os.path.join(OUT_DIR, clip + ".fbx"))
