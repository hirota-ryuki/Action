# 既存のモーション（FBX）を黒騎士の骨格へ移し、垂れ布の揺れを付けて FBX に書き出す。
#
# 使い方（Blender 5.2 で確認）:
#   blender -b --python retarget_black_knight.py -- <設定の .json> [--preview <プレビューのフォルダ>]
#
# 設定（black_knight_anims.json）
#   target    書き出す骨格とメッシュ（BlackKnight.fbx）
#   out_dir   書き出すフォルダ。<out_dir>/<name>.fbx に書く
#   clips     書き出すクリップの一覧。1つずつ次を書く
#     name     書き出す名前
#     source   元のモーションの FBX
#     rig      元の骨格の種類。"kevin"（Kevin Iglesias の B-hips 式）、"mixamo"（mixamorig: 式）、"vrm"（J_Bip_ 式）
#     loop     ループするか（true なら最後のコマを最初のコマと同じにする）
#     rest     T ポーズを読む、元と同じ骨格のモデルの FBX（省くと source の中の初期の姿勢を使う）
#     frames   使うコマの範囲 [最初, 最後]（元の FBX のコマ番号。省くと全部）
#     mask     腕などのポーズだけを重ねる。{"fbx": 腕だけのポーズの FBX, "bones": ["B-upperArm.R", ...], "frame": 何コマ目を使うか}
#              ここに書いた元の骨は、元のモーションの回転の代わりに、このポーズの回転を使う（親から見た回転）
#     adjust   移したあとで骨を回す。[{"bone": "J_Bip_R_Hand", "axis": [x, y, z], "deg": 度}, ...]
#              軸は体（腰の向き）から見た向き（+X が体の左、-Y が前、+Z が上）。その骨と、その下の骨がまとめて回る
#
# 移し方
#   - 元と黒騎士は、どちらも T ポーズで、前が -Y・上が +Z。骨の世界での向きを「T ポーズからどれだけ回ったか」でそろえる。
#     初期の骨の向きが少し違う分は、元の初期の骨の向きに合わせる（元が A ポーズ寄りでも、そのまま写る）。
#   - 元に無い骨（Chest など）は、親と子の間を半分ずつ回す。指は対応があるものだけ移す。
#   - 腰の位置は、腰の高さの比で伸び縮みさせる。
#   - 垂れ布とフードの骨は、腰・頭の動きから、ばねで遅れて揺れるように後から付ける（前のスクリプトの処理を移したもの）。
import bpy
import sys
import os
import json
import math
from mathutils import Vector, Quaternion, Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
CONFIG_PATH = os.path.abspath(argv[0])
PREVIEW_DIR = os.path.abspath(argv[argv.index("--preview") + 1]) if "--preview" in argv else None
with open(CONFIG_PATH, encoding="utf-8") as fp:
    CONFIG = json.load(fp)
BASE = os.path.dirname(CONFIG_PATH)


def resolve(p):
    return p if os.path.isabs(p) else os.path.normpath(os.path.join(BASE, p))


TARGET_FBX = resolve(CONFIG["target"])
OUT_DIR = resolve(CONFIG["out_dir"])
FPS = 30

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=TARGET_FBX)
TGT = next(o for o in bpy.data.objects if o.type == "ARMATURE")
SCENE = bpy.context.scene
SCENE.render.fps = FPS
SCENE.render.fps_base = 1.0

M_WORLD = TGT.matrix_world.copy()
M_INV = M_WORLD.inverted()
Q_ARM = M_WORLD.to_quaternion()
Q_ARM_INV = Q_ARM.inverted()
UNIT = M_WORLD.to_scale()[0]


def C(name):
    return f"J_Bip_C_{name}"


def S(side, name):
    return f"J_Bip_{side}_{name}"


BONES = TGT.data.bones
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
CHILDREN = {n: [m for m in ORDER if PARENT[m] == n] for n in ORDER}


def descendants(n):
    out = []
    for c in CHILDREN[n]:
        out.append(c)
        out += descendants(c)
    return out


def w2a_dir(v):
    return Q_ARM_INV @ Vector(v)


def a2w_dir(v):
    return Q_ARM @ Vector(v)


# ---------------------------------------------------------------------------
# 骨の対応（黒騎士の骨 ← 元の骨）
# ---------------------------------------------------------------------------
def kevin_map():
    m = {
        C("Hips"): "B-hips", C("Spine"): "B-spine", C("UpperChest"): "B-chest",
        C("Neck"): "B-neck", C("Head"): "B-head",
    }
    for s, k in (("L", "L"), ("R", "R")):
        m.update({
            S(s, "Shoulder"): f"B-shoulder.{k}", S(s, "UpperArm"): f"B-upperArm.{k}",
            S(s, "LowerArm"): f"B-forearm.{k}", S(s, "Hand"): f"B-hand.{k}",
            S(s, "UpperLeg"): f"B-thigh.{k}", S(s, "LowerLeg"): f"B-shin.{k}",
            S(s, "Foot"): f"B-foot.{k}", S(s, "ToeBase"): f"B-toe.{k}",
        })
        for vrm, kev in (("Index", "indexFinger"), ("Middle", "middleFinger"), ("Ring", "ringFinger"), ("Little", "pinky")):
            for i in (1, 2, 3):
                m[S(s, f"{vrm}{i}")] = f"B-{kev}0{i}.{k}"
        for i in (1, 2, 3):
            m[S(s, f"Thumb{i}")] = f"B-thumb0{i}.{k}"
    return m


def mixamo_map():
    p = "mixamorig:"
    m = {
        C("Hips"): p + "Hips", C("Spine"): p + "Spine", C("Chest"): p + "Spine1", C("UpperChest"): p + "Spine2",
        C("Neck"): p + "Neck", C("Head"): p + "Head",
    }
    for s, k in (("L", "Left"), ("R", "Right")):
        m.update({
            S(s, "Shoulder"): f"{p}{k}Shoulder", S(s, "UpperArm"): f"{p}{k}Arm",
            S(s, "LowerArm"): f"{p}{k}ForeArm", S(s, "Hand"): f"{p}{k}Hand",
            S(s, "UpperLeg"): f"{p}{k}UpLeg", S(s, "LowerLeg"): f"{p}{k}Leg",
            S(s, "Foot"): f"{p}{k}Foot", S(s, "ToeBase"): f"{p}{k}ToeBase",
        })
        for vrm in ("Index", "Middle", "Ring", "Thumb"):
            for i in (1, 2, 3):
                m[S(s, f"{vrm}{i}")] = f"{p}{k}Hand{vrm}{i}"
        for i in (1, 2, 3):
            m[S(s, f"Little{i}")] = f"{p}{k}HandPinky{i}"
    return m


def vrm_map(src_bones):
    return {b.name: b.name for b in BONES if b.name.startswith("J_Bip_") and b.name in src_bones}


# 元の骨に無いとき、親と子の間を半分ずつ回す骨（骨, 親側の骨, 子側の骨）
BLEND_BONES = [(C("Chest"), C("Spine"), C("UpperChest"))]


# ---------------------------------------------------------------------------
# 元のモーションを読む
# ---------------------------------------------------------------------------
def import_armature(path):
    before_obj = set(bpy.data.objects)
    before_act = set(bpy.data.actions)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in before_obj]
    arm = next(o for o in new if o.type == "ARMATURE")
    for o in new:
        if o is not arm:
            bpy.data.objects.remove(o, do_unlink=True)
    acts = [a for a in bpy.data.actions if a not in before_act]
    return arm, (acts[0] if acts else None)


def set_action(arm, act):
    arm.animation_data_create()
    arm.animation_data.action = act
    if hasattr(arm.animation_data, "action_slot") and act.slots:
        arm.animation_data.action_slot = act.slots[0]


def read_basis(arm, frame):
    SCENE.frame_set(frame)
    return {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}


def posed_world(arm, basis):
    """骨ごとの、ポーズを付けたときの世界での行列（4x4）。"""
    out = {}
    bones = sorted(arm.data.bones, key=depth)
    for b in bones:
        m = b.matrix_local @ basis.get(b.name, Matrix())
        if b.parent:
            m = out[b.parent.name] @ b.parent.matrix_local.inverted() @ m
        out[b.name] = m
    return {n: arm.matrix_world @ m for n, m in out.items()}


def rest_world(arm):
    return {b.name: arm.matrix_world @ b.matrix_local for b in arm.data.bones}


def forward_of(world_rest, foot, toe):
    """足首からつま先への向き（水平）を、その骨格の前とする。"""
    v = world_rest[toe].translation - world_rest[foot].translation
    v.z = 0
    return v.normalized()


# ---------------------------------------------------------------------------
# 1本のクリップを黒騎士の骨の回転にする
# ---------------------------------------------------------------------------
TGT_REST_W = rest_world(TGT)


def retarget_clip(clip):
    src, act = import_armature(resolve(clip["source"]))
    set_action(src, act)
    rig = clip.get("rig", "kevin")
    if rig == "kevin":
        bmap = kevin_map()
    elif rig == "mixamo":
        bmap = mixamo_map()
    else:
        bmap = vrm_map({b.name for b in src.data.bones})
    bmap = {t: s for t, s in bmap.items() if t in REST_Q and s in src.data.bones}

    # 腕だけのポーズ（mask）を読む
    mask_basis = {}
    if "mask" in clip:
        mk = clip["mask"]
        marm, mact = import_armature(resolve(mk["fbx"]))
        set_action(marm, mact)
        mb = read_basis(marm, mk.get("frame", int(mact.frame_range[0])))
        mask_basis = {n: mb[n] for n in mk["bones"] if n in mb}
        bpy.data.objects.remove(marm, do_unlink=True)

    src_rest = rest_world(src)
    if "rest" in clip:
        # アニメーションだけの FBX（UE から書き出したものなど）は、中の初期の姿勢が T ポーズでないことがある。
        # そのときは、同じ骨格のモデルの FBX から T ポーズを読む。
        rarm, ract = import_armature(resolve(clip["rest"]))
        src_rest = rest_world(rarm)
        bpy.data.objects.remove(rarm, do_unlink=True)
        if ract:
            bpy.data.actions.remove(ract)
    # 前の向きをそろえる回転（元の前 → 黒騎士の前）
    if rig == "kevin":
        fs = forward_of(src_rest, "B-foot.L", "B-toe.L")
    elif rig == "mixamo":
        fs = forward_of(src_rest, "mixamorig:LeftFoot", "mixamorig:LeftToeBase")
    else:
        fs = forward_of(src_rest, S("L", "Foot"), S("L", "ToeBase"))
    ft = forward_of(TGT_REST_W, S("L", "Foot"), S("L", "ToeBase"))
    G = fs.rotation_difference(ft)

    # 骨ごとに「元の T ポーズの向き → 黒騎士の骨の向き」の対応を作る。
    # 黒騎士の骨の初期の向きを、元の初期の骨の向きに合わせたもの（REF）を、元の回転の差で回す。
    rest_q_src = {n: (G @ m.to_quaternion()) for n, m in src_rest.items()}
    REF = {}
    for t, s in bmap.items():
        bt, bs = BONES[t], src.data.bones[s]
        dt_ = (TGT_REST_W[t].to_3x3() @ Vector((0, 1, 0))).normalized()
        ds_ = (G @ (src_rest[s].to_3x3() @ Vector((0, 1, 0)))).normalized()
        # 長さがほぼ 0 の骨（Kevin の B-head など）は向きが当てにならないので、合わせない
        if bs.length * src.matrix_world.to_scale()[0] < 0.02 or bt.length * UNIT < 0.02:
            A = Quaternion()
        else:
            A = dt_.rotation_difference(ds_)
        REF[t] = A @ TGT_REST_W[t].to_quaternion()

    hips_src = bmap[C("Hips")]
    h_ratio = TGT_REST_W[C("Hips")].translation.z / src_rest[hips_src].translation.z

    f0, f1 = clip.get("frames", [int(act.frame_range[0]), int(act.frame_range[1])])
    frames_world, hips_world = [], []
    for f in range(f0, f1 + 1):
        basis = read_basis(src, f)
        basis.update(mask_basis)
        pw = posed_world(src, basis)
        W = {}
        for t, s in bmap.items():
            q = G @ pw[s].to_quaternion()
            W[t] = (q @ rest_q_src[s].inverted() @ REF[t]).normalized()
        hp = G @ pw[hips_src].translation
        hips_world.append(Vector((hp.x, hp.y, hp.z)) * h_ratio)
        frames_world.append(W)
    bpy.data.objects.remove(src, do_unlink=True)
    if act:
        bpy.data.actions.remove(act)
    return frames_world, hips_world


def body_yaw_q(W):
    """腰の向き（水平に回った分）の回転。"""
    d = W[C("Hips")] @ TGT_REST_W[C("Hips")].to_quaternion().inverted()
    f = d @ Vector((0, -1, 0))
    return Quaternion((0, 0, 1), math.atan2(f.x, -f.y))


def to_basis(W):
    """世界での向き（骨ごと） → 骨の回転（親から見た回転）。無い骨は親に付いていく。"""
    # 元に無い骨を、親と子の間で半分ずつ回す
    for n, a, b in BLEND_BONES:
        if n not in W and a in W and b in W:
            qa = W[a] @ TGT_REST_W[a].to_quaternion().inverted()
            qb = W[b] @ TGT_REST_W[b].to_quaternion().inverted()
            W[n] = qa.slerp(qb, 0.5) @ TGT_REST_W[n].to_quaternion()
    posed = {}
    basis = {}
    for n in ORDER:
        p = PARENT[n]
        rest_rel = (REST_Q[p].inverted() @ REST_Q[n]) if p else REST_Q[n]
        if n in W:
            arm_q = Q_ARM_INV @ W[n]
            if p:
                b = rest_rel.inverted() @ posed[p].inverted() @ arm_q
            else:
                b = REST_Q[n].inverted() @ arm_q
            basis[n] = b.normalized()
            posed[n] = arm_q
        else:
            basis[n] = Quaternion()
            posed[n] = (posed[p] @ rest_rel) if p else REST_Q[n]
    return basis


def apply_adjust(W, adjust):
    if not adjust:
        return
    yaw = body_yaw_q(W)
    for a in adjust:
        q = yaw @ Quaternion(Vector(a["axis"]).normalized(), math.radians(a["deg"])) @ yaw.inverted()
        for n in [a["bone"]] + descendants(a["bone"]):
            if n in W:
                W[n] = (q @ W[n]).normalized()


# ---------------------------------------------------------------------------
# 垂れ布とフードの揺れ（make_black_knight_anims.py から移したもの）
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
# 焼き込みと書き出し
# ---------------------------------------------------------------------------
def bake(clip):
    frames_world, hips_world = retarget_clip(clip)
    loop = clip.get("loop", False)
    for W in frames_world:
        apply_adjust(W, clip.get("adjust"))
    frames_basis = [to_basis(W) for W in frames_world]
    if loop:
        # 最後のコマを最初のコマと同じにして、つなぎ目で跳ねないようにする
        frames_basis[-1] = dict(frames_basis[0])
        hips_world[-1] = hips_world[0].copy()
    secondary(frames_basis, hips_world[:-1] if loop else hips_world, loop)
    if loop:
        frames_basis[-1] = dict(frames_basis[0])

    TGT.animation_data_create()
    TGT.animation_data.action = None
    for pb in TGT.pose.bones:
        pb.rotation_mode = "QUATERNION"
        pb.rotation_quaternion = Quaternion()
        pb.location = Vector()
    length = len(frames_basis) - 1
    keyed = [n for n in ORDER if any(fb.get(n, Quaternion()).angle > 1e-5 for fb in frames_basis)]
    hips = C("Hips")
    for f in range(length + 1):
        prev = {}
        for n in keyed:
            pb = TGT.pose.bones[n]
            q = frames_basis[f].get(n, Quaternion())
            if f > 0 and q.dot(pb.rotation_quaternion) < 0:
                q = -q
            pb.rotation_quaternion = q
            pb.keyframe_insert("rotation_quaternion", frame=f)
        hb = TGT.pose.bones[hips]
        p_arm = M_INV @ hips_world[f]
        hb.location = REST_Q[hips].inverted() @ (p_arm - REST_H[hips])
        hb.keyframe_insert("location", frame=f)
    act = TGT.animation_data.action
    act.name = clip["name"]
    act.use_fake_user = True
    SCENE.frame_start, SCENE.frame_end = 0, length
    print(f"[{clip['name']}] {length + 1} コマ（{(length) / FPS:.2f} 秒）")
    return act, length


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


# ---------------------------------------------------------------------------
# プレビュー（6コマを、横と斜め前から並べた1枚）
# ---------------------------------------------------------------------------
def render_preview(name, act, length):
    import numpy as np
    mesh = next(o for o in bpy.data.objects if o.type == "MESH")
    if not getattr(render_preview, "ready", False):
        SCENE.render.engine = "BLENDER_WORKBENCH"
        SCENE.display.shading.light = "STUDIO"
        SCENE.display.shading.color_type = "TEXTURE"
        SCENE.display.shading.show_cavity = True
        SCENE.world = bpy.data.worlds.new("w")
        SCENE.world.color = (0.55, 0.57, 0.6)
        SCENE.view_settings.view_transform = "Standard"
        SCENE.view_settings.exposure = 1.6
        SCENE.render.resolution_x, SCENE.render.resolution_y = 300, 420
        cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
        SCENE.collection.objects.link(cam)
        SCENE.camera = cam
        cam.data.type = "ORTHO"
        cam.data.ortho_scale = 2.8
        render_preview.ready = True
    cam = SCENE.camera
    TGT.animation_data.action = act
    if hasattr(TGT.animation_data, "action_slot") and act.slots:
        TGT.animation_data.action_slot = act.slots[0]
    frames = [round(length * i / 5) for i in range(6)]
    rows = []
    for ang in (90, 30):
        tiles = []
        for f in frames:
            SCENE.frame_set(f)
            a = math.radians(ang)
            t = Vector((0, 0, 1.0))
            cam.location = t + Vector((math.sin(a) * 5.0, -math.cos(a) * 5.0, 0.5))
            cam.rotation_euler = (t - cam.location).to_track_quat("-Z", "Y").to_euler()
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
    os.remove(os.path.join(PREVIEW_DIR, "_tile.png"))
    print("PREVIEW", img.filepath_raw, frames)


os.makedirs(OUT_DIR, exist_ok=True)
if PREVIEW_DIR:
    os.makedirs(PREVIEW_DIR, exist_ok=True)
only = argv[argv.index("--only") + 1].split(",") if "--only" in argv else None
for clip in CONFIG["clips"]:
    if only and clip["name"] not in only:
        continue
    act, length = bake(clip)
    export(os.path.join(OUT_DIR, clip["name"] + ".fbx"))
    if PREVIEW_DIR:
        render_preview(clip["name"], act, length)
