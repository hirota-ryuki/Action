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
  "swing_scale": {                                       # 省略可。ボーンの振り幅を、平均の向きを中心に何倍にするか
    "J_Bip_L_UpperLeg": 1.3
  },
  "keep_feet_on_ground": true,                           # 省略可。つま先の一番低い高さが元と同じになるよう腰を上下させる
  "log": "C:/.../result.txt"                             # 省略すると %TEMP%/adjust_bone_rotation.txt
}

注意: in と out を同じにすると、実行するたびに角度が足されていく。リターゲットし直した直後に1回だけ実行する。

角度は「キャラクターの左右の軸」まわりで、全フレームに同じだけ足す（リターゲットポーズを直すのと同じ）。
左右の軸は、0フレーム目のつま先とかかとの位置からキャラクターの前を求めて決める。
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
    """全フレームの平均で、体の形を測る（度。前へ倒れているほど大きい）。
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


def adjust(anim, pitch_deg, swing_scale, keep_feet_on_ground=False):
    P = unreal.AnimPoseExtensions
    W = unreal.AnimPoseSpaces.WORLD
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()

    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    first = P.get_anim_pose_at_frame(anim, 0, opts)
    axis = side_axis(P.get_reference_pose(anim.get_editor_property("skeleton")))

    # 先に全フレームの元の値を読む（書き換えると、あとのフレームの評価が変わるため）。
    bones = list(dict.fromkeys(list(swing_scale) + list(pitch_deg) + ([HIPS] if keep_feet_on_ground else [])))
    tracks = {}
    for bone in bones:
        tracks[bone] = {"pos": [], "rot": [], "scl": [], "axis": []}
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
            t["pos"].append(local.translation)
            t["rot"].append(local.rotation)
            t["scl"].append(local.scale3d)

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

    fwd = unreal.Vector(0.0, 0.0, 1.0).cross(axis) * -1.0
    before_shape = body_shape(anim, fwd)

    c = get_controller(anim)
    c.open_bracket(unreal.Text("adjust"), False)
    for bone, t in tracks.items():
        ok = c.set_bone_track_keys(bone, t["pos"], t["rot"], t["scl"], False)
        log("set", bone, "deg", pitch_deg.get(bone, 0.0), "swing", swing_scale.get(bone, 1.0), "keys", num_keys, ok)
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

    # 確かめ: 頭と腰の前後の差（0フレーム目、前が正）。
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


def trim(anim, start, count):
    """start から count コマだけを残す。最後のコマ（ループなら先頭と同じ姿勢）を越えたら先頭へ戻る。"""
    P = unreal.AnimPoseExtensions
    L = unreal.AnimPoseSpaces.LOCAL
    opts = eval_options()
    num_keys = unreal.AnimationLibrary.get_num_frames(anim) + 1
    period = num_keys - 1
    src = [(start + i) % period for i in range(count)]
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
    SEGMENT_CHILD["J_Bip_%s_UpperArm" % _s] = "J_Bip_%s_LowerArm" % _s
    SEGMENT_CHILD["J_Bip_%s_LowerArm" % _s] = "J_Bip_%s_Hand" % _s
# 向きを合わせるときに、先に回した親の分を差し引くための親子関係（SEGMENT_CHILD の中だけ）。
SEGMENT_PARENT = {}
for _s in "LR":
    SEGMENT_PARENT["J_Bip_%s_LowerLeg" % _s] = "J_Bip_%s_UpperLeg" % _s
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
    lift_cm を指定すると、同じ割合で腰を持ち上げる（足が地面から離れて、宙に浮く）。
    cfg = {"ramp_start": 3, "lift_cm": 15, "angle_deg": {"J_Bip_L_UpperArm": -60, ...}}
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

    # ramp_start までは 0、最後のコマで 1（smoothstep）。
    weights = []
    for f in range(num_keys):
        u = 1.0 if last <= ramp_start else max(0.0, min(1.0, (f - ramp_start) / float(last - ramp_start)))
        weights.append(u * u * (3.0 - 2.0 * u))

    # 元の値を読む。膝は反らないように抑えるので、両足のすねも読んでおく。
    shins = ["J_Bip_%s_LowerLeg" % s for s in "LR"]
    bones = list(dict.fromkeys(list(targets) + [HIPS] + shins))
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

    end_pose = P.get_anim_pose_at_frame(anim, last, opts)
    before = {b: segment_angle(end_pose, b, fwd) for b in targets}
    pitch = {b: 0.0 for b in targets}  # 最後のコマで足す角度（正 = 前へ倒す）。

    def write():
        c = get_controller(anim)
        c.open_bracket(unreal.Text("pose_to"), False)
        for b in bones:
            o = orig[b]
            rots, poss = [], []
            for f in range(num_keys):
                deg = pitch.get(b, 0.0) * weights[f] + (extra[b][f] if b in extra else 0.0)
                half = math.radians(deg) * 0.5
                s = math.sin(half)
                a = o["axis"][f]
                rots.append(q_mul(unreal.Quat(a.x * s, a.y * s, a.z * s, math.cos(half)), o["rot"][f]))
                p = o["pos"][f]
                if b == HIPS and lift != 0.0:
                    p = p + q_rotate(q_inv(o["parent"][f]), unreal.Vector(0.0, 0.0, lift * weights[f]))
                poss.append(p)
            c.set_bone_track_keys(b, poss, rots, o["scl"], False)
        c.close_bracket(False)

    for it in range(3):
        end_pose = P.get_anim_pose_at_frame(anim, last, opts)
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

    end_pose = P.get_anim_pose_at_frame(anim, last, opts)
    for b in targets:
        log("pose_to", b, "before", round(before[b], 1), "target", targets[b],
            "after", round(segment_angle(end_pose, b, fwd), 1), "pitch", round(pitch[b], 1))
    unreal.EditorAssetLibrary.save_loaded_asset(anim)
    toe = min(P.get_bone_pose(end_pose, "J_Bip_%s_ToeBase" % s, W).translation.z for s in "LR")
    log("pose_to lift", lift, "lowest toe at end(cm)", round(toe, 1))


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
            adjust(anim, cfg["pitch_deg"], cfg.get("swing_scale", {}), cfg.get("keep_feet_on_ground", False))
            frames = clip.get("frames")
            if frames:
                trim(anim, frames["start"], frames["count"])
            if clip.get("pose_to"):
                pose_to(anim, clip["pose_to"])
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
