# 黒騎士のアニメーション（Idle・Walk・Attack）を作って FBX に書き出す。
#
# 使い方:
#   blender -b --python make_black_knight_anims.py -- <BlackKnight.fbx> <出力フォルダ> [--preview <プレビューのフォルダ>]
#
# BlackKnight.fbx（make_black_knight.py で作ったもの）の骨格にポーズを付けて、
# 出力フォルダに Idle.fbx・Walk.fbx・Attack.fbx を書き出す。
# 動きは西洋の騎士らしく、背筋を伸ばして厳かに（待機は剣を地面に突いて柄頭に手を重ね、攻撃は両手で縦に振り下ろす）。
# --preview を付けると、各アニメーションのコマを並べた画像（<名前>.png）も書き出す。
#
# ポーズの付け方
#   - 胴・首・頭は「骨の初期の向きのまま、ワールドの軸で何度回すか」で指定する（rot）。
#     親が回っていれば、その回転に乗ったまま回る。
#   - 足は足首の位置・つま先の向き・膝の向きを指定し、IK で太もも・すねの向きを決める（legs）。
#   - 腕は手首の位置・肘の向きを指定し、IK で上腕・前腕の向きを決める（arms）。
#     右手は剣の刃の向きも指定する。左手は ("grip", 柄の上の位置, 肘の向き) で剣の柄に添えられる。
#   - 座標はワールド（m）で、前は -Y、上は +Z、キャラクターの左は +X。
#   - キーにしたポーズの間は、時間つきの Catmull-Rom で毎フレーム補間してキーを打つ（30fps）。
import bpy
import sys
import os
import math
from mathutils import Vector, Quaternion, Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
SRC_FBX, OUT_DIR = argv[0], argv[1]
PREVIEW_DIR = argv[argv.index("--preview") + 1] if "--preview" in argv else None
FPS = 30

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC_FBX)
ARM = next(o for o in bpy.data.objects if o.type == "ARMATURE")
SCENE = bpy.context.scene
SCENE.render.fps = FPS
SCENE.render.fps_base = 1.0

M_WORLD = ARM.matrix_world.copy()
M_INV = M_WORLD.inverted()
Q_ARM = M_WORLD.to_quaternion()       # 骨格のオブジェクトの回転（ワールド → 骨格の空間の変換に使う）
Q_ARM_INV = Q_ARM.inverted()


def C(name):
    return f"J_Bip_C_{name}"


def S(side, name):
    return f"J_Bip_{side}_{name}"


BONES = ARM.data.bones
REST_Q = {b.name: b.matrix_local.to_quaternion() for b in BONES}   # 骨格の空間での初期の向き
REST_H = {b.name: b.head_local.copy() for b in BONES}              # 骨格の空間での付け根の位置


def depth(b):
    n = 0
    while b.parent:
        b = b.parent
        n += 1
    return n


ORDER = sorted([b.name for b in BONES], key=lambda n: depth(BONES[n]))


def w2a_dir(v):
    return Q_ARM_INV @ Vector(v)


def w2a_pos(p):
    return M_INV @ Vector(p)


def a2w_pos(p):
    return M_WORLD @ p


def world_rot(seq):
    """[(軸, 度), ...] を先頭から順にワールドの軸で回す回転。"""
    q = Quaternion()
    for axis, deg in seq:
        ax = {"X": Vector((1, 0, 0)), "Y": Vector((0, 1, 0)), "Z": Vector((0, 0, 1))}[axis]
        q = Quaternion(ax, math.radians(deg)) @ q
    return Q_ARM_INV @ q @ Q_ARM


def frame3(a, b):
    x = a.normalized()
    y = (b - x * b.dot(x)).normalized()
    z = x.cross(y)
    return Matrix((x, y, z)).transposed()


def align(c, cs, t, ts):
    """向き c（と副の向き cs）を t（と ts）に重ねる回転。"""
    return (frame3(t, ts) @ frame3(c, cs).transposed()).to_quaternion()


def two_bone_ik(a, b0, c0, target, pole):
    """付け根 a から target へ届く2本の骨の、関節の位置を返す（関節は pole の側へ曲がる）。"""
    l1 = (b0 - a).length
    l2 = (c0 - b0).length
    d = target - a
    dist = min(max(d.length, 1e-4), (l1 + l2) * 0.9995)
    u = d.normalized()
    cos_a = (l1 * l1 + dist * dist - l2 * l2) / (2 * l1 * dist)
    cos_a = max(-1.0, min(1.0, cos_a))
    pp = (pole - u * pole.dot(u)).normalized()
    elbow = a + (u * cos_a + pp * math.sqrt(1 - cos_a * cos_a)) * l1
    return elbow, a + u * dist


# 剣の刃の、初期の（T ポーズの）向き。make_black_knight.py の build_sword と同じ。
_a = math.radians(20)   # make_black_knight.py の SWORD_ANGLE と合わせる
BLADE_D0 = Vector((-math.sin(_a), -math.cos(_a), 0)).normalized()
BLADE_W0 = (Vector((-1, 0, 0)) - BLADE_D0 * Vector((-1, 0, 0)).dot(BLADE_D0)).normalized()

LIMB_IK_BONES = {S(s, n) for s in "LR" for n in ("UpperLeg", "LowerLeg", "Foot", "UpperArm", "LowerArm", "Hand")}


def _compute_pose(pose):
    """ポーズの指定から、骨ごとの回転（matrix_basis の回転）と腰の移動（骨の空間）を求める。"""
    D = {}       # 骨格の空間での、初期の向きからの回転
    P = {}       # 骨格の空間での、付け根の位置
    basis = {}
    ik = {}      # 骨 → (向き, 副の向き)。IK で求めた太もも・すね・上腕・前腕の向き
    rot = pose.get("rot", {})
    loc_w = Vector(pose.get("loc", (0, 0, 0)))
    loc_a = M_INV.to_3x3() @ loc_w
    for name in ORDER:
        b = BONES[name]
        Bq = REST_Q[name]
        if b.parent is None:
            Dp = Quaternion()
            P[name] = REST_H[name] + (loc_a if name == C("Hips") else Vector())
        else:
            pn = b.parent.name
            Dp = D[pn]
            P[name] = P[pn] + Dp @ (REST_H[name] - REST_H[pn])

        def set_rot(R):
            basis[name] = Bq.inverted() @ R @ Bq
            D[name] = Dp @ R

        def set_aim(d0, s0, t, ts):
            c = Dp @ d0
            cs = Dp @ s0
            delta = align(c, cs, t, ts)
            basis[name] = Bq.inverted() @ Dp.inverted() @ delta @ Dp @ Bq
            D[name] = delta @ Dp

        side = name[6] if name.startswith("J_Bip_") and name[6] in "LR" else None
        short = name[8:] if side else None

        if side and short == "UpperLeg" and side in pose.get("legs", {}):
            ankle, yaw, pitch, pole = pose["legs"][side]
            knee_b = S(side, "LowerLeg")
            foot_b = S(side, "Foot")
            hip = P[name]
            knee0 = hip + Dp @ (REST_H[knee_b] - REST_H[name])
            ankle0 = knee0 + Dp @ (REST_H[foot_b] - REST_H[knee_b])
            pole_a = w2a_dir(pole)
            knee, ankle_reached = two_bone_ik(hip, knee0, ankle0, w2a_pos(ankle), pole_a)
            for bn, t in ((name, knee - hip), (knee_b, ankle_reached - knee)):
                ik[bn] = (t, pole_a - t.normalized() * pole_a.dot(t.normalized()))
            toe0 = REST_H[S(side, "ToeBase")] - REST_H[foot_b]
            rz = Quaternion(w2a_dir((0, 0, 1)), math.radians(yaw))
            rx = Quaternion(w2a_dir((1, 0, 0)), math.radians(pitch))
            ik[foot_b] = (rz @ rx @ toe0, rz @ w2a_dir((1, 0, 0)), toe0, w2a_dir((1, 0, 0)))
        if side and short == "UpperArm" and side in pose.get("arms", {}):
            spec = pose["arms"][side]
            wrist, pole = spec[0], spec[1]
            elbow_b = S(side, "LowerArm")
            hand_b = S(side, "Hand")
            sh = P[name]
            elbow0 = sh + Dp @ (REST_H[elbow_b] - REST_H[name])
            wrist0 = elbow0 + Dp @ (REST_H[hand_b] - REST_H[elbow_b])
            pole_a = w2a_dir(pole)
            elbow, wrist_reached = two_bone_ik(sh, elbow0, wrist0, w2a_pos(wrist), pole_a)
            for bn, t in ((name, elbow - sh), (elbow_b, wrist_reached - elbow)):
                ik[bn] = (t, pole_a - t.normalized() * pole_a.dot(t.normalized()))
            if len(spec) > 2 and spec[2] is not None:
                fore = (wrist_reached - elbow).normalized()
                ik[hand_b] = (w2a_dir(spec[2]).normalized(), fore, w2a_dir(BLADE_D0), w2a_dir(BLADE_W0))

        if name in ik and name in LIMB_IK_BONES and short in ("UpperLeg", "LowerLeg", "UpperArm", "LowerArm"):
            t, ts = ik[name]
            child = {"UpperLeg": "LowerLeg", "LowerLeg": "Foot", "UpperArm": "LowerArm", "LowerArm": "Hand"}[short]
            d0 = REST_H[S(side, child)] - REST_H[name]
            # 膝は前（-Y）、肘は後ろ（+Y）を向いているのが初期の向き。
            s0 = w2a_dir((0, -1, 0)) if "Leg" in short else w2a_dir((0, 1, 0))
            set_aim(d0, s0, t, ts)
        elif name in ik and short in ("Foot", "Hand"):
            t, ts, d0, s0 = ik[name]
            set_aim(d0, s0, t, ts)
        elif name in rot:
            set_rot(world_rot(rot[name]))
        else:
            basis[name] = Quaternion()
            D[name] = Dp
    hips_loc = REST_Q[C("Hips")].inverted() @ loc_a
    hand = S("R", "Hand")
    fist = P[hand] + D[hand] @ (w2a_pos(FIST_R) - REST_H[hand])

    def sword_point(sd):
        """柄・刃の上の点（拳の中心から刃の向きに sd m）のワールド座標。"""
        return a2w_pos(fist + D[hand] @ (w2a_dir(BLADE_D0) * (sd / M_WORLD.to_scale()[0])))

    shoulder_l = a2w_pos(P[S("L", "UpperArm")])
    return basis, hips_loc, sword_point, shoulder_l


FIST_R = (-.610, .012, 1.272)   # 右の拳の中心（T ポーズ）。make_black_knight.py と同じ。


def compute_pose(pose):
    """左手を剣の柄に添えるポーズ（arms の L が ("grip", 柄の上の位置, 肘の向き)）は、
    右手と剣の位置を先に求めてから、左の手首の位置を決めてもう一度求める。"""
    left = pose.get("arms", {}).get("L")
    if left is not None and left[0] == "grip":
        first = dict(pose)
        first["arms"] = {k: v for k, v in pose["arms"].items() if k != "L"}
        _, _, sword_point, shoulder_l = _compute_pose(first)
        gp = sword_point(left[1])
        wrist = gp + (shoulder_l - gp).normalized() * 0.065   # 拳の中心が柄に来るよう、手首は少し手前
        second = dict(pose)
        second["arms"] = dict(pose["arms"])
        second["arms"]["L"] = (tuple(wrist), left[2])
        pose = second
    basis, hips_loc, sword_point, _ = _compute_pose(pose)
    return basis, hips_loc, sword_point(1.16)


# ---------------------------------------------------------------------------
# 補間（時間つきの Catmull-Rom。ループなら端もつなぐ）
# ---------------------------------------------------------------------------
def hermite(p0, p1, m0, m1, s, dt):
    s2, s3 = s * s, s * s * s
    h00 = 2 * s3 - 3 * s2 + 1
    h10 = s3 - 2 * s2 + s
    h01 = -2 * s3 + 3 * s2
    h11 = s3 - s2
    return [h00 * a + h10 * dt * ma + h01 * b + h11 * dt * mb for a, b, ma, mb in zip(p0, p1, m0, m1)]


def sample_track(times, values, t, loop, length):
    """times と values（数の並び）から、時刻 t の値を求める。"""
    n = len(times)
    if loop:
        ext_t = [times[-2] - length] + times + [times[1] + length]
        ext_v = [values[-2]] + values + [values[1]]
    else:
        ext_t = [times[0] - 1] + times + [times[-1] + 1]
        ext_v = [values[0]] + values + [values[-1]]
    for i in range(1, n):
        if times[i - 1] <= t <= times[i]:
            k = i  # ext の添字では k..k+1
            break
    else:
        return values[-1]
    t0, t1 = ext_t[k], ext_t[k + 1]
    p0, p1 = ext_v[k], ext_v[k + 1]

    def tangent(j):
        if not loop and (j == 1 or j == n):
            return [0.0] * len(p0)
        return [(a - b) / (ext_t[j + 1] - ext_t[j - 1]) for a, b in zip(ext_v[j + 1], ext_v[j - 1])]

    dt = t1 - t0
    return hermite(p0, p1, tangent(k), tangent(k + 1), (t - t0) / dt, dt)


def bake(name, keys, length, loop):
    """keys: [(コマ, ポーズ), ...]。length コマのアクションを作って骨格に付ける。"""
    computed = [(f,) + compute_pose(p) for f, p in keys]
    times = [float(f) for f, *_ in computed]
    bone_names = [n for n in ORDER if any(c[1][n].angle > 1e-5 for c in computed)]
    tracks = {}
    for n in bone_names:
        vals = []
        prev = None
        for c in computed:
            q = c[1][n].copy()
            if prev is not None and q.dot(prev) < 0:
                q.negate()
            vals.append(list(q))
            prev = q
        tracks[n] = vals
    hip_vals = [list(c[2]) for c in computed]

    ARM.animation_data_create()
    ARM.animation_data.action = None
    for pb in ARM.pose.bones:
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = Quaternion()
        pb.location = Vector()
    for f in range(0, length + 1):
        for n in bone_names:
            pb = ARM.pose.bones[n]
            pb.rotation_quaternion = Quaternion(sample_track(times, tracks[n], f, loop, length)).normalized()
            pb.keyframe_insert("rotation_quaternion", frame=f)
        hb = ARM.pose.bones[C("Hips")]
        hb.location = Vector(sample_track(times, hip_vals, f, loop, length))
        hb.keyframe_insert("location", frame=f)
    act = ARM.animation_data.action
    act.name = name
    act.use_fake_user = True
    for i, c in enumerate(computed):
        print(f"[{name}] key {c[0]:>3}: sword tip {tuple(round(x, 3) for x in c[3])}")
    return act


# ---------------------------------------------------------------------------
# ポーズ
# ---------------------------------------------------------------------------
def lerp3(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def stand(breath=0.0):
    """待機。背筋を伸ばして立ち、切っ先を前の地面に突いた剣の柄頭に、左手を重ねる。"""
    return {
        "loc": (0.0, 0.0, -0.012 - 0.004 * breath),
        "rot": {
            C("Spine"): [("X", -2)],
            C("Chest"): [("X", -2 + 0.6 * breath)],
            C("UpperChest"): [("X", -1 + 0.8 * breath)],
            C("Neck"): [("X", 2)],
            C("Head"): [("X", 4 - 0.6 * breath)],
        },
        "legs": {
            "L": ((0.115, -0.010, 0.109), 8, 0, (0.15, -1, 0)),
            "R": ((-0.115, -0.010, 0.109), -8, 0, (-0.15, -1, 0)),
        },
        "arms": {
            "R": ((-0.035, -0.235, 0.985), (-0.9, 0.4, -0.2), (0.0, -0.58, -0.81)),
            "L": ("grip", -0.15, (0.9, 0.4, -0.2)),
        },
    }


WALK_LEN = 36      # 1.2秒で2歩
WALK_STRIDE = 0.27  # 足首が前後に動く幅の半分（m）


def walk_pose(phase):
    """歩き（phase: 0〜1 で1周。0 で右足が前に着く）。背筋を伸ばしたまま、ゆっくり大きく歩く。
    剣は右手に下げ、切っ先を前下へ向ける。"""
    def foot(ph):
        ph %= 1.0
        if ph < 0.6:   # 着いている間: 前から後ろへ一定の速さで動く
            t = ph / 0.6
            return -WALK_STRIDE + 2 * WALK_STRIDE * t, 0.109, 0.0
        t = (ph - 0.6) / 0.4   # 浮いている間: 後ろから前へ
        s = t * t * (3 - 2 * t)
        y = WALK_STRIDE - 2 * WALK_STRIDE * s
        z = 0.109 + 0.085 * math.sin(math.pi * t)
        pitch = 28 * math.sin(math.pi * t) * (1 - t) - 10 * t * t   # 蹴り出しでつま先が下がり、着く前に上がる
        return y, z, pitch

    ry, rz, ra = foot(phase)
    ly, lz, la = foot(phase + 0.5)
    s = math.cos(2 * math.pi * phase)               # 1: 右足が前、-1: 左足が前
    bob = math.cos(4 * math.pi * (phase - 0.3))     # 片足で立った真ん中で高く、着いたときに低い
    sway = math.sin(2 * math.pi * phase)            # 体重を乗せている足の方へ少し寄る
    return {
        "loc": (-0.014 * sway, 0.0, -0.045 + 0.010 * bob),
        "rot": {
            C("Hips"): [("Z", 6 * s)],
            C("Spine"): [("X", -1), ("Z", -2 * s)],
            C("Chest"): [("X", -2), ("Z", -2 * s)],
            C("UpperChest"): [("Z", -1 * s)],
            C("Neck"): [("X", 1)],
            C("Head"): [("X", 2)],
        },
        "legs": {
            "R": ((-0.090, ry, rz), -4, ra, (-0.1, -1, 0)),
            "L": ((0.090, ly, lz), 4, la, (0.1, -1, 0)),
        },
        "arms": {
            "R": ((-0.225, -0.060 + 0.015 * s, 0.905 + 0.006 * bob), (-0.7, 0.7, 0.0), (-0.20, -0.78, -0.60)),
            # 左腕は左足と逆に、小さく振る（右足が前のとき左腕が前）。
            "L": (lerp3((0.200, 0.090, 0.925), (0.190, -0.150, 0.960), 0.5 + 0.5 * s), (0.4, 0.9, 0.0)),
        },
    }


def attack_pose(wrist, blade, chest_x, hips_loc, l_foot, head_x=0.0):
    """両手で握った剣を縦に振るときのポーズ。右足は後ろ、左足を前に踏み込む。"""
    return {
        "loc": hips_loc,
        "rot": {
            C("Spine"): [("X", chest_x * 0.4)],
            C("Chest"): [("X", chest_x * 0.35)],
            C("UpperChest"): [("X", chest_x * 0.25)],
            C("Head"): [("X", head_x)],
        },
        "legs": {
            "L": (l_foot, 4, 0, (0.15, -1, 0)),
            "R": ((-0.125, 0.150, 0.109), -18, 0, (-0.3, -1, 0)),
        },
        "arms": {
            "R": (wrist, (-1.0, 0.2, -0.2), blade),
            "L": ("grip", -0.09, (1.0, 0.2, -0.2)),
        },
    }


def attack_windup():
    # 頭の上へ振りかぶる（切っ先は後ろ上）。少し胸を張る。
    return attack_pose((-0.040, 0.060, 1.660), (0.0, 0.62, 0.78), -6, (0.0, 0.035, -0.035),
                       (0.115, -0.120, 0.109), head_x=-2)


def attack_top():
    # 振り下ろし始め（切っ先は真上から前へ倒れる）。左足を上げて踏み出す。
    return attack_pose((-0.040, -0.120, 1.700), (0.0, -0.35, 0.94), 0, (0.0, -0.010, -0.050),
                       (0.120, -0.210, 0.170))


def attack_mid():
    # 体の前を真っすぐ振り下ろしている途中（刃は前へ水平）。
    return attack_pose((-0.030, -0.420, 1.380), (0.0, -0.97, 0.24), 8, (0.0, -0.060, -0.075),
                       (0.120, -0.290, 0.109), head_x=-2)


def attack_end():
    # 振り下ろした（切っ先は前の地面の少し上）。
    return attack_pose((-0.030, -0.420, 0.930), (0.0, -0.72, -0.69), 15, (0.0, -0.090, -0.105),
                       (0.120, -0.300, 0.109), head_x=-6)


ANIMS = []
# Idle: 3秒で1回、ゆっくり息をする。
ANIMS.append(("Idle", [(0, stand(0)), (45, stand(1)), (90, stand(0))], 90, True))
# Walk: 1.2秒（36コマ）で2歩。
ANIMS.append(("Walk", [(f, walk_pose(f / WALK_LEN)) for f in range(0, WALK_LEN + 1, 3)], WALK_LEN, True))
# Attack: 0.3秒で振りかぶり、少しためて、0.23秒で縦に振り下ろす。構えに戻るまで 1.2秒。
ANIMS.append(("Attack", [
    (0, stand(0)),
    (9, attack_windup()),
    (12, attack_windup()),
    (14, attack_top()),
    (16, attack_mid()),
    (19, attack_end()),
    (25, attack_end()),
    (36, stand(0)),
], 36, False))


# ---------------------------------------------------------------------------
# 書き出し
# ---------------------------------------------------------------------------
os.makedirs(OUT_DIR, exist_ok=True)
actions = {}
for name, keys, length, loop in ANIMS:
    act = bake(name, keys, length, loop)
    actions[name] = (act, length)
    SCENE.frame_start, SCENE.frame_end = 0, length
    for o in bpy.data.objects:
        o.select_set(o == ARM)
    bpy.context.view_layer.objects.active = ARM
    out = os.path.join(OUT_DIR, name + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=out, use_selection=True, object_types={"ARMATURE"},
        axis_forward="Y", axis_up="Z", add_leaf_bones=False,
        primary_bone_axis="Y", secondary_bone_axis="X",
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", use_armature_deform_only=False,
        bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
        bake_anim_force_startend_keying=True, bake_anim_step=1.0, bake_anim_simplify_factor=0.0,
        path_mode="STRIP")
    print("EXPORTED", out)


# ---------------------------------------------------------------------------
# プレビュー（確かめ用）
# ---------------------------------------------------------------------------
def render_previews():
    import numpy as np
    mesh = next(o for o in bpy.data.objects if o.type == "MESH")
    d = os.path.dirname(SRC_FBX)
    mat = bpy.data.materials.new("prev")
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    ta = nt.nodes.new("ShaderNodeTexImage")
    ta.image = bpy.data.images.load(os.path.join(d, "BlackKnight_albedo.png"))
    nt.links.new(ta.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Metallic"].default_value = 0.6
    bsdf.inputs["Roughness"].default_value = 0.4
    mesh.data.materials.clear()
    mesh.data.materials.append(mat)
    try:
        SCENE.render.engine = "BLENDER_EEVEE"
    except TypeError:
        SCENE.render.engine = "BLENDER_EEVEE_NEXT"
    SCENE.render.resolution_x, SCENE.render.resolution_y = 300, 400
    SCENE.world = bpy.data.worlds.new("w")
    SCENE.world.use_nodes = True
    bg = SCENE.world.node_tree.nodes["Background"]
    bg.inputs[0].default_value = (0.55, 0.6, 0.7, 1)
    bg.inputs[1].default_value = 0.9
    SCENE.view_settings.view_transform = "Standard"
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sun.data.energy = 3.5
    sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
    SCENE.collection.objects.link(sun)
    bpy.ops.mesh.primitive_plane_add(size=6)
    fl = bpy.context.active_object
    fm = bpy.data.materials.new("floor")
    fm.use_nodes = True
    fm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.35, 0.42, 0.3, 1)
    fl.data.materials.append(fm)
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    SCENE.collection.objects.link(cam)
    SCENE.camera = cam
    cam.data.lens = 45
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    plan = {
        "Idle": ([0, 45], [25, 90]),
        "Walk": ([0, 6, 12, 18, 24, 30], [25, 90]),
        "Attack": ([0, 9, 13, 14, 15, 16, 17, 19, 25, 31], [25, 90]),
    }
    for name, (act, length) in actions.items():
        ARM.animation_data.action = act
        if hasattr(ARM.animation_data, "action_slot") and act.slots:
            ARM.animation_data.action_slot = act.slots[0]
        frames, angles = plan[name]
        rows = []
        for ang in angles:
            tiles = []
            for f in frames:
                SCENE.frame_set(f)
                a = math.radians(ang)
                tgt = Vector((0, 0, 0.95))
                cam.location = tgt + Vector((math.sin(a) * 3.6, -math.cos(a) * 3.6, 0.5))
                cam.rotation_euler = (tgt - cam.location).to_track_quat("-Z", "Y").to_euler()
                p = os.path.join(PREVIEW_DIR, "_tile.png")
                SCENE.render.filepath = p
                bpy.ops.render.render(write_still=True)
                im = bpy.data.images.load(p)
                tiles.append(np.array(im.pixels[:]).reshape(im.size[1], im.size[0], 4))
                bpy.data.images.remove(im)
            rows.append(np.concatenate(tiles, axis=1))
        full = np.concatenate(rows[::-1], axis=0)
        img = bpy.data.images.new("sheet_" + name, full.shape[1], full.shape[0])
        img.pixels.foreach_set(full.astype(np.float32).ravel())
        img.filepath_raw = os.path.join(PREVIEW_DIR, name + ".png")
        img.file_format = "PNG"
        img.save()
        print("PREVIEW", img.filepath_raw)


if PREVIEW_DIR:
    render_previews()
