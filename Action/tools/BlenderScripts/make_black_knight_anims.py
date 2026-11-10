# 黒騎士のアニメーション（30fps）を作って FBX に書き出す。
#
# 使い方（Blender 5.1 で確認）:
#   blender -b --python make_black_knight_anims.py -- <PoseRig.fbx> <BlackKnight.fbx> <出力フォルダ> [--preview <プレビューのフォルダ>]
#
#   PoseRig.fbx      ポーズを計算する骨格（BlackKnightSource/PoseRig.fbx。SK_Player の比率）
#   BlackKnight.fbx  書き出す骨格とメッシュ（make_black_knight_meshy.py の出力。Meshy の体に合わせた比率）
#
# 動きのテーマは「静から動へ一瞬で切り替わる、低く素早い暗殺者の動き」。
# どの攻撃も「予備動作（短い）→ 攻撃（非常に速い）→ 残心（やや長め）」の3段で作る。
#
#   Idle       重心を低くし、剣は逆手気味に後ろへ下げて構える。呼吸ほどの小さな揺れだけ（ループ）
#   Walk       低い姿勢で忍び寄る歩き（ループ。ゲームの移動用）
#   DashSlash  低い姿勢から一気に前へ踏み込み、すれ違いざまに横一閃（攻撃は5コマ）
#   SpinCombo  3連撃。2撃目で体を1回転させ、垂れ布を大きく広げる
#   LeapSlam   高く跳び、落ちると同時に剣を地面へ突き立てる。着地のあと一瞬止まる
#   Backstep   後ろへ大きく跳んで距離を取り、すぐ構え直す
#   FeintDash  横へ素早く動いて軌道を変え、そのまま斬り込む
#
# 書き出し
#   <出力フォルダ>/RootMotion/<名前>.fbx   ルートモーションあり（腰の骨が移動の分も動く）。上の7つ全部
#   <出力フォルダ>/<名前>.fbx              その場で再生する版（腰の水平の移動を抜く）。ゲームで使う Idle・Walk・DashSlash
#   --preview を付けると、キーポーズを <プレビュー>/<名前>/<名前>_<コマ>.png に連番で書き、並べた1枚 <名前>.png も書く。
#
# ポーズの付け方
#   - 座標はワールド（m）で、前は -Y、上は +Z、キャラクターの左は +X。
#   - root（地面の上の体の位置）と yaw（体ごと回す向き、度。+ で左回り）を付けると、
#     足首・手首・肘の向き・剣の向きは、その位置と向きから見た座標として扱う。
#   - 胴・首・頭は「骨の初期の向きのまま、ワールドの軸で何度回すか」で指定する（rot。+X で前へ倒れる、+Z で左を向く）。
#   - 足は足首の位置・つま先の向き・つま先の下がり・膝の向きを指定し、IK で太もも・すねの向きを決める（legs）。
#   - 腕は手首の位置・肘の向き・（右手は）剣の向きを指定する（arms）。左手は ("grip", 柄の上の位置, 肘の向き) で柄に添える。
#   - キーにしたポーズの間は、時間つきの Catmull-Rom で毎フレーム補間してキーを打つ。
#   - 垂れ布とフードの骨は、腰・頭の動き（速さ・回る速さ・落ちる速さ）から、ばねで遅れて揺れるように後から付ける。
#     脚や背中に入り込まないよう、太もも・すね・胸の傾きに合わせて押し出す。
#   - 刀身の発光の強さ（GLOW）は、ゲームのエンジンに発光がないので、プレビューの描画にだけ使う。
#     プレビューは、BlackKnight.fbx の横にある、マテリアルごとの albedo・normal・emission（発光のマスク）の画像を使う。
#   - ポーズは PoseRig の上で計算し、骨の世界での向きがそろうように BlackKnight.fbx の骨格へ移して書き出す。
#     腰の移動は、脚の長さの比（LEG_RATIO）で伸び縮みさせる。
import bpy
import sys
import os
import math
from mathutils import Vector, Quaternion, Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
RIG_FBX, SRC_FBX, OUT_DIR = (os.path.abspath(p) for p in argv[:3])
PREVIEW_DIR = os.path.abspath(argv[argv.index("--preview") + 1]) if "--preview" in argv else None
FPS = 30

bpy.ops.wm.read_factory_settings(use_empty=True)
# ポーズを計算する骨格（PoseRig。SK_Player の比率）。剣のメッシュは使わない
bpy.ops.import_scene.fbx(filepath=RIG_FBX)
ARM = next(o for o in bpy.data.objects if o.type == "ARMATURE")
for o in [o for o in bpy.data.objects if o.type == "MESH"]:
    bpy.data.objects.remove(o, do_unlink=True)
# 書き出す骨格（BlackKnight.fbx。Meshy の体に合わせた比率）
_before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=SRC_FBX)
TGT = next(o for o in bpy.data.objects if o not in _before and o.type == "ARMATURE")
SCENE = bpy.context.scene
SCENE.render.fps = FPS
SCENE.render.fps_base = 1.0

M_WORLD = ARM.matrix_world.copy()
M_INV = M_WORLD.inverted()
Q_ARM = M_WORLD.to_quaternion()
Q_ARM_INV = Q_ARM.inverted()
UNIT = M_WORLD.to_scale()[0]


def C(name):
    return f"J_Bip_C_{name}"


def S(side, name):
    return f"J_Bip_{side}_{name}"


BONES = ARM.data.bones
REST_Q = {b.name: b.matrix_local.to_quaternion() for b in BONES}
REST_H = {b.name: b.head_local.copy() for b in BONES}
REST_DIR = {b.name: (b.tail_local - b.head_local).normalized() for b in BONES}


def depth(b):
    n = 0
    while b.parent:
        b = b.parent
        n += 1
    return n


ORDER = sorted([b.name for b in BONES], key=lambda n: depth(BONES[n]))
PARENT = {b.name: (b.parent.name if b.parent else None) for b in BONES}

# 書き出す骨格へ移すための値。骨の世界での向きを PoseRig とそろえると、
# 骨ごとの回転は D⁻¹·（PoseRig の回転）·D になる（D は PoseRig から見た、書き出す骨格の初期の向きの差）
TGT_REST_Q = {b.name: b.matrix_local.to_quaternion() for b in TGT.data.bones}
REST_D = {n: REST_Q[n].inverted() @ TGT_REST_Q[n] for n in REST_Q if n in TGT_REST_Q}


def retarget(name, q):
    d = REST_D[name]
    return (d.inverted() @ q @ d).normalized()


def _leg_len(bones):
    return bones[S("L", "UpperLeg")].head_local.z - bones[S("L", "Foot")].head_local.z


LEG_RATIO = _leg_len(TGT.data.bones) / _leg_len(BONES)
print("LEG_RATIO", round(LEG_RATIO, 4))


def w2a_dir(v):
    return Q_ARM_INV @ Vector(v)


def a2w_dir(v):
    return Q_ARM @ Vector(v)


def w2a_pos(p):
    return M_INV @ Vector(p)


def a2w_pos(p):
    return M_WORLD @ p


def world_rot(seq):
    """[(軸, 度), ...] を先頭から順にワールドの軸で回す回転（骨格の空間で返す）。"""
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


# 剣の初期の（T ポーズの）向き。make_black_knight.py の sword_axes と同じ。
SWORD_ANGLE = math.radians(20)
BLADE_D0 = Vector((-math.sin(SWORD_ANGLE), -math.cos(SWORD_ANGLE), 0)).normalized()
BLADE_E0 = Vector((0, 0, -1))   # 刃のある側
FIST_R = (-.610, .012, 1.272)
SWORD_TIP = .70

LIMB_IK_BONES = {S(s, n) for s in "LR" for n in ("UpperLeg", "LowerLeg", "Foot", "UpperArm", "LowerArm", "Hand")}


def place(p):
    """体の位置（root）と向き（yaw）を付けたポーズを、ワールドの指定に直す。"""
    root = Vector(p.get("root", (0, 0, 0)))
    yaw = p.get("yaw", 0.0)
    Rz = Matrix.Rotation(math.radians(yaw), 3, "Z")

    def T(v):
        return tuple(root + Rz @ Vector(v))

    def Rd(v):
        return tuple(Rz @ Vector(v))

    q = {"loc": T(p.get("loc", (0, 0, 0))), "root": tuple(root)}
    rot = {k: list(v) for k, v in p.get("rot", {}).items()}
    if yaw:
        rot.setdefault(C("Hips"), []).append(("Z", yaw))
    q["rot"] = rot
    q["legs"] = {s: (T(a), fy + yaw, pitch, Rd(pole)) for s, (a, fy, pitch, pole) in p.get("legs", {}).items()}
    arms = {}
    for s, spec in p.get("arms", {}).items():
        if spec[0] == "grip":
            arms[s] = ("grip", spec[1], Rd(spec[2]))
        else:
            arms[s] = (T(spec[0]), Rd(spec[1])) + ((Rd(spec[2]),) if len(spec) > 2 and spec[2] is not None else ())
    q["arms"] = arms
    return q


def _compute_pose(pose):
    """ワールドの指定から、骨ごとの回転（matrix_basis の回転）を求める。"""
    D, P, basis, ik = {}, {}, {}, {}
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
            knee_b, foot_b = S(side, "LowerLeg"), S(side, "Foot")
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
            elbow_b, hand_b = S(side, "LowerArm"), S(side, "Hand")
            sh = P[name]
            elbow0 = sh + Dp @ (REST_H[elbow_b] - REST_H[name])
            wrist0 = elbow0 + Dp @ (REST_H[hand_b] - REST_H[elbow_b])
            pole_a = w2a_dir(pole)
            elbow, wrist_reached = two_bone_ik(sh, elbow0, wrist0, w2a_pos(wrist), pole_a)
            for bn, t in ((name, elbow - sh), (elbow_b, wrist_reached - elbow)):
                ik[bn] = (t, pole_a - t.normalized() * pole_a.dot(t.normalized()))
            if len(spec) > 2 and spec[2] is not None:
                fore = (wrist_reached - elbow).normalized()
                # 刃の向きを合わせ、刃のある側はなるべく前腕と逆（拳の小指側）へ向ける
                ik[hand_b] = (w2a_dir(spec[2]).normalized(), -fore, w2a_dir(BLADE_D0), w2a_dir(BLADE_E0))

        if name in ik and name in LIMB_IK_BONES and short in ("UpperLeg", "LowerLeg", "UpperArm", "LowerArm"):
            t, ts = ik[name]
            child = {"UpperLeg": "LowerLeg", "LowerLeg": "Foot", "UpperArm": "LowerArm", "LowerArm": "Hand"}[short]
            d0 = REST_H[S(side, child)] - REST_H[name]
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
    hand = S("R", "Hand")
    fist = P[hand] + D[hand] @ (w2a_pos(FIST_R) - REST_H[hand])

    def sword_point(sd):
        return a2w_pos(fist + D[hand] @ (w2a_dir(BLADE_D0) * (sd / UNIT)))

    shoulder_l = a2w_pos(P[S("L", "UpperArm")])
    return basis, loc_w, sword_point, shoulder_l


def compute_pose(local_pose):
    pose = place(local_pose)
    left = pose.get("arms", {}).get("L")
    if left is not None and left[0] == "grip":
        first = dict(pose)
        first["arms"] = {k: v for k, v in pose["arms"].items() if k != "L"}
        _, _, sword_point, shoulder_l = _compute_pose(first)
        gp = sword_point(left[1])
        wrist = gp + (shoulder_l - gp).normalized() * 0.065
        second = dict(pose)
        second["arms"] = dict(pose["arms"])
        second["arms"]["L"] = (tuple(wrist), left[2])
        pose = second
    basis, loc_w, sword_point, _ = _compute_pose(pose)
    return basis, loc_w, Vector(pose["root"]), sword_point(SWORD_TIP)


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
    n = len(times)
    if n == 1:
        return list(values[0])
    if loop:
        ext_t = [times[-2] - length] + times + [times[1] + length]
        ext_v = [values[-2]] + values + [values[1]]
    else:
        ext_t = [times[0] - 1] + times + [times[-1] + 1]
        ext_v = [values[0]] + values + [values[-1]]
    for i in range(1, n):
        if times[i - 1] <= t <= times[i]:
            k = i
            break
    else:
        return list(values[-1])
    t0, t1 = ext_t[k], ext_t[k + 1]
    p0, p1 = ext_v[k], ext_v[k + 1]

    def tangent(j):
        if not loop and (j == 1 or j == n):
            return [0.0] * len(p0)
        return [(a - b) / (ext_t[j + 1] - ext_t[j - 1]) for a, b in zip(ext_v[j + 1], ext_v[j - 1])]

    dt = t1 - t0
    return hermite(p0, p1, tangent(k), tangent(k + 1), (t - t0) / dt, dt)


def sample_linear(points, t):
    """[(コマ, 値), ...] を直線でつないだ値（発光の強さ）。"""
    if t <= points[0][0]:
        return points[0][1]
    for (fa, va), (fb, vb) in zip(points, points[1:]):
        if t <= fb:
            return va + (vb - va) * (t - fa) / (fb - fa)
    return points[-1][1]


# ---------------------------------------------------------------------------
# 垂れ布とフードの揺れ
# ---------------------------------------------------------------------------
# 鎖: (骨の頭の名前, 付け根の位置（腰・頭から見た水平の位置）, 向きを決める骨, 種類)
CHAINS = [
    ("J_Sec_ClothF", Vector((0.0, -0.10, 0)), C("Hips"), "front"),
    ("J_Sec_ClothL", Vector((0.07, 0.105, 0)), C("Hips"), "backL"),
    ("J_Sec_ClothR", Vector((-0.07, 0.105, 0)), C("Hips"), "backR"),
    ("J_Sec_Hood", Vector((0.0, 0.12, 0)), C("Head"), "hood"),
]
SHARES = (0.45, 0.33, 0.22)     # 揺れの角度を、根元・中・先の骨に分ける割合
OMEGA = (16.0, 12.0, 9.0)       # ばねの固さ（根元ほど速く追いつく）
ZETA = 0.42                     # 減衰（小さいほど揺れが残る）
DRAG = 7.0                      # 速さ 1 m/s あたり、後ろへなびく角度（度）
SPIN = 0.075                    # 回る速さ（rad/s）の2乗あたり、外へ広がる角度（度）
FALL = 10.0                     # 落ちる速さ 1 m/s あたり、外へ広がる角度（度）
MAX_TILT = 82.0


def fk(basis):
    D = {}
    for n in ORDER:
        Bq = REST_Q[n]
        R = Bq @ basis.get(n, Quaternion()) @ Bq.inverted()
        D[n] = (D[PARENT[n]] @ R) if PARENT[n] else R
    return D


def yaw_of(Dq):
    f = a2w_dir(Dq @ w2a_dir((0, -1, 0)))
    return math.atan2(f.x, -f.y)


def to_body(v, psi):
    return Matrix.Rotation(-psi, 3, "Z") @ Vector(v)


def tilt_dir(ax, ay, psi):
    """後ろ（+Y）へ ax 度、左（+X）へ ay 度倒した、下向きの向き（ワールド）。"""
    th = math.radians(math.hypot(ax, ay))
    if th < 1e-6:
        d = Vector((0, 0, -1))
    else:
        k = math.sin(th) / math.radians(math.hypot(ax, ay))
        d = Vector((math.radians(ay) * k, math.radians(ax) * k, -math.cos(th)))
    return Matrix.Rotation(psi, 3, "Z") @ d


def secondary(frames_basis, hips_world, loop):
    """毎フレームの骨の回転に、垂れ布とフードの骨の回転を書き足す。"""
    n = len(hips_world)
    dt = 1.0 / FPS
    info = []
    for f in range(n):
        D = fk(frames_basis[f])
        psi_h = yaw_of(D[C("Hips")])
        psi_head = yaw_of(D[C("Head")])
        legs = {}
        for side in "LR":
            th = a2w_dir(D[S(side, "UpperLeg")] @ (REST_H[S(side, "LowerLeg")] - REST_H[S(side, "UpperLeg")]))
            ca = a2w_dir(D[S(side, "LowerLeg")] @ (REST_H[S(side, "Foot")] - REST_H[S(side, "LowerLeg")]))
            tb, cb = to_body(th, psi_h), to_body(ca, psi_h)
            legs[side] = (math.degrees(math.atan2(-tb.y, -tb.z)), math.degrees(math.atan2(-cb.y, -cb.z)))
        up = to_body(a2w_dir(D[C("UpperChest")] @ w2a_dir((0, 0, 1))), psi_head)
        chest_pitch = math.degrees(math.atan2(-up.y, up.z))
        info.append(dict(D=D, psi={"hips": psi_h, "head": psi_head}, legs=legs, chest=chest_pitch))

    def unwrap(vals):
        out = [vals[0]]
        for v in vals[1:]:
            d = v - out[-1]
            d = (d + math.pi) % (2 * math.pi) - math.pi
            out.append(out[-1] + d)
        return out

    psis = {k: unwrap([i["psi"][k] for i in info]) for k in ("hips", "head")}

    def deriv(vals, f):
        if loop:
            a, b = vals[(f - 1) % n], vals[(f + 1) % n]
            return (b - a) / (2 * dt)
        a, b = vals[max(f - 1, 0)], vals[min(f + 1, n - 1)]
        span = (min(f + 1, n - 1) - max(f - 1, 0)) * dt
        return (b - a) / span

    hips_v = [deriv(hips_world, f) for f in range(n)]
    omega = {k: [deriv(psis[k], f) if not loop else
                 (((psis[k][(f + 1) % n] - psis[k][(f - 1) % n] + math.pi) % (2 * math.pi)) - math.pi) / (2 * dt)
                 for f in range(n)] for k in psis}

    targets = {}
    for head_name, attach, frame_bone, kind in CHAINS:
        key = "head" if kind == "hood" else "hips"
        tg = []
        for f in range(n):
            psi = psis[key][f]
            w = omega[key][f]
            vb = to_body(hips_v[f], psi)
            v_panel = Vector((vb.x, vb.y, 0)) + Vector((-w * attach.y, w * attach.x, 0))
            r_hat = attach.normalized()
            h = -v_panel * DRAG + r_hat * (SPIN * w * w) + r_hat * (FALL * max(0.0, -vb.z))
            if h.length > MAX_TILT:
                h *= MAX_TILT / h.length
            tg.append((h.y, h.x))   # (後ろへ, 左へ)
        targets[head_name] = tg

    result = {}
    for head_name, attach, frame_bone, kind in CHAINS:
        tg = targets[head_name]
        state = [[Vector((tg[0][0] * s, tg[0][1] * s)), Vector((0, 0))] for s in SHARES]
        cycles = 3 if loop else 1
        out = []
        for c in range(cycles):
            out = []
            for f in range(n):
                tgt = Vector(tg[f])
                for k in range(3):
                    a, v = state[k]
                    goal = tgt * SHARES[k]
                    for _ in range(6):
                        h_ = dt / 6
                        acc = (goal - a) * OMEGA[k] ** 2 - v * (2 * ZETA * OMEGA[k])
                        v = v + acc * h_
                        a = a + v * h_
                    state[k] = [a, v]
                out.append([state[k][0].copy() for k in range(3)])
        result[head_name] = out

    # 骨の回転にする（体へ入り込まないよう押し出してから）
    for f in range(n):
        D = info[f]["D"]
        basis = frames_basis[f]
        for head_name, attach, frame_bone, kind in CHAINS:
            key = "head" if kind == "hood" else "hips"
            psi = psis[key][f]
            angs = result[head_name][f]
            cum = Vector((0, 0))
            for k in range(3):
                cum = cum + angs[k]
                ax, ay = cum.x, cum.y
                if kind == "front":
                    need = max(max(info[f]["legs"][s][0] if k == 0 else max(info[f]["legs"][s])
                                   for s in "LR") * 0.75 - 8, 0.0)
                    ax = min(ax, -need)
                elif kind in ("backL", "backR"):
                    s = "L" if kind == "backL" else "R"
                    th_b = -info[f]["legs"][s][0]
                    ca_b = -info[f]["legs"][s][1]
                    need = (th_b if k == 0 else max(th_b, ca_b * 0.8)) * 0.9 - 4
                    ax = max(ax, need)
                else:
                    ax = max(ax, info[f]["chest"] - 2)
                name = f"{head_name}_{k + 1}"
                target = w2a_dir(tilt_dir(ax, ay, psi))
                pn = PARENT[name]
                Dp = D[pn]
                cur = Dp @ REST_DIR[name]
                delta = cur.rotation_difference(target)
                Bq = REST_Q[name]
                basis[name] = Bq.inverted() @ Dp.inverted() @ delta @ Dp @ Bq
                D[name] = delta @ Dp
                cum = Vector((ax, ay))
    return frames_basis


# ---------------------------------------------------------------------------
# 焼き込み
# ---------------------------------------------------------------------------
def bake(name, keys, length, loop, in_place):
    computed = [(f,) + compute_pose(p) for f, p in keys]
    times = [float(f) for f, *_ in computed]
    bone_names = [n for n in ORDER if any(c[1][n].angle > 1e-5 for c in computed)]
    tracks = {}
    for n in bone_names:
        vals, prev = [], None
        for c in computed:
            q = c[1][n].copy()
            if prev is not None and q.dot(prev) < 0:
                q.negate()
            vals.append(list(q))
            prev = q
        tracks[n] = vals
    loc_vals = [list(c[2]) for c in computed]
    root_vals = [list(c[3]) for c in computed]

    frames_basis, hips_world, hips_key = [], [], []
    for f in range(0, length + 1):
        b = {n: Quaternion(sample_track(times, tracks[n], f, loop, length)).normalized() for n in bone_names}
        frames_basis.append(b)
        lw = Vector(sample_track(times, loc_vals, f, loop, length))
        rw = Vector(sample_track(times, root_vals, f, loop, length))
        hips_world.append(lw)
        if in_place:
            lw = lw - Vector((rw.x, rw.y, 0))
        hips_key.append(REST_Q[C("Hips")].inverted() @ (M_INV.to_3x3() @ lw))
    secondary(frames_basis, hips_world[:-1] if loop else hips_world, loop)
    if loop:
        frames_basis[-1] = dict(frames_basis[0])

    # 書き出す骨格へ移す（骨の世界での向きをそろえる。腰の移動は脚の長さの比で伸び縮みさせる）
    TGT.animation_data_create()
    TGT.animation_data.action = None
    for pb in TGT.pose.bones:
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = Quaternion()
        pb.location = Vector()
    keyed = sorted(set(bone_names) | {n for n in ORDER if n.startswith("J_Sec_") and n != "J_Sec_Sword"})
    for f in range(0, length + 1):
        for n in keyed:
            pb = TGT.pose.bones[n]
            pb.rotation_quaternion = retarget(n, frames_basis[f].get(n, Quaternion()))
            pb.keyframe_insert("rotation_quaternion", frame=f)
        hb = TGT.pose.bones[C("Hips")]
        hb.location = TGT_REST_Q[C("Hips")].inverted() @ (REST_Q[C("Hips")] @ hips_key[f]) * LEG_RATIO
        hb.keyframe_insert("location", frame=f)
    act = TGT.animation_data.action
    act.name = name + ("" if in_place else "_RM")
    act.use_fake_user = True
    for c in computed:
        print(f"[{name}] key {c[0]:>3}: sword tip {tuple(round(x, 3) for x in c[4])}")
    return act


# ---------------------------------------------------------------------------
# ポーズ
# ---------------------------------------------------------------------------
def lerp3(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def mix_pose(a, b, t):
    """2つのポーズを t で混ぜる（数の並びだけを直線で混ぜる）。"""
    def mix(x, y):
        if isinstance(x, (int, float)) and isinstance(y, (int, float)):
            return x + (y - x) * t
        if isinstance(x, str):
            return x
        if isinstance(x, dict):
            keys = set(x) | set(y)
            return {k: mix(x.get(k, y.get(k)), y.get(k, x.get(k))) for k in keys}
        if isinstance(x, (list, tuple)) and len(x) == len(y):
            return type(x)(mix(p, q) for p, q in zip(x, y))
        return y if t >= 0.5 else x
    return mix(a, b)


def P(loc, rot, legs, arms, root=(0, 0, 0), yaw=0.0):
    return {"loc": loc, "rot": rot, "legs": legs, "arms": arms, "root": root, "yaw": yaw}


def body_rot(hips_z=0, spine_x=0, chest_x=0, chest_z=0, upper_x=0, neck_x=0, head_x=0, head_z=0,
             spine_y=0, hips_x=0):
    return {
        C("Hips"): [("X", hips_x), ("Z", hips_z)],
        C("Spine"): [("X", spine_x), ("Y", spine_y)],
        C("Chest"): [("X", chest_x), ("Z", chest_z)],
        C("UpperChest"): [("X", upper_x)],
        C("Neck"): [("X", neck_x), ("Z", head_z * 0.4)],
        C("Head"): [("X", head_x), ("Z", head_z * 0.6)],
    }


# 構え（Idle の基本）。左足を前、右足を後ろに開いて腰を落とし、体を少し右へひねる。
# 剣は右手で後ろ下へ下げ、刀身を後ろへ流す（逆手気味の構え）。左手は前に低く出す。
def guard(breath=0.0, sink=0.0):
    return P(
        loc=(0.0, 0.035, -0.125 - 0.006 * breath - sink),
        rot=body_rot(hips_z=-22, spine_x=7, chest_x=5 + 0.8 * breath, chest_z=-6, upper_x=3 + 0.6 * breath,
                     neck_x=-4, head_x=-8 - 0.6 * breath, head_z=30),
        legs={"L": ((0.13, -0.18, 0.109), 14, 0, (0.35, -1, 0)),
              "R": ((-0.17, 0.21, 0.122), -38, 12, (-0.5, -1, 0))},
        arms={"R": ((-0.27, 0.07, 0.83 - 0.004 * breath), (-0.5, 0.8, 0.2), (-0.12, 0.72, -0.68)),
              "L": ((0.13, -0.25, 0.95 - 0.004 * breath), (0.8, 0.2, -0.5))},
    )


def with_root(p, root, yaw=None):
    q = dict(p)
    q["root"] = root
    if yaw is not None:
        q["yaw"] = yaw
    return q


ANIMS = []   # (名前, キー, 長さ, ループ, 発光の強さ, プレビューのコマ, ゲームで使うか)

# Idle: 3秒で1回、小さく息をする。
ANIMS.append(("Idle", [(0, guard(0)), (45, guard(1)), (90, guard(0))], 90, True, [(0, 1.0)], [0, 45], True))


# Walk: 低い姿勢で忍び寄る（1.1秒で2歩）。
WALK_LEN = 32
WALK_STRIDE = 0.25


def walk_pose(phase):
    def foot(ph):
        ph %= 1.0
        if ph < 0.6:
            t = ph / 0.6
            return -WALK_STRIDE + 2 * WALK_STRIDE * t, 0.109, 0.0
        t = (ph - 0.6) / 0.4
        s_ = t * t * (3 - 2 * t)
        return WALK_STRIDE - 2 * WALK_STRIDE * s_, 0.109 + 0.07 * math.sin(math.pi * t), 26 * math.sin(math.pi * t) * (1 - t) - 8 * t * t

    ry, rz, ra = foot(phase)
    ly, lz, la = foot(phase + 0.5)
    s = math.cos(2 * math.pi * phase)
    bob = math.cos(4 * math.pi * (phase - 0.3))
    return P(
        loc=(-0.012 * math.sin(2 * math.pi * phase), 0.02, -0.085 + 0.010 * bob),
        rot=body_rot(hips_z=-10 + 5 * s, spine_x=9, chest_x=5, chest_z=-3 * s, upper_x=2, neck_x=-4, head_x=-8,
                     head_z=10),
        legs={"R": ((-0.095, ry, rz), -6, ra, (-0.1, -1, 0)), "L": ((0.095, ly, lz), 6, la, (0.1, -1, 0))},
        arms={"R": ((-0.26, 0.06 + 0.02 * s, 0.86 + 0.006 * bob), (-0.5, 0.8, 0.2), (-0.12, 0.72, -0.68)),
              "L": (lerp3((0.20, 0.08, 0.92), (0.17, -0.20, 0.96), 0.5 + 0.5 * s), (0.6, 0.6, -0.3))},
    )


ANIMS.append(("Walk", [(f, walk_pose(f / WALK_LEN)) for f in range(0, WALK_LEN + 1, 2)], WALK_LEN, True,
              [(0, 1.0)], [0, 8, 16, 24], True))


# ---- Dash Slash -------------------------------------------------------------
def dash_coil(sink=0.0, root=(0, 0, 0)):
    # 予備動作: さらに沈み、剣を右の腰の後ろへ引く
    return P(loc=(0, 0.06, -0.20 - sink),
             rot=body_rot(hips_z=-34, spine_x=15, chest_x=10, chest_z=-10, upper_x=6, neck_x=-8, head_x=-12, head_z=42),
             legs={"L": ((0.13, -0.20, 0.109), 14, 0, (0.35, -1, 0)),
                   "R": ((-0.17, 0.28, 0.14), -36, 26, (-0.5, -1, 0))},
             arms={"R": ((-0.30, 0.18, 0.86), (-0.4, 0.9, 0.2), (-0.10, 0.95, -0.28)),
                   "L": ((0.18, -0.30, 0.90), (0.8, 0.2, -0.5))},
             root=root)


def lunge(root, hips_z, chest_z, wrist, pole, blade, l_wrist, l_foot=(0.14, -0.55, 0.109), r_foot=(-0.13, 0.55, 0.20),
          r_pitch=40, sink=0.27, spine=22, l_pitch=0, l_yaw=10):
    return P(loc=(0, -0.04, -sink),
             rot=body_rot(hips_z=hips_z, spine_x=spine, chest_x=10, chest_z=chest_z, upper_x=4, neck_x=-10, head_x=-14,
                          head_z=-hips_z - chest_z * 0.5),
             legs={"L": (l_foot, l_yaw, l_pitch, (0.3, -1, 0)), "R": (r_foot, -20, r_pitch, (-0.3, -1, 0))},
             arms={"R": (wrist, pole, blade), "L": (l_wrist, (0.4, 0.9, 0))},
             root=root)


DASH_END = (0, -2.45, 0)
dash_keys = [
    (0, guard(0)),
    (4, dash_coil()),
    (6, dash_coil(0.012)),
    # 攻撃（7〜12 コマ）: 一気に踏み込み、すれ違いざまに右から左へ横一閃
    (7, lunge((0, -0.30, 0), -30, -8, (-0.40, 0.08, 0.90), (-0.5, 0.6, 0.6), (-0.65, 0.70, -0.10), (0.25, 0.10, 0.95),
              l_foot=(0.12, -0.42, 0.19), r_foot=(-0.15, 0.38, 0.15), r_pitch=35, sink=0.23, spine=20, l_pitch=-10)),
    (9, lunge((0, -1.30, 0), -12, 0, (-0.34, -0.42, 1.02), (-0.6, 0.2, 0.8), (-0.72, -0.69, 0.0), (0.30, 0.25, 0.98))),
    (10, lunge((0, -1.85, 0), 12, 10, (0.00, -0.50, 1.02), (-0.3, 0.2, 0.9), (0.10, -1.0, 0.0), (0.30, 0.28, 0.98))),
    (11, lunge((0, -2.22, 0), 30, 14, (0.28, -0.36, 1.00), (-0.1, 0.0, 1.0), (0.85, -0.50, -0.02), (0.30, 0.30, 0.96))),
    (12, lunge(DASH_END, 44, 15, (0.36, -0.06, 0.98), (-0.2, -0.3, 0.9), (0.72, 0.62, -0.20), (0.30, 0.32, 0.95),
               l_foot=(0.20, -0.36, 0.109), r_foot=(-0.18, 0.45, 0.109), r_pitch=0, sink=0.28, spine=18, l_yaw=30)),
    # 残心: 振り抜いた低い姿勢のまま静かに止まる
    (15, lunge((0, -2.55, 0), 46, 15, (0.37, -0.04, 0.96), (-0.2, -0.3, 0.9), (0.70, 0.62, -0.28), (0.30, 0.32, 0.94),
               l_foot=(0.20, -0.36, 0.109), r_foot=(-0.18, 0.45, 0.109), r_pitch=0, sink=0.30, spine=18, l_yaw=30)),
    (30, lunge((0, -2.55, 0), 45, 14, (0.36, -0.03, 0.95), (-0.2, -0.3, 0.9), (0.68, 0.62, -0.32), (0.30, 0.32, 0.94),
               l_foot=(0.20, -0.36, 0.109), r_foot=(-0.18, 0.45, 0.109), r_pitch=0, sink=0.295, spine=17, l_yaw=30)),
    (42, with_root(guard(0), (0, -2.55, 0))),
]
ANIMS.append(("DashSlash", dash_keys, 42, False,
              [(0, 1.0), (6, 1.2), (8, 2.6), (13, 2.6), (30, 1.0)], [0, 4, 7, 9, 10, 11, 12, 20, 42], True))


# ---- Spin Combo -------------------------------------------------------------
def stance(root, yaw=0.0, hips_z=0, spine=10, chest_z=0, sink=0.15, wrist=(-0.27, 0.07, 0.83), pole=(-0.5, 0.8, 0.2),
           blade=(-0.12, 0.72, -0.68), l_wrist=(0.15, -0.25, 0.95), l_foot=(0.13, -0.20, 0.109),
           r_foot=(-0.16, 0.22, 0.12), r_pitch=12, feet_up=0.0, head_z=None, l_pitch=0):
    lf = (l_foot[0], l_foot[1], l_foot[2] + feet_up)
    rf = (r_foot[0], r_foot[1], r_foot[2] + feet_up)
    return P(loc=(0, 0.03, -sink),
             rot=body_rot(hips_z=hips_z, spine_x=spine, chest_x=6, chest_z=chest_z, upper_x=3, neck_x=-5, head_x=-9,
                          head_z=(-hips_z - chest_z) if head_z is None else head_z),
             legs={"L": (lf, 14, l_pitch, (0.35, -1, 0)), "R": (rf, -30, r_pitch, (-0.5, -1, 0))},
             arms={"R": (wrist, pole, blade), "L": (l_wrist, (0.8, 0.2, -0.5))},
             root=root, yaw=yaw)


spin_keys = [(0, guard(0))]
# 1撃目: 右上から左下へ袈裟に斬り下ろす
spin_keys += [
    (4, stance((0, 0, 0), hips_z=-30, wrist=(-0.24, -0.04, 1.46), pole=(-0.8, 0.3, 0.3), blade=(-0.30, 0.35, 0.88),
               l_wrist=(0.20, -0.22, 1.08), spine=6)),
    (7, stance((0, -0.18, 0), hips_z=16, chest_z=10, wrist=(0.14, -0.40, 0.84), pole=(-0.3, 0.2, 0.9),
               blade=(0.62, -0.42, -0.66), l_wrist=(0.26, 0.18, 0.98), spine=16, sink=0.20, l_foot=(0.14, -0.34, 0.109))),
    (11, stance((0, -0.20, 0), hips_z=17, chest_z=10, wrist=(0.15, -0.39, 0.82), pole=(-0.3, 0.2, 0.9),
                blade=(0.60, -0.42, -0.68), l_wrist=(0.26, 0.18, 0.97), spine=16, sink=0.21, l_foot=(0.14, -0.34, 0.109))),
]
# 2撃目: 剣を左へ引いてから、右回りに1回転して水平に薙ぐ（足は少し浮かせる）
spin_keys.append((14, stance((0, -0.22, 0), hips_z=32, chest_z=12, wrist=(0.24, -0.12, 1.04), pole=(0.0, 0.3, 0.9),
                             blade=(0.70, 0.62, 0.20), l_wrist=(0.28, 0.10, 1.00), spine=10, sink=0.22,
                             l_foot=(0.13, -0.30, 0.109))))
for i, yw in enumerate((-40, -100, -160, -220, -280, -335, -372)):
    f = 15 + i
    rt = (0, -0.25 - 0.06 * i, 0)
    spin_keys.append((f, stance(rt, yaw=yw, hips_z=0, chest_z=-6, wrist=(-0.50, -0.16, 1.04), pole=(-0.2, 0.3, 0.9),
                                blade=(-0.92, -0.38, 0.0), l_wrist=(0.34, 0.10, 1.02), spine=8, sink=0.12,
                                l_foot=(0.12, -0.10, 0.109), r_foot=(-0.12, 0.10, 0.109), r_pitch=0,
                                feet_up=0.07 * math.sin(math.pi * (i + 1) / 8), head_z=0)))
# 3撃目: 剣を左下へ構え、左下から右上へ斬り上げる
spin_keys += [
    (25, stance((0, -0.66, 0), yaw=-360, hips_z=26, chest_z=10, wrist=(0.18, -0.22, 0.80), pole=(0.2, 0.4, 0.9),
                blade=(0.55, -0.35, -0.76), l_wrist=(0.30, 0.10, 0.92), spine=18, sink=0.24, l_foot=(0.13, -0.28, 0.109))),
    (28, stance((0, -0.78, 0), yaw=-360, hips_z=-26, chest_z=-10, wrist=(-0.30, -0.30, 1.50), pole=(-0.8, 0.2, 0.4),
                blade=(-0.42, -0.20, 0.88), l_wrist=(0.28, 0.12, 1.00), spine=4, sink=0.16, l_foot=(0.14, -0.36, 0.109))),
    (31, stance((0, -0.80, 0), yaw=-360, hips_z=-30, chest_z=-12, wrist=(-0.32, -0.26, 1.54), pole=(-0.8, 0.2, 0.4),
                blade=(-0.45, -0.05, 0.89), l_wrist=(0.28, 0.12, 1.00), spine=3, sink=0.16, l_foot=(0.14, -0.36, 0.109))),
    # 残心: 剣をゆっくり右下へ払い、低い構えで止まる
    (40, stance((0, -0.80, 0), yaw=-360, hips_z=-24, chest_z=-6, wrist=(-0.40, -0.10, 0.92), pole=(-0.6, 0.6, 0.3),
                blade=(-0.62, 0.25, -0.74), l_wrist=(0.16, -0.24, 0.95), spine=12, sink=0.20, l_foot=(0.14, -0.30, 0.109))),
    (52, stance((0, -0.80, 0), yaw=-360, hips_z=-24, chest_z=-6, wrist=(-0.40, -0.09, 0.91), pole=(-0.6, 0.6, 0.3),
                blade=(-0.62, 0.26, -0.74), l_wrist=(0.16, -0.24, 0.94), spine=12, sink=0.205, l_foot=(0.14, -0.30, 0.109))),
    (62, with_root(guard(0), (0, -0.80, 0), yaw=-360)),
]
ANIMS.append(("SpinCombo", spin_keys, 62, False,
              [(0, 1.0), (4, 1.3), (6, 2.5), (10, 1.6), (15, 2.6), (21, 2.6), (25, 1.8), (27, 2.8), (32, 2.2), (50, 1.0)],
              [0, 4, 7, 14, 16, 18, 20, 25, 28, 40, 62], False))


# ---- Leap Slam --------------------------------------------------------------
def air(root, loc_z, l_foot, r_foot, wrist, blade, spine=0, l_pitch=30, r_pitch=30, grip=True, l_wrist=None,
        pole=(-0.8, 0.2, 0.3), head_x=-6):
    return P(loc=(0, 0.0, loc_z),
             rot=body_rot(spine_x=spine, chest_x=spine * 0.5, upper_x=spine * 0.3, neck_x=-spine * 0.3, head_x=head_x),
             legs={"L": (l_foot, 8, l_pitch, (0.2, -1, 0)), "R": (r_foot, -8, r_pitch, (-0.2, -1, 0))},
             arms={"R": (wrist, pole, blade), "L": ("grip", -0.03, (0.8, 0.2, -0.3)) if grip else (l_wrist, (0.8, 0.2, -0.5))},
             root=root)


LEAP_END = (0, -1.20, 0)
LAND_ROOT = LEAP_END


def slam(sink, spine=30, glow_hold=False):
    # 着地: 左足を前に、右膝を地面について、剣を前の地面へまっすぐ突き立てる
    return P(loc=(0, 0.05, -sink),
             rot=body_rot(spine_x=spine, chest_x=10, upper_x=6, neck_x=-8, head_x=-14),
             legs={"L": ((0.15, -0.26, 0.109), 10, 0, (0.3, -1, 0)), "R": ((-0.12, 0.40, 0.13), -6, 62, (-0.1, -1, -0.4))},
             arms={"R": ((-0.03, -0.40, 0.62), (-0.8, 0.3, 0.2), (0.0, -0.06, -1.0)),
                   "L": ("grip", -0.03, (0.8, 0.2, -0.2))},
             root=LAND_ROOT)


leap_keys = [
    (0, guard(0)),
    # 予備動作: 一瞬深く沈む
    (4, P(loc=(0, 0.05, -0.30), rot=body_rot(spine_x=26, chest_x=10, upper_x=4, neck_x=-12, head_x=-14),
          legs={"L": ((0.13, -0.06, 0.109), 10, 0, (0.3, -1, 0)), "R": ((-0.13, 0.08, 0.109), -10, 0, (-0.3, -1, 0))},
          arms={"R": ((-0.30, 0.26, 0.82), (-0.4, 0.9, 0.2), (-0.10, 0.62, -0.78)), "L": ((0.30, 0.25, 0.85), (0.4, 0.9, 0))})),
    # 跳ぶ: 脚を伸ばし、剣を頭の上へ
    (7, air((0, -0.10, 0.12), 0.06, (0.10, -0.02, 0.22), (-0.10, 0.10, 0.16), (-0.12, -0.10, 1.74), (0.0, 0.25, 0.97),
            spine=-4, l_pitch=45, r_pitch=50, head_x=-2)),
    # 上る: 膝を引き寄せ、両手で剣を頭の上に構える
    (12, air((0, -0.50, 0.78), -0.02, (0.12, -0.18, 0.46), (-0.12, 0.02, 0.40), (-0.05, -0.06, 1.66),
             (0.0, 0.32, 0.95), spine=-6, l_pitch=30, r_pitch=40)),
    # 頂点: 剣を返して切っ先を下に向ける
    (17, air((0, -0.80, 1.05), -0.04, (0.12, -0.26, 0.50), (-0.12, -0.12, 0.42), (-0.03, -0.30, 1.42),
             (0.0, -0.24, -0.97), spine=20, l_pitch=20, r_pitch=30, head_x=-16)),
    (20, air((0, -0.95, 0.98), -0.04, (0.13, -0.24, 0.40), (-0.12, -0.04, 0.34), (-0.03, -0.30, 1.48),
             (0.0, -0.22, -0.97), spine=20, l_pitch=15, r_pitch=25, head_x=-16)),
    # 落ちる: 脚を伸ばし、剣を構えたまま
    (24, air((0, -1.12, 0.48), -0.02, (0.14, -0.24, 0.22), (-0.12, 0.14, 0.24), (-0.03, -0.32, 1.50),
             (0.0, -0.18, -0.98), spine=16, l_pitch=10, r_pitch=20, head_x=-14)),
    (27, air((0, -1.19, 0.08), -0.10, (0.15, -0.26, 0.15), (-0.12, 0.30, 0.20), (-0.03, -0.36, 1.10),
             (0.0, -0.10, -1.0), spine=22, l_pitch=5, r_pitch=30, head_x=-14)),
    # 着地と同時に突き立てる。そのまま一瞬止まる
    (28, slam(0.42)),
    (30, slam(0.445, spine=32)),
    (44, slam(0.44, spine=31)),
    # 剣を抜いて立ち上がる
    (51, P(loc=(0, 0.05, -0.30), rot=body_rot(spine_x=20, chest_x=8, upper_x=4, neck_x=-8, head_x=-12),
           legs={"L": ((0.15, -0.24, 0.109), 10, 0, (0.3, -1, 0)), "R": ((-0.13, 0.30, 0.122), -20, 30, (-0.3, -1, 0))},
           arms={"R": ((-0.10, -0.34, 0.82), (-0.8, 0.3, 0.2), (-0.10, -0.30, -0.95)), "L": ((0.18, -0.20, 0.90), (0.8, 0.2, -0.5))},
           root=LAND_ROOT)),
    (60, with_root(guard(0), LAND_ROOT)),
]
ANIMS.append(("LeapSlam", leap_keys, 60, False,
              [(0, 1.0), (12, 1.6), (25, 2.2), (28, 3.2), (44, 1.4), (60, 1.0)],
              [0, 4, 7, 12, 17, 24, 28, 40, 51, 60], False))


# ---- Backstep ---------------------------------------------------------------
back_keys = [
    (0, guard(0)),
    (3, with_root(guard(0, sink=0.06), (0, 0, 0))),
    # 蹴り出す（足は地面に残し、体が後ろへ）
    (5, P(loc=(0, 0.02, -0.10), rot=body_rot(hips_z=-18, spine_x=4, chest_x=2, chest_z=-4, neck_x=-2, head_x=-4, head_z=24),
          legs={"L": ((0.13, -0.40, 0.14), 14, 30, (0.35, -1, 0)), "R": ((-0.16, -0.02, 0.15), -30, 30, (-0.5, -1, 0))},
          arms={"R": ((-0.28, -0.02, 0.90), (-0.5, 0.8, 0.2), (-0.18, 0.40, -0.90)), "L": ((0.18, -0.30, 1.02), (0.8, 0.2, -0.5))},
          root=(0, 0.22, 0.05))),
    # 宙: 膝を引き、剣と左手は前に残す
    (9, P(loc=(0, 0.0, -0.08), rot=body_rot(hips_z=-16, spine_x=10, chest_x=4, chest_z=-4, neck_x=-6, head_x=-8, head_z=24),
          legs={"L": ((0.12, -0.24, 0.30), 14, 20, (0.35, -1, 0)), "R": ((-0.13, -0.06, 0.26), -30, 25, (-0.5, -1, 0))},
          arms={"R": ((-0.30, -0.16, 0.94), (-0.5, 0.8, 0.2), (-0.20, -0.55, -0.81)), "L": ((0.20, -0.32, 1.04), (0.8, 0.2, -0.5))},
          root=(0, 1.00, 0.26))),
    # 着地: 低く受け止める
    (12, with_root(guard(0, sink=0.12), (0, 1.58, 0))),
    (15, with_root(guard(0, sink=0.15), (0, 1.62, 0))),
    (22, with_root(guard(0, sink=0.01), (0, 1.64, 0))),
    (30, with_root(guard(0), (0, 1.64, 0))),
]
ANIMS.append(("Backstep", back_keys, 30, False, [(0, 1.0)], [0, 3, 5, 9, 12, 15, 30], False))


# ---- Feint Dash -------------------------------------------------------------
feint_keys = [
    (0, guard(0)),
    # 予備動作: 一瞬左へ体重をかける（見せかけ）
    (3, P(loc=(0.03, 0.04, -0.17), rot=body_rot(hips_z=-20, spine_x=8, spine_y=6, chest_x=5, chest_z=-6, neck_x=-4,
                                                   head_x=-8, head_z=28),
          legs={"L": ((0.14, -0.18, 0.109), 14, 0, (0.35, -1, 0)), "R": ((-0.17, 0.21, 0.122), -38, 12, (-0.5, -1, 0))},
          arms={"R": ((-0.26, 0.08, 0.82), (-0.5, 0.8, 0.2), (-0.12, 0.72, -0.68)), "L": ((0.15, -0.24, 0.94), (0.8, 0.2, -0.5))})),
    # 右へ跳ぶ
    (5, P(loc=(-0.02, 0.03, -0.15), rot=body_rot(hips_z=-20, spine_x=8, spine_y=-10, chest_x=5, chest_z=-6, neck_x=-4,
                                                    head_x=-8, head_z=28),
          legs={"L": ((0.32, -0.10, 0.13), 14, 25, (0.35, -1, 0)), "R": ((-0.10, 0.12, 0.20), -30, 20, (-0.5, -1, 0))},
          arms={"R": ((-0.30, 0.10, 0.84), (-0.5, 0.8, 0.2), (-0.15, 0.72, -0.67)), "L": ((0.24, -0.18, 0.98), (0.8, 0.2, -0.5))},
          root=(-0.25, 0, 0.02))),
    (8, P(loc=(0, 0.03, -0.12), rot=body_rot(hips_z=-22, spine_x=8, spine_y=-14, chest_x=5, chest_z=-6, neck_x=-4,
                                                head_x=-8, head_z=30),
          legs={"L": ((0.18, -0.10, 0.20), 14, 20, (0.35, -1, 0)), "R": ((-0.18, 0.12, 0.17), -30, 15, (-0.5, -1, 0))},
          arms={"R": ((-0.30, 0.12, 0.86), (-0.5, 0.8, 0.2), (-0.15, 0.80, -0.58)), "L": ((0.26, -0.14, 1.00), (0.8, 0.2, -0.5))},
          root=(-0.95, 0, 0.06))),
    # 右足で踏みとどまり、向きを変える
    (10, P(loc=(-0.04, 0.03, -0.22), rot=body_rot(hips_z=-32, spine_x=14, spine_y=-6, chest_x=8, chest_z=-10, neck_x=-8,
                                                     head_x=-12, head_z=38),
           legs={"L": ((0.20, -0.02, 0.14), 14, 20, (0.35, -1, 0)), "R": ((-0.22, 0.04, 0.109), -40, 0, (-0.5, -1, 0))},
           arms={"R": ((-0.30, 0.16, 0.88), (-0.4, 0.9, 0.2), (-0.12, 0.93, -0.33)), "L": ((0.18, -0.30, 0.92), (0.8, 0.2, -0.5))},
           root=(-1.22, 0, 0))),
    # 攻撃（12〜16 コマ）: 左前へ斬り込み、右から左へ薙ぐ
    (12, lunge((-1.08, -0.45, 0), -28, -8, (-0.40, 0.06, 0.90), (-0.5, 0.6, 0.6), (-0.65, 0.70, -0.10), (0.25, 0.10, 0.95),
               l_foot=(0.12, -0.42, 0.19), r_foot=(-0.15, 0.38, 0.15), r_pitch=35, sink=0.23, spine=20, l_pitch=-10)),
    (14, lunge((-0.82, -1.25, 0), -6, 4, (-0.20, -0.48, 1.02), (-0.5, 0.2, 0.8), (-0.45, -0.89, 0.0), (0.30, 0.25, 0.98))),
    (15, lunge((-0.70, -1.62, 0), 20, 12, (0.18, -0.44, 1.01), (-0.2, 0.1, 1.0), (0.55, -0.83, 0.0), (0.30, 0.28, 0.97))),
    (16, lunge((-0.62, -1.85, 0), 42, 15, (0.36, -0.06, 0.98), (-0.2, -0.3, 0.9), (0.72, 0.62, -0.20), (0.30, 0.32, 0.95),
               l_foot=(0.20, -0.36, 0.109), r_foot=(-0.18, 0.45, 0.109), r_pitch=0, sink=0.28, spine=18, l_yaw=30)),
    # 残心
    (19, lunge((-0.58, -1.94, 0), 45, 15, (0.37, -0.04, 0.96), (-0.2, -0.3, 0.9), (0.70, 0.62, -0.28), (0.30, 0.32, 0.94),
               l_foot=(0.20, -0.36, 0.109), r_foot=(-0.18, 0.45, 0.109), r_pitch=0, sink=0.30, spine=18, l_yaw=30)),
    (32, lunge((-0.58, -1.94, 0), 44, 14, (0.36, -0.03, 0.95), (-0.2, -0.3, 0.9), (0.68, 0.62, -0.32), (0.30, 0.32, 0.94),
               l_foot=(0.20, -0.36, 0.109), r_foot=(-0.18, 0.45, 0.109), r_pitch=0, sink=0.295, spine=17, l_yaw=30)),
    (42, with_root(guard(0), (-0.58, -1.94, 0))),
]
ANIMS.append(("FeintDash", feint_keys, 42, False, [(0, 1.0), (11, 1.3), (13, 2.6), (17, 2.6), (32, 1.0)],
              [0, 3, 5, 8, 10, 12, 14, 16, 26, 42], False))


# ---------------------------------------------------------------------------
# 書き出し
# ---------------------------------------------------------------------------
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


os.makedirs(os.path.join(OUT_DIR, "RootMotion"), exist_ok=True)
results = {}
for name, keys, length, loop, glow, prev_frames, in_game in ANIMS:
    SCENE.frame_start, SCENE.frame_end = 0, length
    if in_game:
        bake(name, keys, length, loop, in_place=True)
        export(os.path.join(OUT_DIR, name + ".fbx"))
    act = bake(name, keys, length, loop, in_place=False)
    export(os.path.join(OUT_DIR, "RootMotion", name + ".fbx"))
    results[name] = (act, length, glow, prev_frames)


# ---------------------------------------------------------------------------
# プレビュー（キーポーズの連番画像）
# ---------------------------------------------------------------------------
def preview_material(mesh):
    """トゥーン寄りの陰影と発光を、マテリアル（体・剣）ごとに、その名前の albedo・normal・emission の画像から組む。"""
    d = os.path.dirname(SRC_FBX)

    def toon(prefix):
        m = bpy.data.materials.new("prev_" + prefix)
        m.use_nodes = True
        t = m.node_tree
        for nd in list(t.nodes):
            t.nodes.remove(nd)
        out = t.nodes.new("ShaderNodeOutputMaterial")
        ta = t.nodes.new("ShaderNodeTexImage")
        ta.image = bpy.data.images.load(os.path.join(d, prefix + "_albedo.png"))
        # 拡散の3段
        diff = t.nodes.new("ShaderNodeBsdfDiffuse")
        s2r = t.nodes.new("ShaderNodeShaderToRGB")
        t.links.new(diff.outputs[0], s2r.inputs[0])
        if os.path.exists(os.path.join(d, prefix + "_normal.png")):
            tn = t.nodes.new("ShaderNodeTexImage")
            tn.image = bpy.data.images.load(os.path.join(d, prefix + "_normal.png"))
            tn.image.colorspace_settings.name = "Non-Color"
            nmap = t.nodes.new("ShaderNodeNormalMap")
            t.links.new(tn.outputs["Color"], nmap.inputs["Color"])
            t.links.new(nmap.outputs["Normal"], diff.inputs["Normal"])
        ramp = t.nodes.new("ShaderNodeValToRGB")
        ramp.color_ramp.interpolation = "CONSTANT"
        els = ramp.color_ramp.elements
        els[0].position, els[0].color = 0.0, (0.34, 0.34, 0.40, 1)
        els[1].position, els[1].color = 0.18, (0.66, 0.66, 0.70, 1)
        e3 = els.new(0.55)
        e3.color = (1, 1, 1, 1)
        t.links.new(s2r.outputs["Color"], ramp.inputs["Fac"])
        mul = t.nodes.new("ShaderNodeMix")
        mul.data_type = "RGBA"
        mul.blend_type = "MULTIPLY"
        mul.inputs["Factor"].default_value = 1.0
        t.links.new(ta.outputs["Color"], mul.inputs[6])
        t.links.new(ramp.outputs["Color"], mul.inputs[7])
        # 発光のマスク（<名前>_emission.png）の白いところだけを光らせる
        te = t.nodes.new("ShaderNodeTexImage")
        te.image = bpy.data.images.load(os.path.join(d, prefix + "_emission.png"))
        te.image.colorspace_settings.name = "Non-Color"
        sep = t.nodes.new("ShaderNodeSeparateColor")
        t.links.new(te.outputs["Color"], sep.inputs[0])

        def mult(a, b):
            n_ = t.nodes.new("ShaderNodeMath")
            n_.operation = "MULTIPLY"
            t.links.new(a, n_.inputs[0])
            t.links.new(b, n_.inputs[1])
            return n_.outputs[0]
        mask = sep.outputs[0]
        strength = t.nodes.new("ShaderNodeValue")
        strength.name = "GlowStrength"
        strength.outputs[0].default_value = 1.0
        em = t.nodes.new("ShaderNodeVectorMath")
        em.operation = "SCALE"
        em.inputs[0].default_value = (0.54 * 1.2, 0.36 * 1.2, 1.0 * 1.2)
        t.links.new(mult(mask, strength.outputs[0]), em.inputs["Scale"])
        add = t.nodes.new("ShaderNodeVectorMath")
        add.operation = "ADD"
        t.links.new(mul.outputs[2], add.inputs[0])
        t.links.new(em.outputs[0], add.inputs[1])
        emis = t.nodes.new("ShaderNodeEmission")
        t.links.new(add.outputs[0], emis.inputs["Color"])
        t.links.new(emis.outputs[0], out.inputs["Surface"])
        return m, strength

    names = [m_.name.split(".")[0] for m_ in mesh.data.materials]
    built = [toon(n) for n in names]
    mesh.data.materials.clear()
    for m_, _ in built:
        mesh.data.materials.append(m_)
    ol = bpy.data.materials.new("outline")
    ol.use_nodes = True
    ot = ol.node_tree
    for nd in list(ot.nodes):
        ot.nodes.remove(nd)
    o_out = ot.nodes.new("ShaderNodeOutputMaterial")
    o_em = ot.nodes.new("ShaderNodeEmission")
    o_em.inputs["Color"].default_value = (0.012, 0.010, 0.018, 1)
    ot.links.new(o_em.outputs[0], o_out.inputs["Surface"])
    ol.use_backface_culling = True
    for _ in built:
        mesh.data.materials.append(ol)
    sol = mesh.modifiers.new("Outline", "SOLIDIFY")
    sol.thickness = 0.0035 / UNIT if mesh.matrix_world.to_scale()[0] < 0.5 else 0.0035
    sol.offset = 1.0
    sol.use_flip_normals = True
    sol.material_offset = len(built)
    return [st for _, st in built]


def render_previews():
    import numpy as np
    mesh = next(o for o in bpy.data.objects if o.type == "MESH")
    strengths = preview_material(mesh)
    try:
        SCENE.render.engine = "BLENDER_EEVEE"
    except TypeError:
        SCENE.render.engine = "BLENDER_EEVEE_NEXT"
    SCENE.render.resolution_x, SCENE.render.resolution_y = 300, 380
    SCENE.world = bpy.data.worlds.new("w")
    SCENE.world.use_nodes = True
    bg = SCENE.world.node_tree.nodes["Background"]
    bg.inputs[0].default_value = (0.62, 0.64, 0.68, 1)
    bg.inputs[1].default_value = 0.8
    SCENE.view_settings.view_transform = "Standard"
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sun.data.energy = 3.2
    sun.rotation_euler = (math.radians(48), 0, math.radians(-35))
    SCENE.collection.objects.link(sun)
    bpy.ops.mesh.primitive_plane_add(size=12)
    fl = bpy.context.active_object
    fm = bpy.data.materials.new("floor")
    fm.use_nodes = True
    fm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.42, 0.45, 0.40, 1)
    fl.data.materials.append(fm)
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    SCENE.collection.objects.link(cam)
    SCENE.camera = cam
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 2.6
    hips = TGT.pose.bones[C("Hips")]
    for name, (act, length, glow, frames) in results.items():
        TGT.animation_data.action = act
        if hasattr(TGT.animation_data, "action_slot") and act.slots:
            TGT.animation_data.action_slot = act.slots[0]
        folder = os.path.join(PREVIEW_DIR, name)
        os.makedirs(folder, exist_ok=True)
        rows = []
        for ang in (40, 100):
            tiles = []
            for f in frames:
                SCENE.frame_set(f)
                for st in strengths:
                    st.outputs[0].default_value = sample_linear(glow, f)
                hw = TGT.matrix_world @ hips.head
                a = math.radians(ang)
                tgt = Vector((hw.x, hw.y, 0.95 + max(0.0, hw.z - 0.95) * 0.8))
                cam.location = tgt + Vector((math.sin(a) * 5.0, -math.cos(a) * 5.0, 0.6))
                cam.rotation_euler = (tgt - cam.location).to_track_quat("-Z", "Y").to_euler()
                p = os.path.join(folder, f"{name}_{f:03d}.png" if ang == 40 else "_tile.png")
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
        tp = os.path.join(folder, "_tile.png")
        if os.path.exists(tp):
            os.remove(tp)
        print("PREVIEW", img.filepath_raw)


if PREVIEW_DIR:
    render_previews()
