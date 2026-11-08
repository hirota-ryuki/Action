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
    {"in": "Idle.fbx", "out": "Idle.fbx"},               # in と out は同じでもよい
    {"in": "Run.fbx", "out": "Kick.fbx",                 # frames: 省略可。start から count コマだけを切り出す。
     "frames": {"start": 10, "count": 10}}               #   ループのクリップなら、最後のコマを越えると先頭へ戻る
  ],
  "pitch_deg": {                                         # 正 = 前に倒す、負 = 後ろに起こす（度）
    "J_Bip_C_Spine": 3.0,
    "J_Bip_C_Head": -3.0
  },
  "out_deg": {                                           # 省略可。前後の軸まわりに回して体の外へ開く（度、正 = 外）。
    "J_Bip_L_UpperArm": 8.0                              #   左右とも正で外へ開く
  },
  "splay_deg": {                                         # 省略可。ボーン → 子の線を、体の外の向きへ倒す（度、正 = 外）。
    "J_Bip_L_LowerArm": 20.0                             #   軸はコマごとの「骨の向き × 外向き」なので、骨が前を向いていても
  },                                                     #   ねじれにならず、先の端が外へ出る（out_deg は前後の軸に固定）
  "motion_scale": 0.5,                                   # 省略可。体全体の動きを、平均のポーズを中心に何倍にするか
  "swing_scale": {                                       # 省略可。ボーンの振り幅を、平均の向きを中心に何倍にするか
    "J_Bip_L_UpperLeg": 1.3
  },
  "keep_feet_on_ground": true,                           # 省略可。つま先の一番低い高さが元と同じになるよう腰を上下させる
  "log": "C:/.../result.txt"                             # 省略すると %TEMP%/adjust_bone_rotation.txt
}

注意: in と out を同じにすると、実行するたびに角度が足されていく。リターゲットし直した直後に1回だけ実行する。

角度は「キャラクターの左右の軸」まわりで、全コマに同じだけ足す（リターゲットポーズを直すのと同じ）。
左右の軸は、0コマ目のつま先とかかとの位置からキャラクターの前を求めて決める。
振り幅（swing_scale）は角度を足す前に、元のローカルの回転に対してかける。
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


def q_dot(a, b):
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w


def q_normalize(q):
    n = math.sqrt(q_dot(q, q))
    return unreal.Quat(q.x / n, q.y / n, q.z / n, q.w / n)


def q_mean(quats):
    """回転の平均（向きをそろえて足し、正規化する）。回転の差が小さいときの近似。"""
    ref = quats[0]
    x = y = z = w = 0.0
    for q in quats:
        sign = 1.0 if q_dot(q, ref) >= 0.0 else -1.0
        x += q.x * sign
        y += q.y * sign
        z += q.z * sign
        w += q.w * sign
    return q_normalize(unreal.Quat(x, y, z, w))


def q_scale_from(q, center, k):
    """center から q への回転を k 倍にした回転を返す（振り幅を k 倍にする）。"""
    d = q_mul(q, q_inv(center))
    if d.w < 0.0:
        d = unreal.Quat(-d.x, -d.y, -d.z, -d.w)
    angle = 2.0 * math.acos(max(-1.0, min(1.0, d.w)))
    s = math.sin(angle * 0.5)
    if s < 1e-6:
        return q
    ax, ay, az = d.x / s, d.y / s, d.z / s
    half = angle * k * 0.5
    sk = math.sin(half)
    return q_mul(unreal.Quat(ax * sk, ay * sk, az * sk, math.cos(half)), center)


HIPS = "J_Bip_C_Hips"


def body_shape(anim, fwd):
    """全コマの平均で、体の形を測る（度。前へ倒れているほど大きい）。
    torso: 腰→首の線の前傾。leg: 両足の真ん中→腰の線の前傾（足が腰より後ろにあれば正）。
    torso - leg が「体の折れ」。大きいほど腰で「く」の字に曲がり、お尻が突き出て見える。
    toe_min: つま先の一番低い高さ（cm）。"""
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    opts = eval_options()
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    torso = leg = 0.0
    toe_min = 1e9
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)

        def pos(b):
            return P.get_bone_pose(pose, b, W).translation

        hips, neck = pos(HIPS), pos("J_Bip_C_Neck")
        lf, rf = pos("J_Bip_L_Foot"), pos("J_Bip_R_Foot")
        feet = (lf + rf) * 0.5
        d = neck - hips
        torso += math.degrees(math.atan2(d.dot(fwd), d.z))
        d = hips - feet
        leg += math.degrees(math.atan2(d.dot(fwd), d.z))
        toe_min = min(toe_min, pos("J_Bip_L_ToeBase").z, pos("J_Bip_R_ToeBase").z)
    return {"torso": torso / num_keys, "leg": leg / num_keys, "toe_min": toe_min}


def adjust(anim, pitch_deg, swing_scale, keep_feet_on_ground=False, out_deg=None, motion_scale=1.0, splay_deg=None):
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    out_deg = out_deg or {}
    splay_deg = splay_deg or {}

    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    first = P.get_anim_pose_at_frame(anim, 0, opts)
    ref = P.get_reference_pose(anim.get_editor_property("skeleton"))
    axis = side_axis(ref)
    # out_deg: 前後の軸まわりに回し、ボーンの下の端を体の外へ開く（腕を広げる）。
    # 外の向きは、基準のポーズでボーンが腰のどちら側にあるかで決める。回す軸 = 下向き × 外向き。
    out_axis = {}
    out_side = {}
    for bone in list(out_deg) + list(splay_deg):
        d = P.get_bone_pose(ref, bone, W).translation - P.get_bone_pose(ref, HIPS, W).translation
        side = axis * (1.0 if d.dot(axis) >= 0.0 else -1.0)
        out_side[bone] = side
        out_axis[bone] = unreal.Vector(0.0, 0.0, -1.0).cross(side).normal()

    # 先に全コマの元の値を読む（書き換えると、あとのコマの評価が変わるため）。
    # motion_scale: 体全体の動きを、全コマの平均のポーズを中心に k 倍にする（全ボーンの回転と位置）。
    # 全部の関節を同じ割合で縮めるので、地面に着いた足はほぼその場に残る（Idle の横揺れを抑えるときに使う）。
    scaled = []
    if motion_scale != 1.0:
        scaled = [str(n) for n in P.get_bone_names(first)]
    bones = list(dict.fromkeys(scaled + list(swing_scale) + list(pitch_deg) + list(out_deg) + list(splay_deg) +
                               ([HIPS] if keep_feet_on_ground else [])))
    tracks = {}
    for bone in bones:
        tracks[bone] = {"pos": [], "rot": [], "scl": [], "axis": [], "out_axis": [], "splay_axis": []}
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        for bone in bones:
            t = tracks[bone]
            local = P.get_bone_pose(pose, bone, L)
            world = P.get_bone_pose(pose, bone, W)
            # 親のワールドの向き = 自分のワールドの向き × ローカルの向きの逆。
            parent_rot = q_mul(world.rotation, q_inv(local.rotation))
            # ワールドの左右の軸を、親の向きで見た軸に直しておく（角度を足すときに使う）。
            t["axis"].append(q_rotate(q_inv(parent_rot), axis))
            if bone in out_axis:
                t["out_axis"].append(q_rotate(q_inv(parent_rot), out_axis[bone]))
            # splay_deg の軸 = 骨の向き × 外向き。この軸まわりに正に回すと、骨の先が外向きの方へ倒れる。
            if bone in splay_deg:
                tip = P.get_bone_pose(pose, SEGMENT_CHILD[bone], W).translation - world.translation
                t["splay_axis"].append(q_rotate(q_inv(parent_rot), tip.cross(out_side[bone]).normal()))
            t["pos"].append(local.translation)
            t["rot"].append(local.rotation)
            t["scl"].append(local.scale3d)

    for bone in scaled:
        t = tracks[bone]
        center = q_mean(t["rot"])
        t["rot"] = [q_scale_from(q, center, motion_scale) for q in t["rot"]]
        n = float(len(t["pos"]))
        mean = unreal.Vector(sum(p.x for p in t["pos"]) / n, sum(p.y for p in t["pos"]) / n, sum(p.z for p in t["pos"]) / n)
        t["pos"] = [mean + (p - mean) * motion_scale for p in t["pos"]]

    # 振り幅を変える（平均の向きを中心に k 倍）。
    for bone, k in swing_scale.items():
        t = tracks[bone]
        center = q_mean(t["rot"])
        t["rot"] = [q_scale_from(q, center, k) for q in t["rot"]]

    # 左右の軸まわりの回転を、ローカルの回転の前に掛ける。
    for bone, deg in pitch_deg.items():
        t = tracks[bone]
        half = math.radians(deg) * 0.5
        s = math.sin(half)
        rots = []
        for a, r in zip(t["axis"], t["rot"]):
            q = unreal.Quat(a.x * s, a.y * s, a.z * s, math.cos(half))
            rots.append(q_mul(q, r))
        t["rot"] = rots

    for bone, deg in out_deg.items():
        t = tracks[bone]
        half = math.radians(deg) * 0.5
        s = math.sin(half)
        t["rot"] = [q_mul(unreal.Quat(a.x * s, a.y * s, a.z * s, math.cos(half)), r)
                    for a, r in zip(t["out_axis"], t["rot"])]

    for bone, deg in splay_deg.items():
        t = tracks[bone]
        half = math.radians(deg) * 0.5
        s = math.sin(half)
        t["rot"] = [q_mul(unreal.Quat(a.x * s, a.y * s, a.z * s, math.cos(half)), r)
                    for a, r in zip(t["splay_axis"], t["rot"])]

    fwd =unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0
    before_shape = body_shape(anim, fwd)

    c = get_controller(anim)
    c.open_bracket(unreal.Text("adjust"), False)
    for bone, t in tracks.items():
        ok = c.set_bone_track_keys(bone, t["pos"], t["rot"], t["scl"], False)
        log("set", bone, "deg", pitch_deg.get(bone, 0.0), "out", out_deg.get(bone, 0.0), "splay", splay_deg.get(bone, 0.0), "swing", swing_scale.get(bone, 1.0), "keys", num_keys, ok)
    c.close_bracket(False)

    # 足を地面に合わせる: 脚ごと倒すと足が浮くので、つま先の一番低い高さが元と同じになるまで腰を下げる。
    if keep_feet_on_ground:
        after_shape = body_shape(anim, fwd)
        drop = after_shape["toe_min"] - before_shape["toe_min"]
        log("toe min before", round(before_shape["toe_min"], 2), "after", round(after_shape["toe_min"], 2), "lower hips(cm)", round(drop, 2))
        if abs(drop) > 0.01:
            t = tracks[HIPS]
            pos = []
            for f in range(num_keys):
                pose = P.get_anim_pose_at_frame(anim, f, opts)
                local = P.get_bone_pose(pose, HIPS, L)
                world = P.get_bone_pose(pose, HIPS, W)
                parent_rot = q_mul(world.rotation, q_inv(local.rotation))
                # ワールドで真下へ drop だけ動かす量を、親の向きで見た量に直す。
                offset = q_rotate(q_inv(parent_rot), unreal.Vector(0.0, 0.0, -drop))
                pos.append(t["pos"][f] + offset)
            t["pos"] = pos
            c.open_bracket(unreal.Text("ground"), False)
            c.set_bone_track_keys(HIPS, t["pos"], t["rot"], t["scl"], False)
            c.close_bracket(False)
    unreal.EditorAssetLibrary.save_loaded_asset(anim)

    after_shape = body_shape(anim, fwd)
    for label, sh in (("before", before_shape), ("after", after_shape)):
        log(label, "shape: torso lean(deg) =", round(sh["torso"], 1), "leg lean(deg) =", round(sh["leg"], 1),
            "bend(deg) =", round(sh["torso"] - sh["leg"], 1), "toe min(cm) =", round(sh["toe_min"], 2))

    # 確かめ: 頭と腰の前後の差（0コマ目、前が正）。
    after = P.get_anim_pose_at_frame(anim, 0, opts)
    for label, pose in (("before", first), ("after", after)):
        head = P.get_bone_pose(pose, "J_Bip_C_Head", W).translation
        hips = P.get_bone_pose(pose, "J_Bip_C_Hips", W).translation
        d = head - hips
        forward_cm = d.dot(fwd)
        up_cm = d.z
        # 腰→頭の線が、まっすぐ上から前へ何度倒れているか。
        lean_deg = math.degrees(math.atan2(forward_cm, up_cm))
        log(label, "head - hips forward(cm) =", round(forward_cm, 2), "lean(deg) =", round(lean_deg, 1))


def trim(anim, start, count, hold=0):
    """start から count コマだけを残す。最後のコマ（ループなら先頭と同じ姿勢）を越えたら先頭へ戻る。
    hold を指定すると、最後のコマを hold コマ分くり返して足す（ポーズで止まる間を作る）。"""
    P = unreal.AnimPoseExtensions
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    period = num_keys - 1
    src = [(start + i) % period for i in range(count)]
    src += [src[-1]] * hold
    count = len(src)
    log("trim frames", src)

    names = P.get_bone_names(P.get_anim_pose_at_frame(anim, 0, opts))
    tracks = {n: {"pos": [], "rot": [], "scl": []} for n in names}
    for f in src:
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        for n in names:
            local = P.get_bone_pose(pose, n, L)
            tracks[n]["pos"].append(local.translation)
            tracks[n]["rot"].append(local.rotation)
            tracks[n]["scl"].append(local.scale3d)

    c = get_controller(anim)
    c.open_bracket(unreal.Text("trim"), False)
    c.set_number_of_frames(unreal.FrameNumber(count - 1), False)
    failed = 0
    for n, t in tracks.items():
        if not c.set_bone_track_keys(n, t["pos"], t["rot"], t["scl"], False):
            failed += 1
    c.close_bracket(False)
    unreal.EditorAssetLibrary.save_loaded_asset(anim)
    log("trim bones", len(names), "failed", failed, "len", anim.get_play_length())


# 向きを合わせられるボーンと、その向き（自分 → 子）を測るための子のボーン。
SEGMENT_CHILD = {}
for _s in "LR":
    SEGMENT_CHILD["J_Bip_%s_UpperLeg" % _s] = "J_Bip_%s_LowerLeg" % _s
    SEGMENT_CHILD["J_Bip_%s_LowerLeg" % _s] = "J_Bip_%s_Foot" % _s
    SEGMENT_CHILD["J_Bip_%s_Foot" % _s] = "J_Bip_%s_ToeBase" % _s
    SEGMENT_CHILD["J_Bip_%s_UpperArm" % _s] = "J_Bip_%s_LowerArm" % _s
    SEGMENT_CHILD["J_Bip_%s_LowerArm" % _s] = "J_Bip_%s_Hand" % _s
# 向きを合わせるときに、先に回した親の分を差し引くための親子関係（SEGMENT_CHILD の中だけ）。
SEGMENT_PARENT = {}
for _s in "LR":
    SEGMENT_PARENT["J_Bip_%s_LowerLeg" % _s] = "J_Bip_%s_UpperLeg" % _s
    SEGMENT_PARENT["J_Bip_%s_Foot" % _s] = "J_Bip_%s_LowerLeg" % _s
    SEGMENT_PARENT["J_Bip_%s_LowerArm" % _s] = "J_Bip_%s_UpperArm" % _s


def segment_angle(pose, bone, fwd):
    """ボーン → 子のボーンの線の向き（度）。前 = 0、真上 = 90、真下 = -90、真後ろ = ±180。"""
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    a = P.get_bone_pose(pose, bone, W).translation
    b = P.get_bone_pose(pose, SEGMENT_CHILD[bone], W).translation
    d = b - a
    return math.degrees(math.atan2(d.z, d.dot(fwd)))


def wrap180(deg):
    return (deg + 180.0) % 360.0 - 180.0


def pose_to(anim, cfg):
    """最後のコマで、指定したボーンの向きを目標の角度にする。ramp_start のコマから最後のコマへ、なめらかに寄せる。
    lift_cm を指定すると、腰を上下させる（正 = 持ち上げて宙に浮く、負 = 下げて低く飛ぶ）。
    腰は lift_ramp_start（省略すると ramp_start）のコマから寄せる。接地しているコマで下げると足がめり込むため、
    足が離れるコマを指定する。
    ground_toe_cm を指定すると、lift_cm の代わりに、最後のコマで一番低いつま先がその高さになるよう腰を上下させる
    （しゃがんだポーズで、足を地面に着けるとき）。
    cfg = {"ramp_start": 1, "lift_ramp_start": 3, "lift_cm": -8, "angle_deg": {"J_Bip_L_UpperArm": -60, ...}}
    角度は segment_angle と同じ測り方。左右の軸まわりにしか回さないので、横へ開いた腕などは少しずれる。
    そのため、測り直して残りの差を足すのを3回くり返す。"""
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    last = num_keys - 1
    axis = side_axis(P.get_reference_pose(anim.get_editor_property("skeleton")))
    fwd = unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0
    targets = cfg.get("angle_deg", {})
    lift = cfg.get("lift_cm", 0.0)
    ramp_start = cfg.get("ramp_start", 0)
    ramp_end = cfg.get("ramp_end", last)  # このコマで目標に着き、あとはそのまま。

    # start までは 0、最後のコマで 1（smoothstep）。
    def make_weights(start):
        ws = []
        for f in range(num_keys):
            u = 1.0 if ramp_end <= start else max(0.0, min(1.0, (f - start) / float(ramp_end - start)))
            ws.append(u * u * (3.0 - 2.0 * u))
        return ws
    weights = make_weights(ramp_start)
    lift_weights = make_weights(cfg.get("lift_ramp_start", ramp_start))
    # drift_deg: ramp_end のコマで目標に着いたあと、最後のコマまでに少しずつ足す角度（正 = 前へ倒す。どのボーンでもよい）。
    # 止まったポーズにせず、ゆっくり動き続けるようにする。
    drift = cfg.get("drift_deg", {})
    drift_w = [0.0 if f <= ramp_end or last <= ramp_end else (f - ramp_end) / float(last - ramp_end) for f in range(num_keys)]

    # 元の値を読む。膝は反らないように抑えるので、両足のすねも読んでおく。
    shins = ["J_Bip_%s_LowerLeg" % s for s in "LR"]
    bones = list(dict.fromkeys(list(targets) + [HIPS] + shins + list(drift)))
    extra = {b: [0.0] * num_keys for b in shins}  # コマごとに足す角度（膝が反らないようにする分）。
    orig = {b: {"pos": [], "rot": [], "scl": [], "axis": [], "parent": []} for b in bones}
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        for b in bones:
            local = P.get_bone_pose(pose, b, L)
            world = P.get_bone_pose(pose, b, W)
            parent_rot = q_mul(world.rotation, q_inv(local.rotation))
            o = orig[b]
            o["pos"].append(local.translation)
            o["rot"].append(local.rotation)
            o["scl"].append(local.scale3d)
            o["axis"].append(q_rotate(q_inv(parent_rot), axis))
            o["parent"].append(parent_rot)

    end_pose = P.get_anim_pose_at_frame(anim, ramp_end, opts)
    before = {b: segment_angle(end_pose, b, fwd) for b in targets}
    pitch = {b: 0.0 for b in targets}  # 最後のコマで足す角度（正 = 前へ倒す）。

    def write():
        c = get_controller(anim)
        c.open_bracket(unreal.Text("pose_to"), False)
        for b in bones:
            o = orig[b]
            rots, poss = [], []
            for f in range(num_keys):
                deg = (pitch.get(b, 0.0) * weights[f] + (extra[b][f] if b in extra else 0.0)
                       + drift.get(b, 0.0) * drift_w[f])
                half = math.radians(deg) * 0.5
                s = math.sin(half)
                a = o["axis"][f]
                rots.append(q_mul(unreal.Quat(a.x * s, a.y * s, a.z * s, math.cos(half)), o["rot"][f]))
                p = o["pos"][f]
                if b == HIPS and lift != 0.0:
                    p = p + q_rotate(q_inv(o["parent"][f]), unreal.Vector(0.0, 0.0, lift * lift_weights[f]))
                poss.append(p)
            c.set_bone_track_keys(b, poss, rots, o["scl"], False)
        c.close_bracket(False)

    for it in range(3):
        end_pose = P.get_anim_pose_at_frame(anim, ramp_end, opts)
        # 親から順に、残りの差を足す。前へ回すと角度は小さくなるので、差の符号を反転して足す。
        # 親の角度を変えると子も同じだけ回るので、子は親に足した分を差し引く。
        added = {}
        for b in targets:
            diff = wrap180(targets[b] - segment_angle(end_pose, b, fwd))
            d = -diff
            parent = SEGMENT_PARENT.get(b)
            if parent in added:
                d -= added[parent]
            pitch[b] += d
            added[b] = d
        write()

    # 膝が反らないようにする。曲がり = 太ももの向き − すねの向き（正 = 曲がっている。かかとがお尻へ近づく向き）。
    # min_knee_deg より小さいコマは、すねをその分だけ曲げる。
    min_knee = cfg.get("min_knee_deg", 3.0)
    for it in range(2):
        fixed = 0
        for f in range(num_keys):
            pose = P.get_anim_pose_at_frame(anim, f, opts)
            for s in "LR":
                thigh = "J_Bip_%s_UpperLeg" % s
                shin = "J_Bip_%s_LowerLeg" % s
                bend = wrap180(segment_angle(pose, thigh, fwd) - segment_angle(pose, shin, fwd))
                if bend < min_knee:
                    # すねを前へ回す（正）と向きの角度は小さくなり、曲がりが増える。
                    extra[shin][f] += min_knee - bend
                    fixed += 1
        log("pose_to knee fix pass", it, "frames", fixed)
        if fixed == 0:
            break
        write()

    ground = cfg.get("ground_toe_cm")
    if ground is not None:
        for it in range(2):
            end_pose = P.get_anim_pose_at_frame(anim, ramp_end, opts)
            toe = min(P.get_bone_pose(end_pose, "J_Bip_%s_ToeBase" % s, W).translation.z for s in "LR")
            lift += ground - toe
            write()

    end_pose = P.get_anim_pose_at_frame(anim, ramp_end, opts)
    for b in targets:
        log("pose_to", b, "before", round(before[b], 1), "target", targets[b],
            "after", round(segment_angle(end_pose, b, fwd), 1), "pitch", round(pitch[b], 1))
    unreal.EditorAssetLibrary.save_loaded_asset(anim)
    toe = min(P.get_bone_pose(end_pose, "J_Bip_%s_ToeBase" % s, W).translation.z for s in "LR")
    log("pose_to lift", lift, "lowest toe at end(cm)", round(toe, 1))


def shift_feet(anim, cfg):
    """足首を前後にずらす（腰は動かさない）。太もも・すねの向きを2本の骨の IK（膝は前）で決め、足は地面に対する向きを元のままにする。
    cfg = {"L": [コマごとの前へのずれ(cm)], "R": [...]}。届かないときは脚を伸ばし切ったところで止まる。"""
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    axis = side_axis(P.get_reference_pose(anim.get_editor_property("skeleton")))
    fwd = unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0
    legs = [(sd, "J_Bip_%s_UpperLeg" % sd, "J_Bip_%s_LowerLeg" % sd, "J_Bip_%s_Foot" % sd) for sd in "LR" if sd in cfg]
    bones = [b for leg in legs for b in leg[1:]]

    def planar(v):
        return (v.dot(fwd), v.z)

    orig = {b: {"pos": [], "rot": [], "scl": [], "axis": []} for b in bones}
    goals = []
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        for b in bones:
            local = P.get_bone_pose(pose, b, L)
            world = P.get_bone_pose(pose, b, W)
            parent_rot = q_mul(world.rotation, q_inv(local.rotation))
            o = orig[b]
            o["pos"].append(local.translation)
            o["rot"].append(local.rotation)
            o["scl"].append(local.scale3d)
            o["axis"].append(q_rotate(q_inv(parent_rot), axis))
        g = {}
        for sd, thigh, shin, foot in legs:
            hf, hz = planar(P.get_bone_pose(pose, thigh, W).translation)
            kf, kz = planar(P.get_bone_pose(pose, shin, W).translation)
            af, az = planar(P.get_bone_pose(pose, foot, W).translation)
            l1 = math.hypot(kf - hf, kz - hz)
            l2 = math.hypot(af - kf, az - kz)
            af += cfg[sd][f]
            df, dz = af - hf, az - hz
            dist = min(math.hypot(df, dz), (l1 + l2) * 0.999)
            dist = max(dist, abs(l1 - l2) + 0.01)
            cos_a = (l1 * l1 + dist * dist - l2 * l2) / (2.0 * l1 * dist)
            alpha = math.degrees(math.acos(max(-1.0, min(1.0, cos_a))))
            base = math.degrees(math.atan2(dz, df))
            thigh_deg = base + alpha
            nkf = hf + l1 * math.cos(math.radians(thigh_deg))
            nkz = hz + l1 * math.sin(math.radians(thigh_deg))
            # 届かないときは、目標の方向へ伸ばし切る。
            reach = min(1.0, dist / max(1e-3, math.hypot(df, dz)))
            g[thigh] = thigh_deg
            g[shin] = math.degrees(math.atan2(hz + dz * reach - nkz, hf + df * reach - nkf))
        goals.append(g)

    pitch = {b: [0.0] * num_keys for b in bones}

    def write():
        c = get_controller(anim)
        c.open_bracket(unreal.Text("shift_feet"), False)
        for b in bones:
            o = orig[b]
            rots = []
            for f in range(num_keys):
                half = math.radians(pitch[b][f]) * 0.5
                sn = math.sin(half)
                a = o["axis"][f]
                rots.append(q_mul(unreal.Quat(a.x * sn, a.y * sn, a.z * sn, math.cos(half)), o["rot"][f]))
            c.set_bone_track_keys(b, o["pos"], rots, o["scl"], False)
        c.close_bracket(False)

    for it in range(3):
        for f in range(num_keys):
            pose = P.get_anim_pose_at_frame(anim, f, opts)
            for sd, thigh, shin, foot in legs:
                if abs(cfg[sd][f]) < 1e-6:
                    continue
                d_thigh = -wrap180(goals[f][thigh] - segment_angle(pose, thigh, fwd))
                d_shin = -wrap180(goals[f][shin] - segment_angle(pose, shin, fwd)) - d_thigh
                pitch[thigh][f] += d_thigh
                pitch[shin][f] += d_shin
                pitch[foot][f] = -(pitch[thigh][f] + pitch[shin][f])
        write()
    unreal.EditorAssetLibrary.save_loaded_asset(anim)
    log("shift_feet", cfg)


def crouch(anim, cfg):
    """足の位置を動かさずに腰を落とす（膝を曲げて着地を受け止める形にする）。
    cfg = {"lower_cm": 12, "weights": [0.6, 0.8, 1, 1, 0.7, 0.3, 0]}
    weights はコマごとの割合（コマ数と同じ長さ）。腰を lower_cm × 割合だけ下げ、
    両足とも、足首（Foot）の位置が元と同じになるよう、太ももとすねの向きを2本の骨の IK で決める。
    膝は前へ曲げる。向きは左右の軸まわりの回転だけで合わせるので、測り直して3回くり返す。"""
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    axis = side_axis(P.get_reference_pose(anim.get_editor_property("skeleton")))
    fwd = unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0
    lower = cfg.get("lower_cm", 0.0)
    weights = cfg["weights"]
    if len(weights) != num_keys:
        raise RuntimeError("crouch weights must have %d values" % num_keys)

    legs = []
    for side in "LR":
        legs.append(("J_Bip_%s_UpperLeg" % side, "J_Bip_%s_LowerLeg" % side, "J_Bip_%s_Foot" % side))
    # 足（Foot）は、太もも・すねを回した分を逆に回して、地面に対する向きを元のままにする（つま先がめり込まないように）。
    bones = [HIPS] + [b for leg in legs for b in leg]

    def planar(v):
        return (v.dot(fwd), v.z)

    orig = {b: {"pos": [], "rot": [], "scl": [], "axis": [], "parent": []} for b in bones}
    goals = []  # コマごと、足ごとの太もも・すねの向きの目標（度）。
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        for b in bones:
            local = P.get_bone_pose(pose, b, L)
            world = P.get_bone_pose(pose, b, W)
            parent_rot = q_mul(world.rotation, q_inv(local.rotation))
            o = orig[b]
            o["pos"].append(local.translation)
            o["rot"].append(local.rotation)
            o["scl"].append(local.scale3d)
            o["axis"].append(q_rotate(q_inv(parent_rot), axis))
            o["parent"].append(parent_rot)
        g = {}
        for thigh, shin, foot in legs:
            hf, hz = planar(P.get_bone_pose(pose, thigh, W).translation)
            kf, kz = planar(P.get_bone_pose(pose, shin, W).translation)
            af, az = planar(P.get_bone_pose(pose, foot, W).translation)
            l1 = math.hypot(kf - hf, kz - hz)
            l2 = math.hypot(af - kf, az - kz)
            hz -= lower * weights[f]
            df, dz = af - hf, az - hz
            dist = min(math.hypot(df, dz), (l1 + l2) * 0.999)
            dist = max(dist, abs(l1 - l2) + 0.01)
            cos_a = (l1 * l1 + dist * dist - l2 * l2) / (2.0 * l1 * dist)
            alpha = math.degrees(math.acos(max(-1.0, min(1.0, cos_a))))
            thigh_deg = math.degrees(math.atan2(dz, df)) + alpha  # 膝を前へ出す側
            nkf = hf + l1 * math.cos(math.radians(thigh_deg))
            nkz = hz + l1 * math.sin(math.radians(thigh_deg))
            g[thigh] = thigh_deg
            g[shin] = math.degrees(math.atan2(az - nkz, af - nkf))
        goals.append(g)

    pitch = {b: [0.0] * num_keys for b in bones}

    def write():
        c = get_controller(anim)
        c.open_bracket(unreal.Text("crouch"), False)
        for b in bones:
            o = orig[b]
            rots, poss = [], []
            for f in range(num_keys):
                half = math.radians(pitch[b][f]) * 0.5
                sn = math.sin(half)
                a = o["axis"][f]
                rots.append(q_mul(unreal.Quat(a.x * sn, a.y * sn, a.z * sn, math.cos(half)), o["rot"][f]))
                p = o["pos"][f]
                if b == HIPS:
                    p = p + q_rotate(q_inv(o["parent"][f]), unreal.Vector(0.0, 0.0, -lower * weights[f]))
                poss.append(p)
            c.set_bone_track_keys(b, poss, rots, o["scl"], False)
        c.close_bracket(False)

    write()  # 先に腰だけ下げる（向きは、下げたあとの角度との差で合わせる）。
    for it in range(3):
        for f in range(num_keys):
            if weights[f] <= 0.0:
                continue
            pose = P.get_anim_pose_at_frame(anim, f, opts)
            for thigh, shin, foot in legs:
                d_thigh = -wrap180(goals[f][thigh] - segment_angle(pose, thigh, fwd))
                d_shin = -wrap180(goals[f][shin] - segment_angle(pose, shin, fwd)) - d_thigh
                pitch[thigh][f] += d_thigh
                pitch[shin][f] += d_shin
                pitch[foot][f] = -(pitch[thigh][f] + pitch[shin][f])
        write()
    unreal.EditorAssetLibrary.save_loaded_asset(anim)

    # 確かめ: 膝の曲がり（太もも − すね）と、つま先の高さ。
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        row = "crouch f%02d w=%.2f" % (f, weights[f])
        for thigh, shin, foot in legs:
            bend = wrap180(segment_angle(pose, thigh, fwd) - segment_angle(pose, shin, fwd))
            toe = P.get_bone_pose(pose, foot.replace("Foot", "ToeBase"), W).translation.z
            row += "  %s knee=%.0f toe=%.1f" % (thigh[6], bend, toe)
        row += "  hips=%.1f" % P.get_bone_pose(pose, HIPS, W).translation.z
        log(row)


TORSO_CHAIN = ["J_Bip_C_Hips", "J_Bip_C_Spine", "J_Bip_C_Chest", "J_Bip_C_UpperChest"]


def rotoscope(anim, cfg, base_dir):
    """動画から読み取った角度で、コマごとにポーズを作る（ロトスコープ）。
    cfg = {"data": "genshin_dash_pose.json", "start": 4, "count": 17,
           "map": {"A": "L", "B": "R", "a": "L", "b": "R"},
           "torso_split": [0.4, 0.2, 0.2, 0.2], "toe_offset_cm": 0}
    data はコマごとの角度（前 0°・真上 90°・真下 -90°）。A・B は脚（thigh・shin・foot、toeA・toeB は脚の長さを1とした
    つま先の高さ）、a・b は腕（upper・fore）、torso は腰 → 肩、neck は肩 → 耳。map で、どれをこのキャラクターの左右に当てるか決める。
    クリップはあらかじめ count コマにしておく（frames で切り出す）。目標の向きになるよう、左右の軸まわりの角度を
    親から順に求め、測り直して3回くり返す。最後に、コマごとに一番低いつま先が toe（× 脚の長さ）になるよう腰を上下させる。"""
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    data = json.load(open(os.path.join(base_dir, cfg["data"]), encoding="utf-8"))
    rows = data[cfg["start"]:cfg["start"] + cfg["count"]]
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    if len(rows) != num_keys:
        raise RuntimeError("rotoscope: %d rows for %d keys" % (len(rows), num_keys))
    skel_ref = P.get_reference_pose(anim.get_editor_property("skeleton"))
    axis = side_axis(skel_ref)
    fwd = unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0
    m = cfg["map"]

    def dist(a, b):
        return (P.get_bone_pose(skel_ref, a, W).translation - P.get_bone_pose(skel_ref, b, W).translation).length()
    leg_cm = dist("J_Bip_L_UpperLeg", "J_Bip_L_LowerLeg") + dist("J_Bip_L_LowerLeg", "J_Bip_L_Foot")
    log("rotoscope leg length(cm)", round(leg_cm, 1))

    # コマごとの目標（このキャラクターのボーン名で）。
    # neck_add_deg と neck_add_frames [a, b]: 首（首 → 頭の線）を、a コマ目の 0° から b コマ目の neck_add_deg まで
    # smoothstep でデータより起こす（止まるの最後を Idle の首の角度にそろえ、Idle へ移るとき頭が後ろへ引かれないように）。
    neck_add = cfg.get("neck_add_deg", 0.0)
    na, nb = cfg.get("neck_add_frames", [0, 1])
    # arm_smooth: {"a": 2.5}: その腕の肘の曲がり（前腕 − 上腕）を、前後のコマとガウスでならす（数字は幅のコマ数）。
    # 読み取りで肘が数コマのうちに大きく曲げ伸ばしされ、腕を急に振ったように見えるのを抑える。上腕はならさない
    # （始めと終わりの角度を変えず、前後のモーションとのつながりを保つ）。肘は elbow_min_deg（省略すると 5°）より伸ばさない。
    elbows = {}
    for src, sigma in cfg.get("arm_smooth", {}).items():
        raw = [wrap180(r[src + "fore"] - r[src + "upper"]) for r in rows]
        reach = int(3 * sigma) + 1
        sm = []
        for i in range(len(raw)):
            s = t = 0.0
            for j in range(-reach, reach + 1):
                g = math.exp(-j * j / (2.0 * sigma * sigma))
                s += g * raw[min(len(raw) - 1, max(0, i + j))]
                t += g
            sm.append(max(cfg.get("elbow_min_deg", 5.0), s / t))
        elbows[src] = sm
    # tuck_knee_deg でたたむのは、太ももが一番後ろから一番前まで振られていく間だけ（振り戻す脚）。
    # 一番前まで出て戻り始めた脚は、着地しようとしているのでたたまない（たたむと、着地の1コマ前に膝が折れ、
    # 着地で一気に伸びて、すねがばたついて見えた）。ループのクリップ（最後のコマ = 最初のコマ）は1周を輪にして数える。
    swing = {}
    if cfg.get("tuck_knee_deg", 0.0) > 0.0:
        for src in "AB":
            th = [r[src + "thigh"] for r in rows]
            cycle = len(th) - 1 if len(th) > 1 and abs(th[0] - th[-1]) < 1e-6 else len(th)
            back = min(range(cycle), key=lambda k: th[k])
            front = max(range(cycle), key=lambda k: th[k])
            frames = set()
            k = back
            while True:
                frames.add(k)
                if k == front:
                    break
                k = (k + 1) % cycle
            swing[src] = (frames, cycle)
    targets = []
    for i, r in enumerate(rows):
        # lean_deg: 胴体と首を、データよりさらに前へ倒す角度（正 = 前へ）。
        lean = cfg.get("lean_deg", 0.0)
        u = max(0.0, min(1.0, (i - na) / float(max(1, nb - na))))
        t = {"torso": r["torso"] - lean, "neck": r["neck"] - lean + neck_add * u * u * (3.0 - 2.0 * u)}
        # leg_lean_deg: 脚（太もも・すね・足）もこの角度だけ前へ倒す（体全体を一本の線で倒し、足が腰の下に着くように）。
        # tuck_knee_deg: 振り戻す脚が腰の下を通るとき（足が浮いていて、太ももが -95°〜-35°）、膝をこの角度以上に曲げて、
        # かかとをお尻へたたむ（太ももがまだ後ろにあるうちにたたむと、かかとが高く跳ね上がるので -95° から）。すねを回した分だけ足も回し、つま先の向きは崩さない。
        leg_lean = cfg.get("leg_lean_deg", 0.0)
        tuck = cfg.get("tuck_knee_deg", 0.0)
        # thigh_max_deg: 太ももを、この角度より上へ上げない（前 0°・真下 -90°）。上限の手前 thigh_soft_deg（省略すると 10°）から
        # なだらかに寄せ、上限で急に止まらないようにする。すねはデータのままなので膝の曲がりが減り、腰は高さの合わせで上がる。
        # 短いスカートのモデルで、太ももが水平まで上がってスカートを持ち上げないようにするために使う。
        thigh_max = cfg.get("thigh_max_deg")
        thigh_soft = cfg.get("thigh_soft_deg", 10.0)
        for src in "AB":
            sd = m[src]
            thigh, shin, foot = r[src + "thigh"], r[src + "shin"], r[src + "foot"]
            if thigh_max is not None and thigh > thigh_max - thigh_soft:
                knee0 = thigh_max - thigh_soft
                thigh = knee0 + thigh_soft * math.tanh((thigh - knee0) / thigh_soft)
            if tuck > 0.0 and r["toe" + src] > 0.12 and (i % swing[src][1]) in swing[src][0]:
                def ramp(x, a, b):
                    u = max(0.0, min(1.0, (x - a) / (b - a)))
                    return u * u * (3.0 - 2.0 * u)
                w = ramp(thigh, -95.0, -80.0) * (1.0 - ramp(thigh, -50.0, -35.0))
                knee = wrap180(thigh - shin)
                if knee < tuck and w > 0.0:
                    new_shin = thigh - (knee + (tuck - knee) * w)
                    foot += new_shin - shin
                    shin = new_shin
            t["J_Bip_%s_UpperLeg" % sd] = thigh - leg_lean
            t["J_Bip_%s_LowerLeg" % sd] = shin - leg_lean
            t["J_Bip_%s_Foot" % sd] = foot - leg_lean
        # arm_hold: {"a": [上腕, 前腕], "b": [...], "frames": [a, b, c, d]}: 腕を決まったポーズで止める。
        # a → b コマ目で読み取りの値からポーズへ寄せ（smoothstep）、b〜c コマ目は止め、c → d コマ目で読み取りの値へ戻す。
        # 体に隠れた腕は読み取りが乱れてばたつくので、原神で腕が動かない区間（ダッシュの低く滑るところ）に使う。
        hold = cfg.get("arm_hold")
        hw = 0.0
        if hold:
            ha, hb, hc, hd = hold["frames"]
            if hb <= i <= hc:
                hw = 1.0
            else:
                u = (i - ha) / float(hb - ha) if i < hb else (hd - i) / float(hd - hc)
                u = max(0.0, min(1.0, u))
                hw = u * u * (3.0 - 2.0 * u)
        for src in "ab":
            sd = m[src]
            up, fore = r[src + "upper"], r[src + "fore"]
            if src in elbows:
                fore = up + elbows[src][i]
            if hw > 0.0 and src in hold:
                # 読み取りの値を、止めるポーズに近い向きへそろえてから混ぜる（360° 回って見えないように）。
                hu, hf = hold[src]
                up = hu + wrap180(up - hu) * (1.0 - hw)
                fore = hf + wrap180(fore - hf) * (1.0 - hw)
            t["J_Bip_%s_UpperArm" % sd] = up
            t["J_Bip_%s_LowerArm" % sd] = fore
        t["toe_cm"] = min(r["toeA"], r["toeB"]) * leg_cm + cfg.get("toe_offset_cm", 0.0)
        t["hip_cm"] = r["hipH"] * leg_cm
        targets.append(t)

    limbs = [b for b in targets[0] if b.startswith("J_Bip_") and b in SEGMENT_CHILD]
    bones = TORSO_CHAIN + ["J_Bip_C_Neck", "J_Bip_C_Head"] + limbs
    # head_pitch_deg: 頭だけに足す角度（正 = うつむく、負 = 顔を起こす）。首の線は変えずに、顔の向きだけを変える。
    head_pitch = cfg.get("head_pitch_deg", 0.0)
    # face_deg [始め, 終わり] と face_frames [a, b]: 頭の角度を足すのでなく、顔の向き（水平から、正 = 上を向く。
    # 基準のポーズで前を向く向きを 0°）を決める。a コマ目まで「始め」、b コマ目から「終わり」、間は smoothstep。
    # 首の角度が大きく変わっても、顔は一方向へだけ動く（止まるで起き上がるとき、首が起きて上を向かないように）。
    face_cfg = cfg.get("face_deg")
    face_target = None
    head_ref = None
    if face_cfg:
        fa, fb = cfg["face_frames"]
        face_target = []
        for f in range(num_keys):
            u = max(0.0, min(1.0, (f - fa) / float(max(1, fb - fa))))
            face_target.append(face_cfg[0] + (face_cfg[1] - face_cfg[0]) * u * u * (3.0 - 2.0 * u))
        head_ref = q_rotate(q_inv(P.get_bone_pose(skel_ref, "J_Bip_C_Head", W).rotation), fwd)
    orig = {b: {"pos": [], "rot": [], "scl": [], "axis": [], "parent": []} for b in bones}
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        for b in bones:
            local = P.get_bone_pose(pose, b, L)
            world = P.get_bone_pose(pose, b, W)
            parent_rot = q_mul(world.rotation, q_inv(local.rotation))
            o = orig[b]
            o["pos"].append(local.translation)
            o["rot"].append(local.rotation)
            o["scl"].append(local.scale3d)
            o["axis"].append(q_rotate(q_inv(parent_rot), axis))
            o["parent"].append(parent_rot)

    split = cfg.get("torso_split", [0.4, 0.2, 0.2, 0.2])
    # 鎖ごとの合計の回転（正 = 前へ倒す）。ボーン自身の回転 = 自分の合計 − 親の合計。
    tot = [dict() for _ in range(num_keys)]
    lift = [0.0] * num_keys
    parent_of = {}
    for b in limbs:
        parent_of[b] = SEGMENT_PARENT.get(b, "torso" if "Arm" in b else "hips")

    def own(f, b):
        t = tot[f]
        T = t.get("torso", 0.0)
        if b in TORSO_CHAIN:
            return T * split[TORSO_CHAIN.index(b)]
        if b == "J_Bip_C_Neck":
            return t.get("neck", 0.0) - T
        if b == "J_Bip_C_Head":
            return t.get("head", head_pitch)
        par = parent_of[b]
        base = T * split[0] if par == "hips" else (T if par == "torso" else t.get(par, 0.0))
        return t.get(b, 0.0) - base

    def write():
        c = get_controller(anim)
        c.open_bracket(unreal.Text("rotoscope"), False)
        for b in bones:
            o = orig[b]
            rots, poss = [], []
            for f in range(num_keys):
                half = math.radians(own(f, b)) * 0.5
                sn = math.sin(half)
                a = o["axis"][f]
                rots.append(q_mul(unreal.Quat(a.x * sn, a.y * sn, a.z * sn, math.cos(half)), o["rot"][f]))
                p = o["pos"][f]
                if b == HIPS:
                    p = p + q_rotate(q_inv(o["parent"][f]), unreal.Vector(0.0, 0.0, lift[f]))
                poss.append(p)
            c.set_bone_track_keys(b, poss, rots, o["scl"], False)
        c.close_bracket(False)

    def seg(pose, a, b):
        d = P.get_bone_pose(pose, b, W).translation - P.get_bone_pose(pose, a, W).translation
        return math.degrees(math.atan2(d.z, d.dot(fwd)))

    for it in range(4):
        worst = 0.0
        for f in range(num_keys):
            pose = P.get_anim_pose_at_frame(anim, f, opts)
            t = tot[f]
            # 胴体（腰 → 首）と首（首 → 頭）。
            for key, a, b in (("torso", HIPS, "J_Bip_C_Neck"), ("neck", "J_Bip_C_Neck", "J_Bip_C_Head")):
                r = wrap180(targets[f][key] - seg(pose, a, b))
                t[key] = t.get(key, 0.0) - r
                worst = max(worst, abs(r))
            if face_target:
                v = q_rotate(P.get_bone_pose(pose, "J_Bip_C_Head", W).rotation, head_ref)
                r = wrap180(face_target[f] - math.degrees(math.atan2(v.z, v.dot(fwd))))
                # 頭の回転は正 = うつむく（顔が下へ）なので、上へ r だけ向けるには r を引く。
                t["head"] = t.get("head", head_pitch) - r
                worst = max(worst, abs(r))
            for b in limbs:
                r = wrap180(targets[f][b] - segment_angle(pose, b, fwd))
                t[b] = t.get(b, 0.0) - r
                worst = max(worst, abs(r))
        write()
        log("rotoscope pass", it, "worst residual(deg)", round(worst, 1))

    # 高さ: "toe" はコマごとに一番低いつま先を合わせる。"hips" は腰の高さの動きを動画に合わせ、
    # クリップ全体で一番低いつま先が 0.5cm になるよう全体を上下させる（走りのように、両足が浮くコマで跳ね上がらないように）。
    mode = cfg.get("height", "toe")
    for it in range(2):
        toes = []
        for f in range(num_keys):
            pose = P.get_anim_pose_at_frame(anim, f, opts)
            toe = min(P.get_bone_pose(pose, "J_Bip_%s_ToeBase" % sd, W).translation.z for sd in "LR")
            if mode == "hips":
                hz = P.get_bone_pose(pose, HIPS, W).translation.z
                lift[f] += targets[f]["hip_cm"] - hz
                toes.append(toe + targets[f]["hip_cm"] - hz)
            else:
                lift[f] += targets[f]["toe_cm"] - toe
        if mode == "hips":
            shift = 0.5 - min(toes)
            lift = [x + shift for x in lift]
            for t in targets:
                t["hip_cm"] += shift
            # 動画で足が地面に着いているコマ（つま先の高さが脚の長さの 0.08 未満）は、浮いた分だけ腰を下げて足を着ける。
            # 1コマで腰が跳ねないよう、下げる量を前後のコマへなだらかに広げる（ループなので端はつなげる。最後のコマ = 最初のコマ）。
            if it == 1:
                n = num_keys - 1
                corr = [0.0] * n
                for f in range(n):
                    gap = toes[f] + shift - 0.5
                    if (rows[f]["toeA"] < 0.08 or rows[f]["toeB"] < 0.08) and gap > 0.0:
                        corr[f] = gap
                for _ in range(3):
                    corr = [max(corr[f], 0.25 * corr[f - 1] + 0.5 * corr[f] + 0.25 * corr[(f + 1) % n]) for f in range(n)]
                    corr = [0.25 * corr[f - 1] + 0.5 * corr[f] + 0.25 * corr[(f + 1) % n] for f in range(n)]
                for f in range(num_keys):
                    lift[f] -= corr[f % n]
        write()
    unreal.EditorAssetLibrary.save_loaded_asset(anim)


def skirt_follow(anim, cfg):
    """スカートのボーン（J_Sec_*_Skirt*）が、親の回りに一部だけついていくようにする。
    cfg = {"follow": {"Front": 0.5, "Back": 0.25, "Side": 0.5}, "copies": ["01", "02", "03"]}
    SK_Player のスカートは前・後ろ（J_Sec_L/R_SkirtFront0・SkirtBack0）が太ももの子、横（SkirtSide0）が腰の子で、
    モーションでは動かないので、太ももを上げると前と後ろの布がそのまま一緒に回ってめくれる。
    親（太もも、横は腰 → 背骨の付け根の線）が基準のポーズから左右の軸まわりに回った角度を d とし、
    スカートの根元を (1 - k) × d だけ戻す。k = 1 なら今までどおり、k = 0 なら基準のポーズのまま垂れる。"""
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    ref = P.get_reference_pose(anim.get_editor_property("skeleton"))
    axis = side_axis(ref)
    fwd = unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0

    def seg(pose, a, b):
        d = P.get_bone_pose(pose, b, W).translation - P.get_bone_pose(pose, a, W).translation
        return math.degrees(math.atan2(d.z, d.dot(fwd)))

    # (スカートの根元, 親の線の始まり, 親の線の終わり, k, 測る線の終わり)
    chains = []
    for part, k in cfg["follow"].items():
        for sd in "LR":
            for cp in cfg.get("copies", ["01", "02", "03"]):
                bone = "J_Sec_%s_Skirt%s0_%s" % (sd, part, cp)
                tip = "J_Sec_%s_Skirt%s2_end_%s" % (sd, part, cp)
                if part == "Side":
                    pa, pb = HIPS, "J_Bip_C_Spine"
                else:
                    pa, pb = "J_Bip_%s_UpperLeg" % sd, "J_Bip_%s_LowerLeg" % sd
                chains.append((bone, pa, pb, k, tip))
    ref_parent = {(pa, pb): seg(ref, pa, pb) for _, pa, pb, _, _ in chains}

    tracks = {c[0]: {"pos": [], "rot": [], "scl": []} for c in chains}
    rows = []
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        row = []
        for bone, pa, pb, k, tip in chains:
            d = wrap180(seg(pose, pa, pb) - ref_parent[(pa, pb)])
            local = P.get_bone_pose(pose, bone, L)
            world = P.get_bone_pose(pose, bone, W)
            parent_rot = q_mul(world.rotation, q_inv(local.rotation))
            a = q_rotate(q_inv(parent_rot), axis)
            # 正に回すと、下を向いた布の先は後ろへ動き、向きの角度は減る。親が d 回った分の (1 - k) を戻す。
            half = math.radians((1.0 - k) * d) * 0.5
            s = math.sin(half)
            t = tracks[bone]
            t["pos"].append(local.translation)
            t["rot"].append(q_mul(unreal.Quat(a.x * s, a.y * s, a.z * s, math.cos(half)), local.rotation))
            t["scl"].append(local.scale3d)
            if bone.endswith("_01") and bone[6] == "L":
                row.append((bone.split("_")[3][:-1], round(d), round(seg(pose, bone, tip))))
        rows.append(row)

    c = get_controller(anim)
    c.open_bracket(unreal.Text("skirt_follow"), False)
    for bone, t in tracks.items():
        for add in ("add_bone_track", "add_bone_curve"):
            fn = getattr(c, add, None)
            if fn is not None:
                try:
                    fn(bone, False)
                except Exception:
                    pass
                break
        c.set_bone_track_keys(bone, t["pos"], t["rot"], t["scl"], False)
    c.close_bracket(False)
    unreal.EditorAssetLibrary.save_loaded_asset(anim)

    # 確かめ用: 書いたあとの布の向き（根元 → 先、前 0°・真下 -90°）。
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        msg = "skirt f%02d" % f
        for (part, d, before), (bone, pa, pb, k, tip) in zip(rows[f], [ch for ch in chains if ch[0].endswith("_01") and ch[0][6] == "L"]):
            msg += "  %s parent=%+d before=%d after=%d" % (part, d, before, round(seg(pose, bone, tip)))
        log(msg)


def blend_from(anim, cfg, anim_dir):
    """最初のコマを、別のクリップの最後のコマのポーズから始める（つなぎ目をなめらかにする）。
    cfg = {"clip": "DashCrouchL", "weights": [1.0, 0.7, 0.4, 0.15]}
    clip は同じ実行の中で先に作ったクリップの名前。weights はコマごとに、そのポーズをどれだけ混ぜるか
    （先頭のコマから。足りないコマは 0）。全ボーンのローカルの回転と位置を混ぜる。
    混ぜたコマで、つま先が ground_toe_cm（省略すると 0.5cm）より下になったら、その分だけ腰を上げる。
    bend_knees を true にすると、上げた腰を crouch で元の高さへ戻し、代わりに膝を曲げて足を地面の上に保つ
    （腰が一度上がってから下がる、上下の揺れを出さないため）。"""
    P = unreal.AnimPoseExtensions
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    src = unreal.load_asset(anim_dir + "/A_" + cfg["clip"])
    src_pose = P.get_anim_pose_at_frame(src, unreal.AnimationLibrary.get_num_frames(src), opts)
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    weights = list(cfg["weights"]) + [0.0] * num_keys
    names = [str(n) for n in P.get_bone_names(P.get_anim_pose_at_frame(anim, 0, opts))]
    tracks = {n: {"pos": [], "rot": [], "scl": []} for n in names}
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        w = weights[f]
        for n in names:
            a = P.get_bone_pose(pose, n, L)
            b = P.get_bone_pose(src_pose, n, L)
            q, r = a.rotation, b.rotation
            sign = 1.0 if q_dot(q, r) >= 0.0 else -1.0
            rot = q_normalize(unreal.Quat(q.x * (1 - w) + r.x * w * sign, q.y * (1 - w) + r.y * w * sign,
                                          q.z * (1 - w) + r.z * w * sign, q.w * (1 - w) + r.w * w * sign))
            tracks[n]["pos"].append(a.translation * (1 - w) + b.translation * w)
            tracks[n]["rot"].append(rot)
            tracks[n]["scl"].append(a.scale3d)
    c = get_controller(anim)
    c.open_bracket(unreal.Text("blend_from"), False)
    for n, t in tracks.items():
        c.set_bone_track_keys(n, t["pos"], t["rot"], t["scl"], False)
    c.close_bracket(False)

    W = unreal.AnimPoseSpaces.WORLD
    ground = cfg.get("ground_toe_cm", 0.5)
    t = tracks[HIPS]
    raised = []
    for f in range(num_keys):
        if weights[f] <= 0.0:
            break
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        toe = min(P.get_bone_pose(pose, "J_Bip_%s_ToeBase" % sd, W).translation.z for sd in "LR")
        if toe < ground:
            local = P.get_bone_pose(pose, HIPS, L)
            world = P.get_bone_pose(pose, HIPS, W)
            parent_rot = q_mul(world.rotation, q_inv(local.rotation))
            t["pos"][f] = t["pos"][f] + q_rotate(q_inv(parent_rot), unreal.Vector(0.0, 0.0, ground - toe))
            raised.append((f, round(ground - toe, 1)))
    if raised:
        c.open_bracket(unreal.Text("blend_from_ground"), False)
        c.set_bone_track_keys(HIPS, t["pos"], t["rot"], t["scl"], False)
        c.close_bracket(False)
    unreal.EditorAssetLibrary.save_loaded_asset(anim)
    log("blend_from", cfg["clip"], "weights", cfg["weights"], "raised hips", raised)
    if raised and cfg.get("bend_knees"):
        top = max(r for _, r in raised)
        ws = [0.0] * num_keys
        for f, r in raised:
            ws[f] = r / top
        crouch(anim, {"lower_cm": top, "weights": ws})


def log_frames(anim, label):
    """コマごとの脚の向き・膝の曲がり・つま先と腰の高さを書き出す（形を決めるときの確かめ用）。"""
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    opts = eval_options()
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    axis = side_axis(P.get_reference_pose(anim.get_editor_property("skeleton")))
    fwd = unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0
    for f in range(num_keys):
        pose = P.get_anim_pose_at_frame(anim, f, opts)
        hips = P.get_bone_pose(pose, HIPS, W).translation

        def seg(a_, b_):
            d = P.get_bone_pose(pose, b_, W).translation - P.get_bone_pose(pose, a_, W).translation
            return math.degrees(math.atan2(d.z, d.dot(fwd)))
        # 骨盤の向き（腰 → 背骨の付け根）と、背骨（付け根 → 首）の向き。差が大きいほど骨盤が前へ傾き、お尻が上がる。
        pelvis = seg(HIPS, "J_Bip_C_Spine")
        spine = seg("J_Bip_C_Spine", "J_Bip_C_Neck")
        row = "%s f%02d hips z=%.1f pelvis=%.0f spine=%.0f tilt=%.0f" % (label, f, hips.z, pelvis, spine, wrap180(spine - pelvis))
        for s in "LR":
            th = segment_angle(pose, "J_Bip_%s_UpperLeg" % s, fwd)
            sh = segment_angle(pose, "J_Bip_%s_LowerLeg" % s, fwd)
            ankle = P.get_bone_pose(pose, "J_Bip_%s_Foot" % s, W).translation - hips
            toe = P.get_bone_pose(pose, "J_Bip_%s_ToeBase" % s, W).translation.z
            ft = segment_angle(pose, "J_Bip_%s_Foot" % s, fwd)
            row += "  %s thigh=%.0f shin=%.0f knee=%.0f foot=%.0f ankle(fwd=%.0f) toe=%.1f" % (
                s, th, sh, wrap180(th - sh), ft, ankle.dot(fwd), toe)
        for s in "LR":
            row += "  %s arm=%.0f/%.0f" % (s, segment_angle(pose, "J_Bip_%s_UpperArm" % s, fwd),
                                            segment_angle(pose, "J_Bip_%s_LowerArm" % s, fwd))
            # 肩から見た肘・手の、体の外への距離（cm、正 = 外）。脇の締まり具合を見る。
            sh_ = P.get_bone_pose(pose, "J_Bip_%s_UpperArm" % s, W).translation
            out = axis * (1.0 if (sh_ - hips).dot(axis) >= 0.0 else -1.0)
            row += " out(elbow=%.1f hand=%.1f)" % (
                (P.get_bone_pose(pose, "J_Bip_%s_LowerArm" % s, W).translation - sh_).dot(out),
                (P.get_bone_pose(pose, "J_Bip_%s_Hand" % s, W).translation - sh_).dot(out))
        log(row)


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
            # 資産の名前は書き出す方の名前にする（同じ in から複数のクリップを作れるように）。
            name = os.path.splitext(os.path.basename(clip["out"]))[0]
            anim = import_fbx(path(clip["in"]), work + "/Anim", "A_" + name, skeleton)
            log("clip", name, "len", anim.get_play_length())
            # pitch_deg・swing_scale・keep_feet_on_ground は、クリップごとに上書きできる。
            adjust(anim, clip.get("pitch_deg", cfg["pitch_deg"]), clip.get("swing_scale", cfg.get("swing_scale", {})),
                   clip.get("keep_feet_on_ground", cfg.get("keep_feet_on_ground", False)),
                   clip.get("out_deg", cfg.get("out_deg", {})),
                   clip.get("motion_scale", cfg.get("motion_scale", 1.0)),
                   clip.get("splay_deg", cfg.get("splay_deg", {})))
            frames = clip.get("frames")
            if frames:
                trim(anim, frames["start"], frames["count"], frames.get("hold", 0))
            # crouch_after_pose なら、決めたポーズへ寄せてから腰を落とす（しゃがんだポーズで、さらに沈み込むとき）。
            after = clip.get("crouch_after_pose", False)
            # crouch_after_rotoscope なら、ロトスコープでポーズを作ってから腰を落とす（走りで足を踏み込むとき）。
            after_roto = clip.get("crouch_after_rotoscope", False)
            if clip.get("crouch") and not after and not after_roto:
                crouch(anim, clip["crouch"])
            if clip.get("pose_to"):
                pose_to(anim, clip["pose_to"])
            if clip.get("crouch") and after:
                crouch(anim, clip["crouch"])
            if clip.get("rotoscope"):
                rotoscope(anim, clip["rotoscope"], base)
            if clip.get("crouch") and after_roto:
                crouch(anim, clip["crouch"])
            if clip.get("shift_feet"):
                shift_feet(anim, clip["shift_feet"])
            if clip.get("blend_from"):
                blend_from(anim, clip["blend_from"], work + "/Anim")
            if clip.get("skirt_follow"):
                skirt_follow(anim, clip["skirt_follow"])
            if clip.get("log_frames"):
                log_frames(anim, name)
            if clip.get("out_skip"):
                continue
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
