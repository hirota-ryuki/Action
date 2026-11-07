"""UE 5.8 コマンドライン用: 歩き・走りのアニメーションで、接地中の足が地面を送る速さを測る。

使い方:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="measure_foot_speed.py <AnimSequenceのパス> <結果のtxt>"
  例: measure_foot_speed.py /Game/04_Retarget/Mixamo/Out/Player_A_Walk D:/04_dawnload/mixamo/foot_speed.txt

結果の「速さ」は UE の単位（cm/秒）。ゲームでは Player のモデルの大きさ（scale）を掛けた値を
PlayerStateSettings::walkAnimMoveSpeed などに入れる。
"""
import sys

import unreal

FEET = ["J_Bip_L_Foot", "J_Bip_R_Foot"]
FPS = 30.0


def main():
    anim_path, out_path = sys.argv[-2], sys.argv[-1]
    anim = unreal.load_asset(anim_path)
    length = anim.get_play_length()
    opts = unreal.AnimPoseEvaluationOptions()
    lines = ["%s length=%.3f" % (anim_path, length)]

    # 足ごとに、各コマの位置を取る。
    samples = {foot: [] for foot in FEET}
    n = int(length * FPS) + 1
    for i in range(n):
        t = min(i / FPS, length)
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, t, opts)
        for foot in FEET:
            p = unreal.AnimPoseExtensions.get_bone_pose(pose, foot, unreal.AnimPoseSpaces.WORLD).translation
            samples[foot].append((t, p))

    # 足が低い（接地している）コマの間の、水平方向の速さの中央値を取る。
    # 平均にすると、かかとが着く・つま先が離れる瞬間の遅いコマに引っぱられて小さく出る。
    speeds = []
    for foot in FEET:
        pts = samples[foot]
        ground = min(p.z for _, p in pts)
        for (t0, p0), (t1, p1) in zip(pts, pts[1:]):
            if p0.z < ground + 2.0 and p1.z < ground + 2.0 and t1 > t0:
                d = ((p1.x - p0.x) ** 2 + (p1.y - p0.y) ** 2) ** 0.5
                speeds.append(d / (t1 - t0))
    if speeds:
        speeds.sort()
        lines.append("接地中の足の速さ(中央値) = %.1f cm/秒（%d 区間）" % (speeds[len(speeds) // 2], len(speeds)))
    else:
        lines.append("接地しているコマが見つかりませんでした")
    open(out_path, "w", encoding="utf-8").write("\n".join(lines))


main()
