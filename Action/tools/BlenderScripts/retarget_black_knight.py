# 既存のモーション（FBX）を黒騎士の骨格へ移し、垂れ布の揺れを付けて FBX に書き出す。
#
# 使い方（Blender 5.2 で確認）:
#   blender -b --python retarget_black_knight.py -- <設定の .json> [--preview <プレビューのフォルダ>]
#           [--views 90,30] [--count 6] [--only 名前,名前]
#
# 設定（black_knight_anims.json）
#   target    書き出す骨格とメッシュ（BlackKnight.fbx）
#   out_dir   書き出すフォルダ。<out_dir>/<name>.fbx に書く
#   grip      {"fbx": 手のポーズの FBX, "bones": [右手の指の骨, ...]}。全部のクリップで、剣を持つ右手の指をこのポーズにする
#             （元のモーションが手を開いていても、拳で剣を握るように）。クリップに "grip": false と書くと重ねない
#   clips     書き出すクリップの一覧。1つずつ次を書く
#     name     書き出す名前
#     source   元のモーションの FBX
#     rig      元の骨格の種類。"kevin"（Kevin Iglesias の B-hips 式）、"mixamo"（mixamorig: 式）、"vrm"（J_Bip_ 式）
#     loop     ループするか（true なら最後のコマを最初のコマと同じにする）
#     rest     T ポーズを読む、元と同じ骨格のモデルの FBX（省くと source の中の初期の姿勢を使う）
#     frames   使うコマの範囲 [最初, 最後]（元の FBX のコマ番号。省くと全部）
#     mask     腕などのポーズだけを重ねる。{"fbx": 腕だけのポーズの FBX, "bones": ["B-upperArm.R", ...], "frame": 何コマ目を使うか}
#              ここに書いた元の骨は、元のモーションの回転の代わりに、このポーズの回転を使う（親から見た回転）
#     mirror   true なら左右を反転する（左手の動きを右手にする、など）
#     spin     [[コマ, 度], ...]。そのコマで体ごと縦の軸まわりに何度回っているか（+ で左回り。回転斬り・横一閃用）。
#              コマは frames の最初を 0 と数える。間はなめらかにつなぐ
#     segments 上の項目（source〜spin）を書いたものを並べると、順につないで1本にする。
#              つなぎ目は blend コマ（既定 4）かけて混ぜ、腰の水平の位置は前の終わりに合わせる
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
# プレビューの向き（度。0 = 正面、90 = 体の左から）とコマ数。--views 0,90 --count 12 のように変えられる
PREVIEW_VIEWS = [float(v) for v in argv[argv.index("--views") + 1].split(",")] if "--views" in argv else [90.0, 30.0]
PREVIEW_COUNT = int(argv[argv.index("--count") + 1]) if "--count" in argv else 6
# --focus J_Bip_R_Hand のように骨の名前を渡すと、その骨に寄って描く（握りを確かめる用）
PREVIEW_FOCUS = argv[argv.index("--focus") + 1] if "--focus" in argv else None
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

    # 腕・手だけのポーズ（mask）を読む。設定全体の grip（剣を握る右手の指）を先に、クリップの mask をあとに重ねる
    def read_mask(mk, bones):
        marm, mact = import_armature(resolve(mk["fbx"]))
        set_action(marm, mact)
        mb = read_basis(marm, mk.get("frame", int(mact.frame_range[0])))
        bpy.data.objects.remove(marm, do_unlink=True)
        if mact:
            bpy.data.actions.remove(mact)
        return {n: mb[n] for n in bones if n in mb}

    mask_basis = {}
    grip = CONFIG.get("grip")
    if grip and clip.get("grip", True):
        bones = grip["bones"]
        if clip.get("mirror", False):
            # 左右を反転するクリップでは、黒騎士の右手は元の左手から来る
            bones = [b.replace(".R", ".L") for b in bones]
        mask_basis.update(read_mask(grip, bones))
    if "mask" in clip:
        mask_basis.update(read_mask(clip["mask"], clip["mask"]["bones"]))

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

    # 左右を反転するときは、反対側の骨の回転を、左右（X）で鏡に映して使う
    mirror = clip.get("mirror", False)
    src_names = {b.name for b in src.data.bones}

    def other_side(s):
        for a, b in ((".L", ".R"), ("Left", "Right"), ("_L_", "_R_")):
            if a in s:
                n = s.replace(a, b)
                return n if n in src_names else s
            if b in s:
                n = s.replace(b, a)
                return n if n in src_names else s
        return s

    f0, f1 = clip.get("frames", [int(act.frame_range[0]), int(act.frame_range[1])])
    frames_world, hips_world = [], []
    for f in range(f0, f1 + 1):
        basis = read_basis(src, f)
        basis.update(mask_basis)
        pw = posed_world(src, basis)
        W = {}
        for t, s in bmap.items():
            sm = other_side(s) if mirror else s
            d = (G @ pw[sm].to_quaternion()) @ rest_q_src[sm].inverted()
            if mirror:
                d = Quaternion((d.w, d.x, -d.y, -d.z))
            W[t] = (d @ REF[t]).normalized()
        hp = G @ pw[hips_src].translation
        if mirror:
            hp.x = -hp.x
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
def spin_angle(keys, i):
    """[[コマ, 度], ...] の間を、なめらかに（速くなって遅くなる）つないだ角度。"""
    if i <= keys[0][0]:
        return keys[0][1]
    for (fa, da), (fb, db) in zip(keys, keys[1:]):
        if i <= fb:
            s = (i - fa) / max(fb - fa, 1)
            s = s * s * (3 - 2 * s)
            return da + (db - da) * s
    return keys[-1][1]


def apply_spin(frames_world, hips_world, spin):
    """spin: [[コマ, 度], ...]。そのコマで体ごと縦の軸まわりに何度回っているか（+ で左回り）。間はなめらかにつなぐ。"""
    if not spin:
        return
    for i in range(len(frames_world)):
        q = Quaternion((0, 0, 1), math.radians(spin_angle(spin, i)))
        frames_world[i] = {n: (q @ w).normalized() for n, w in frames_world[i].items()}
        h = hips_world[i]
        hips_world[i] = Vector((0, 0, h.z)) + q @ Vector((h.x, h.y, 0))


def blend_world(wa, wb, t):
    return {n: wa[n].slerp(wb[n], t).normalized() for n in wa if n in wb}


def build_frames(clip):
    """1本の元のモーションか、いくつかをつないだもの（segments）を、骨の世界での向きのコマの並びにする。"""
    segments = clip.get("segments", [clip])
    frames_world, hips_world = [], []
    for k, seg in enumerate(segments):
        W, H = retarget_clip(seg)
        # 何コマ目から始まったか（つなぎ目で blend コマ重なる）。斬撃を出すコマなどを決める用
        start = 0 if not frames_world else len(frames_world) - min(seg.get("blend", clip.get("blend", 4)), len(W), len(frames_world))
        print(f"SEGMENT [{clip['name']}] {k}: 出来上がりの {start} コマ目から（元の {seg.get('frames', ['最初'])[0]} コマ目）")
        for w in W:
            apply_adjust(w, seg.get("adjust"))
        apply_spin(W, H, seg.get("spin"))
        if not frames_world:
            frames_world, hips_world = W, H
            continue
        # つなぎ目は blend コマかけて混ぜる。腰の水平の位置は、前の終わりに合わせる
        b = min(seg.get("blend", clip.get("blend", 4)), len(W), len(frames_world))
        off = hips_world[-1] - H[0]
        off.z = 0
        H = [h + off for h in H]
        for i in range(b):
            t = (i + 1) / (b + 1)
            k = len(frames_world) - b + i
            frames_world[k] = blend_world(frames_world[k], W[i], t)
            hips_world[k] = hips_world[k].lerp(H[i], t)
        frames_world += W[b:]
        hips_world += H[b:]
    return frames_world, hips_world


def bake(clip):
    frames_world, hips_world = build_frames(clip)
    loop = clip.get("loop", False)
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
    fit_sword_to_grip(length)
    act = TGT.animation_data.action
    act.name = clip["name"]
    act.use_fake_user = True
    SCENE.frame_start, SCENE.frame_end = 0, length
    print(f"[{clip['name']}] {length + 1} コマ（{(length) / FPS:.2f} 秒）")
    print_sword_path(clip["name"], length)
    return act, length


SWORD_BONE = "J_Sec_Sword"


def sword_grip_in_hand():
    """剣のメッシュの柄の中心と、柄から剣先への向きを、右手の骨の初期の空間で返す。"""
    mesh = next(o for o in bpy.data.objects if o.type == "MESH")
    vg = mesh.vertex_groups.get(SWORD_BONE)
    to_hand = BONES[S("R", "Hand")].matrix_local.inverted() @ TGT.matrix_world.inverted() @ mesh.matrix_world
    pts = [to_hand @ v.co for v in mesh.data.vertices
           if any(g.group == vg.index and g.weight > 0.5 for g in v.groups)]
    head = BONES[S("R", "Hand")].matrix_local.inverted() @ BONES[SWORD_BONE].head_local
    tip = max(pts, key=lambda p: (p - head).length)
    axis0 = (tip - head).normalized()
    pommel = min(pts, key=lambda p: p.dot(axis0))
    axis = (tip - pommel).normalized()
    # 柄から剣先へ、輪切りにして太さを見る。いちばん太いところ（鍔）までの、根元側の半分が柄
    length = (tip - pommel).dot(axis)
    bins = 60
    width = [0.0] * bins
    for p in pts:
        t = (p - pommel).dot(axis)
        k = min(int(t / length * bins), bins - 1)
        r = ((p - pommel) - axis * t).length
        width[k] = max(width[k], r)
    guard = max(range(bins // 2), key=lambda k: width[k])
    t_guard = guard / bins * length
    # 柄は両端にしか頂点がないので、根元から鍔までの頂点の、軸からのずれの平均を、柄の真ん中の高さに置く
    grip = [p for p in pts if (p - pommel).dot(axis) < t_guard]
    side = sum(((p - pommel) - axis * (p - pommel).dot(axis) for p in grip), Vector()) / max(len(grip), 1)
    center = pommel + axis * (t_guard * 0.5) + side
    print(f"SWORD GRIP 長さ {length:.1f} 鍔まで {t_guard:.1f}（剣の骨の空間ではなく右手の骨の空間）")
    return center, axis


def fit_sword_to_grip(length):
    """剣の柄が、握った右手の指の輪の中を通るように、剣の骨を動かす（全部のコマで同じ）。
    柄の中心は曲げた指の輪の真ん中、柄の向きは手のひらを横切る向き（刃は親指の側へ出る）。"""
    if CONFIG.get("sword_fit") is False:
        return
    SCENE.frame_set(0)
    hand = TGT.pose.bones[S("R", "Hand")]
    to_hand = hand.matrix.inverted()
    # 曲げた指（ミトン。中指の3本の骨に重みがある）の輪の真ん中
    pts = [to_hand @ TGT.pose.bones[S("R", f"Middle{i}")].head for i in (1, 2, 3)]
    pts.append(to_hand @ TGT.pose.bones[S("R", "Middle3")].tail)
    want_center = sum(pts, Vector()) / len(pts)
    # 柄の向きは、T ポーズの手のひらを横切る向き（前 = 親指の側）。手の骨の空間では変わらない
    want_axis = (BONES[S("R", "Hand")].matrix_local.inverted().to_3x3() @ w2a_dir((0, -1, 0))).normalized()
    if not hasattr(fit_sword_to_grip, "grip"):
        fit_sword_to_grip.grip = sword_grip_in_hand()
    have_center, have_axis = fit_sword_to_grip.grip
    move = (Matrix.Translation(want_center) @ have_axis.rotation_difference(want_axis).to_matrix().to_4x4()
            @ Matrix.Translation(-have_center))
    rel = BONES[S("R", "Hand")].matrix_local.inverted() @ BONES[SWORD_BONE].matrix_local
    basis = rel.inverted() @ move @ rel
    pb = TGT.pose.bones[SWORD_BONE]
    for f in range(length + 1):
        pb.matrix_basis = basis
        pb.keyframe_insert("location", frame=f)
        pb.keyframe_insert("rotation_quaternion", frame=f)


def sword_tip_local():
    """剣のメッシュで、剣の骨からいちばん遠い点（剣先）を、剣の骨の空間で返す。"""
    mesh = next(o for o in bpy.data.objects if o.type == "MESH")
    vg = mesh.vertex_groups.get(SWORD_BONE)
    bone_w = TGT.matrix_world @ BONES[SWORD_BONE].matrix_local
    head = bone_w.translation
    best, best_d = None, -1.0
    for v in mesh.data.vertices:
        if any(g.group == vg.index and g.weight > 0.5 for g in v.groups):
            w = mesh.matrix_world @ v.co
            d = (w - head).length
            if d > best_d:
                best, best_d = w, d
    return bone_w.inverted() @ best


def print_sword_path(name, length):
    """剣先の通り道を、腰から見た（右, 前, 上）cm で3コマごとに出す（斬る向きを確かめる用）。"""
    if "--sword" not in argv:
        return
    if not hasattr(print_sword_path, "tip"):
        print_sword_path.tip = sword_tip_local()
    out = []
    for f in range(0, length + 1, 3):
        SCENE.frame_set(f)
        hips = TGT.matrix_world @ TGT.pose.bones[C("Hips")].head
        tip = TGT.matrix_world @ TGT.pose.bones[SWORD_BONE].matrix @ print_sword_path.tip
        d = (tip - hips) * 100
        yaw = math.degrees(math.atan2(*((TGT.matrix_world.to_quaternion() @ TGT.pose.bones[C("Hips")].matrix.to_quaternion()
                                          @ REST_Q[C("Hips")].inverted() @ Vector((0, -1, 0))).xy * Vector((1, -1)))))
        out.append(f"{f}:({-d.x:.0f},{-d.y:.0f},{d.z:.0f}) 腰{yaw:.0f}°")
    print(f"SWORD [{name}] " + " ".join(out))


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
    count = PREVIEW_COUNT
    frames = [round(length * i / (count - 1)) for i in range(count)]
    rows = []
    for ang in PREVIEW_VIEWS:
        tiles = []
        for f in frames:
            SCENE.frame_set(f)
            a = math.radians(ang)
            t = Vector((0, 0, 1.0))
            if PREVIEW_FOCUS:
                t = TGT.matrix_world @ TGT.pose.bones[PREVIEW_FOCUS].head
                cam.data.ortho_scale = 1.1
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
