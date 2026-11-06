"""UE 5.8 コマンドライン用: SK_Player 用のアニメーション FBX の、指定したボーンを前後に傾けて書き出し直す。

リターゲットの結果の姿勢を少しだけ直したいとき（上半身がのけぞっている、など）に使う。
元のモーション（Mixamo など）や、リターゲットに使った UE のプロジェクトがなくても、
リポジトリにある SK_Player.FBX とアニメーションの FBX だけで動く。

使い方:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="adjust_bone_rotation.py <config.json>"

config.json（ファイルのパスは、絶対パスか config.json からの相対パス）:
{
  "work_dir": "/Game/05_Adjust",                         # 作る資産の置き場
  "mesh_fbx": "../../Game/Assets/modelData/player/Model/SK_Player.FBX",  # 骨格の元（メッシュ付きの FBX）
  "clips": [
    {"in": "Idle.fbx", "out": "Idle.fbx"}                # in と out は同じでもよい
  ],
  "pitch_deg": {                                         # 正 = 前に倒す、負 = 後ろに起こす（度）
    "J_Bip_C_Spine": 3.0,
    "J_Bip_C_Head": -3.0
  },
  "log": "C:/.../result.txt"                             # 省略すると %TEMP%/adjust_bone_rotation.txt
}

注意: in と out を同じにすると、実行するたびに角度が足されていく。リターゲットし直した直後に1回だけ実行する。

角度は「キャラクターの左右の軸」まわりで、全フレームに同じだけ足す（リターゲットポーズを直すのと同じ）。
左右の軸は、0フレーム目のつま先とかかとの位置からキャラクターの前を求めて決める。
"""
import json
import math
import os
import sys

import unreal

lines = []


def log(*args):
    msg = " ".join(str(a) for a in args)
    lines.append(msg)
    unreal.log("[adjust] " + msg)


def import_fbx(fbx_path, dest_dir, dest_name, skeleton=None):
    """skeleton が None ならスケルタルメッシュとして、あればアニメーションだけを読み込む。"""
    ui = unreal.FbxImportUI()
    ui.automated_import_should_detect_type = False
    ui.import_materials = False
    ui.import_textures = False
    if skeleton is None:
        ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
        ui.import_mesh = True
        ui.import_as_skeletal = True
        ui.import_animations = False
        ui.create_physics_asset = False
    else:
        ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
        ui.import_mesh = False
        ui.import_animations = True
        ui.skeleton = skeleton
        ui.anim_sequence_import_data.set_editor_property(
            "animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        ui.anim_sequence_import_data.set_editor_property("remove_redundant_keys", False)

    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = dest_dir
    task.destination_name = dest_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.options = ui
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    for p in task.imported_object_paths:
        obj = unreal.load_asset(p)
        if skeleton is None and isinstance(obj, unreal.SkeletalMesh):
            return obj
        if skeleton is not None and isinstance(obj, unreal.AnimSequence):
            return obj
    raise RuntimeError("import failed: " + fbx_path)


def export_fbx(anim, path):
    # retarget_to_player.py と同じ設定（メッシュを含めず、骨格とアニメーションだけ）。
    opt = unreal.FbxExportOption()
    opt.ascii = False
    opt.export_morph_targets = False
    opt.export_preview_mesh = False
    opt.map_skeletal_motion_to_root = False
    opt.export_local_time = True
    task = unreal.AssetExportTask()
    task.object = anim
    task.filename = path
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    task.exporter = unreal.AnimSequenceExporterFBX()
    task.options = opt
    ok = unreal.Exporter.run_asset_export_task(task)
    log("export", anim.get_name(), "->", path, ok)
    return ok


def q_inv(q):
    """単位クォータニオンの逆（共役）。"""
    return unreal.Quat(-q.x, -q.y, -q.z, q.w)


def q_mul(a, b):
    """UE の FQuat と同じ順番: a * b は b を先に回してから a を回す。"""
    return unreal.Quat(
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z)


def q_rotate(q, v):
    r = q_mul(q_mul(q, unreal.Quat(v.x, v.y, v.z, 0.0)), q_inv(q))
    return unreal.Vector(r.x, r.y, r.z)


def eval_options():
    opts = unreal.AnimPoseEvaluationOptions()
    opts.set_editor_property("evaluation_type", unreal.AnimDataEvalType.RAW)
    opts.set_editor_property("should_retarget", False)
    opts.set_editor_property("extract_root_motion", False)
    return opts


def get_controller(anim):
    for getter in (lambda: anim.controller, lambda: anim.get_editor_property("controller")):
        try:
            c = getter()
            if c is not None:
                return c
        except Exception:
            pass
    raise RuntimeError("no AnimationDataController")


def side_axis(pose):
    """キャラクターの左右の軸（ワールド）。この軸まわりに正に回すと、上向きが前へ倒れる。
    pose は基準のポーズ（T ポーズ）を渡す。モーション中は足が開いていて、つま先の向きが前とずれるため。"""
    W = unreal.AnimPoseSpaces.WORLD
    P = unreal.AnimPoseExtensions
    foot = P.get_bone_pose(pose, "J_Bip_L_Foot", W).translation
    toe = P.get_bone_pose(pose, "J_Bip_L_ToeBase", W).translation
    up = unreal.Vector(0.0, 0.0, 1.0)
    fwd = unreal.Vector(toe.x - foot.x, toe.y - foot.y, 0.0)
    fwd = fwd.normal()
    axis = up.cross(fwd).normal()  # up を fwd へ回す軸
    log("forward", fwd, "side axis", axis)
    return axis


def adjust(anim, pitch_deg):
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()

    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    first = P.get_anim_pose_at_frame(anim, 0, opts)
    axis = side_axis(P.get_reference_pose(anim.get_editor_property("skeleton")))

    # 先に全フレームの元の値を読む（書き換えると、あとのフレームの評価が変わるため）。
    tracks = {}
    for bone in pitch_deg:
        tracks[bone] = {"pos": [], "rot": [], "scl": []}
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        for bone, deg in pitch_deg.items():
            t = tracks[bone]
            local = P.get_bone_pose(pose, bone, L)
            world = P.get_bone_pose(pose, bone, W)
            # 親のワールドの向き = 自分のワールドの向き × ローカルの向きの逆。
            parent_rot = q_mul(world.rotation, q_inv(local.rotation))
            # ワールドの左右の軸を、親の向きで見た軸に直してから、ローカルの回転の前に掛ける。
            axis_in_parent = q_rotate(q_inv(parent_rot), axis)
            half = math.radians(deg) * 0.5
            s = math.sin(half)
            q = unreal.Quat(axis_in_parent.x * s, axis_in_parent.y * s, axis_in_parent.z * s, math.cos(half))
            t["pos"].append(local.translation)
            t["rot"].append(q_mul(q, local.rotation))
            t["scl"].append(local.scale3d)

    c = get_controller(anim)
    c.open_bracket(unreal.Text("adjust"), False)
    for bone, t in tracks.items():
        ok = c.set_bone_track_keys(bone, t["pos"], t["rot"], t["scl"], False)
        log("set", bone, "deg", pitch_deg[bone], "keys", num_keys, ok)
    c.close_bracket(False)
    unreal.EditorAssetLibrary.save_loaded_asset(anim)

    # 確かめ: 頭と腰の前後の差（0フレーム目、前が正）。
    after = P.get_anim_pose_at_frame(anim, 0, opts)
    fwd = unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0
    for label, pose in (("before", first), ("after", after)):
        head = P.get_bone_pose(pose, "J_Bip_C_Head", W).translation
        hips = P.get_bone_pose(pose, "J_Bip_C_Hips", W).translation
        d = head - hips
        log(label, "head - hips forward(cm) =", round(d.dot(fwd), 2))


def main():
    cfg_path = sys.argv[-1]
    cfg = json.load(open(cfg_path, encoding="utf-8"))
    base = os.path.dirname(os.path.abspath(cfg_path))

    def path(p):
        return os.path.normpath(os.path.join(base, p))

    work = cfg["work_dir"]
    try:
        mesh_path = work + "/SK_Player"
        if unreal.EditorAssetLibrary.does_asset_exist(mesh_path):
            mesh = unreal.load_asset(mesh_path)
        else:
            mesh = import_fbx(path(cfg["mesh_fbx"]), work, "SK_Player")
        skeleton = mesh.skeleton
        for clip in cfg["clips"]:
            name = os.path.splitext(os.path.basename(clip["in"]))[0]
            anim = import_fbx(path(clip["in"]), work + "/Anim", "A_" + name, skeleton)
            log("clip", name, "len", anim.get_play_length())
            adjust(anim, cfg["pitch_deg"])
            export_fbx(anim, path(clip["out"]))
        log("DONE")
    except Exception as e:
        import traceback
        log("ERROR", e, traceback.format_exc())
    finally:
        log_path = path(cfg["log"]) if cfg.get("log") else os.path.join(
            os.environ.get("TEMP", base), "adjust_bone_rotation.txt")
        open(log_path, "w", encoding="utf-8").write("\n".join(lines))


main()
