#include "stdafx.h"
#include "FirstPersonArms.h"

#include "Player.h"

namespace
{
	// プレイヤーの体から、袖の肘から先を消したモデル（tools/FirstPerson/make_first_person_arms_tkm.py で作る）。
	// 骨は SK_Player と同じ。腕（上腕から先）以外の骨を大きさ 0 につぶして、腕と手だけを映す。
	const char* const MODEL_PATH = "Assets/modelData/player/Model/SK_PlayerFp.tkm";
	const char* const CLIP_DIR = "Assets/modelData/player/Animation/FirstPerson/";
	constexpr float MODEL_SCALE = 1.128f;  // プレイヤーの体と同じ大きさ。
	// この骨とその下の骨だけを残す。
	const wchar_t* const ARM_ROOT_BONES[] = { L"J_Bip_L_UpperArm", L"J_Bip_R_UpperArm" };
	// 腕の置き方（ArmPose）。肩を中心に回す角度と、腕ごと動かす量。
	// 外へずらしてから内へ回すと、肘が画面の端の外に出て、前腕が画面の端から真ん中へ斜めに伸びる。
	struct ArmPose
	{
		float spreadDeg;  // 外へ開く角度（度）。- で内へ向ける。
		float raiseDeg;   // 上げる角度（度）。
		float sideShift;  // 腕ごと画面の外側へずらす量。
		float drop;       // 腕ごと下げる量。
		float push;       // 腕ごと前へ押し出す量。
	};
	// 構え: 手を画面の下 1/3 の左右に置く。
	const ArmPose IDLE_POSE = { -25.0f, 55.0f, 12.0f, 0.0f, 0.0f };
	// 撃つとき（炎の溜め始め・溜め・放つは両腕、氷は右腕だけ）: 両手を上げる動きなので、角度を抑えて下げ、
	// 手を画面の下 1/3 に収めて前が見えるようにする。
	const ArmPose CAST_POSE = { 0.0f, 24.0f, 0.0f, 0.0f, 10.0f };
	// 握りこぶしにするときに、アニメーションの指の形から足して曲げる角度（度。付け根・第二関節・第一関節の順）。
	const float FINGER_CURL_DEGS[3] = { 45.0f, 50.0f, 40.0f };
	// 親指: 先の関節を曲げる角度（度）と、IK で合わせるときの決まり。
	constexpr float THUMB_CURL_DEG = 30.0f;
	constexpr float THUMB_OUTSIDE_OFFSET = 1.5f;  // 目標を指の中節から外へ出す量（指の太さくらい。骨格の単位）。
	constexpr float THUMB_TIP_LENGTH_RATE = 0.9f; // 親指の先の骨の長さを、ひとつ手前の骨の長さの何倍とみなすか。
	constexpr int THUMB_IK_ITERATIONS = 6;
	// 手を開く速さと、握り直す速さ（大きいほど速い）。手を開いた瞬間に魔法が出るので、開くのは速くする。
	constexpr float GRIP_OPEN_SPEED = 40.0f;
	constexpr float GRIP_CLOSE_SPEED = 8.0f;
	// 炎を撃ったあと、両手を開いたまま見せる時間（秒）。そのあと最初の構えに戻る。
	constexpr float RELEASE_OPEN_SECONDS = 0.2f;
	// 構えと撃つときの置き方を混ぜる速さ（大きいほど速く切り替わる）。
	constexpr float POSE_BLEND_SPEED = 10.0f;

	// カメラから見て、腕のモデルの足元を置く位置（x: 右、y: 上、z: 前）。
	// 腕のモデルの目の高さ（約 157）より下げ、手前に寄せて、手が大きく画面の下の方に映るようにする。
	const Vector3 ARMS_OFFSET = Vector3(0.0f, -183.0f, -18.0f);

	// アニメーションを切り替えるときの補間（秒）。
	constexpr float TO_IDLE_SECONDS = 0.15f;
	constexpr float TO_CAST_SECONDS = 0.05f;
	constexpr float TO_CHARGE_START_SECONDS = 0.08f;
	constexpr float TO_CHARGE_SECONDS = 0.1f;
}

void FirstPersonArms::Init()
{
	CharacterModelDesc desc;
	desc.modelPath = getenv("FP_TEST_MODEL") ? getenv("FP_TEST_MODEL") : MODEL_PATH; // FP_TEST_TEMP
	desc.upAxis = enModelUpAxisZ;
	desc.scale = MODEL_SCALE;
	desc.isShadowCaster = false;  // 体を描いていないので、腕だけの影が床に落ちないようにする。
	desc.clips.resize(enClip_Num);
	const std::string dir = CLIP_DIR;
	desc.clips[enClip_Idle] = { dir + "FpIdle.fbx", true };
	desc.clips[enClip_CastRight] = { dir + "FpCastRight.fbx", false };
	desc.clips[enClip_ChargeStart] = { dir + "FpChargeStart.fbx", false };
	desc.clips[enClip_Charge] = { dir + "FpCharge.fbx", true };
	if (getenv("FP_TEST_NOANIM")) { desc.clips.clear(); } // FP_TEST_TEMP
	if (getenv("FP_TEST_SHADOW")) { desc.isShadowCaster = true; } // FP_TEST_TEMP
	if (const char* e = getenv("FP_TEST_IDLE")) { desc.clips[enClip_Idle].path = e; } // FP_TEST_TEMP
	m_model.Init(desc);
	m_clip = enClip_Idle;
	InitHiddenBones();
}

void FirstPersonArms::InitHiddenBones()
{
	Skeleton& skeleton = m_model.GetModelRender().GetSkeleton();
	const int numBones = skeleton.GetNumBones();
	// 腕の骨か（自分か親をたどって、上腕の骨に行き着くか）。
	std::vector<bool> isArm(numBones, false);
	for (int side = 0; side < 2; side++) {
		m_armBones[side].clear();
		m_upperArmBone[side] = skeleton.FindBoneID(ARM_ROOT_BONES[side]);
	}
	for (int boneNo = 0; boneNo < numBones; boneNo++) {
		for (int no = boneNo; no >= 0; no = skeleton.GetBone(no)->GetParentBoneNo()) {
			const wchar_t* name = skeleton.GetBone(no)->GetName();
			const int side = wcscmp(name, ARM_ROOT_BONES[0]) == 0 ? 0 : (wcscmp(name, ARM_ROOT_BONES[1]) == 0 ? 1 : -1);
			if (side >= 0) {
				isArm[boneNo] = true;
				m_armBones[side].push_back(boneNo);
				break;
			}
		}
	}
	m_hiddenBones.clear();
	for (int boneNo = 0; boneNo < numBones; boneNo++) {
		if (!isArm[boneNo]) {
			m_hiddenBones.push_back(boneNo);
		}
	}
	// 握るときに曲げる指の骨（指ごとに、付け根から指先へ3本）。
	const wchar_t* const sideNames[2] = { L"L", L"R" };
	const wchar_t* const fingerNames[FINGER_COUNT] = { L"Index", L"Middle", L"Ring", L"Little", L"Thumb" };
	for (int side = 0; side < 2; side++) {
		wchar_t handName[64];
		swprintf_s(handName, L"J_Bip_%s_Hand", sideNames[side]);
		m_handBone[side] = skeleton.FindBoneID(handName);
		for (int f = 0; f < FINGER_COUNT; f++) {
			for (int j = 0; j < 3; j++) {
				wchar_t name[64];
				swprintf_s(name, L"J_Bip_%s_%s%d", sideNames[side], fingerNames[f], j + 1);
				m_fingerBones[side][f][j] = skeleton.FindBoneID(name);
			}
		}
	}

	// ボーンのワールド行列を計算したあとで、腕以外の骨を、その骨の位置で大きさ 0 にする。
	// 体・頭・髪の頂点は骨の位置の1点に集まって見えなくなる。肩の近くで腕と体の両方に重みがある頂点は、
	// 体の骨の側へ引っ張られるが、一人称では画面の外になる。
	if (getenv("FP_TEST_NOHIDE")) { return; } // FP_TEST_TEMP
	skeleton.SetOnPostUpdateWorldMatrix([this](Skeleton& sk) {
		for (int boneNo : m_hiddenBones) {
			Bone* bone = sk.GetBone(boneNo);
			Matrix collapsed = bone->GetWorldMatrix();
			for (int r = 0; r < 3; r++) {
				collapsed.m[r][0] = 0.0f;
				collapsed.m[r][1] = 0.0f;
				collapsed.m[r][2] = 0.0f;
			}
			bone->SetWorldMatrix(collapsed);
		}
		CurlFingers(sk);
		SpreadArms(sk);
	});
}

void FirstPersonArms::CurlFingers(Skeleton& skeleton)
{
	float curlDegs[3] = { FINGER_CURL_DEGS[0], FINGER_CURL_DEGS[1], FINGER_CURL_DEGS[2] }, thumbDeg = THUMB_CURL_DEG; // FP_TEST_TEMP
	if (const char* e = getenv("FP_TEST_CURL")) { sscanf_s(e, "%f,%f,%f,%f", &curlDegs[0], &curlDegs[1], &curlDegs[2], &thumbDeg); } // FP_TEST_TEMP
	for (int side = 0; side < 2; side++) {
		// 握り具合（1 で握りこぶし、0 で手を開く）。
		const float w = m_grip[side];
		if (w <= 0.0f) {
			continue;
		}
		if (m_handBone[side] < 0) {
			continue;
		}
		// 人差し指〜小指: 付け根の関節から順に、その関節より先の骨をまとめて、その関節を中心に曲げる。
		// 曲げる軸は、その関節で今曲がっている面の軸（手前の骨の向きと先の骨の向きの外積）。
		// 骨の行列の軸をそのまま使うと、指の曲がる向きとずれて、曲げた分の多くがねじれになるため。
		// 指先の関節（j = 2）は先の骨が無いので、ひとつ手前の関節と同じ面で曲げる。
		for (int f = 0; f < THUMB; f++) {
			const int* chain = m_fingerBones[side][f];
			if (chain[0] < 0 || chain[1] < 0 || chain[2] < 0) {
				continue;
			}
			Vector3 axis = Vector3::Zero;
			for (int j = 0; j < 3; j++) {
				if (j < 2) {
					const Vector3 before = BonePosition(skeleton, chain[j]) - BonePosition(skeleton, j == 0 ? m_handBone[side] : chain[j - 1]);
					const Vector3 after = BonePosition(skeleton, chain[j + 1]) - BonePosition(skeleton, chain[j]);
					Vector3 bendAxis;
					bendAxis.Cross(before, after);
					if (bendAxis.LengthSq() > FLT_EPSILON) {
						axis = bendAxis;
					}
				}
				if (axis.LengthSq() <= FLT_EPSILON) {
					continue;
				}
				RotateChain(skeleton, chain, j, axis, curlDegs[j] * w);
			}
		}
		PlaceThumb(skeleton, side, w, thumbDeg);
	}
}

void FirstPersonArms::PlaceThumb(Skeleton& skeleton, int side, float grip, float tipCurlDeg)
{
	// 握りこぶしの親指は、手のひらの中に入れず、曲げた人差し指と中指の中節（第二関節と第一関節の間）の外側に重ねる。
	// 親指の先の目標をそこに置き、付け根側の関節から順に、親指の先が目標へ向くように回して合わせる（CCD の IK）。
	const int* thumb = m_fingerBones[side][THUMB];
	const int* index = m_fingerBones[side][0];
	const int* middle = m_fingerBones[side][1];
	if (thumb[0] < 0 || thumb[1] < 0 || thumb[2] < 0 || index[2] < 0 || middle[2] < 0) {
		return;
	}
	// 先に、親指の先の関節を、ひとつ手前の関節と同じ面で少し曲げておく（指先が自然に丸まるように）。
	{
		const Vector3 before = BonePosition(skeleton, thumb[1]) - BonePosition(skeleton, thumb[0]);
		const Vector3 after = BonePosition(skeleton, thumb[2]) - BonePosition(skeleton, thumb[1]);
		Vector3 axis;
		axis.Cross(before, after);
		if (axis.LengthSq() > FLT_EPSILON) {
			RotateChain(skeleton, thumb, 2, axis, tipCurlDeg * grip);
		}
	}

	// 目標: 人差し指と中指の中節の真ん中を、手のひらの中心から離れる向きへ、指の太さの分だけ外へ出した位置。
	const Vector3 indexMid = (BonePosition(skeleton, index[1]) + BonePosition(skeleton, index[2])) * 0.5f;
	const Vector3 middleMid = (BonePosition(skeleton, middle[1]) + BonePosition(skeleton, middle[2])) * 0.5f;
	Vector3 target = (indexMid + middleMid) * 0.5f;
	const Vector3 palmCenter = (BonePosition(skeleton, m_handBone[side]) + BonePosition(skeleton, middle[0])) * 0.5f;
	Vector3 outward = target - palmCenter;
	if (outward.LengthSq() > FLT_EPSILON) {
		outward.Normalize();
		target += outward * THUMB_OUTSIDE_OFFSET;
	}

	// 親指の先: 先の骨の向きに、先の骨の長さのぶん伸ばした位置（先の骨の先端の骨が無いので、こうして決める）。
	const float tipLength = (BonePosition(skeleton, thumb[2]) - BonePosition(skeleton, thumb[1])).Length() * THUMB_TIP_LENGTH_RATE;
	auto thumbTip = [&]() {
		Vector3 direction = BonePosition(skeleton, thumb[2]) - BonePosition(skeleton, thumb[1]);
		direction.Normalize();
		return BonePosition(skeleton, thumb[2]) + direction * tipLength;
	};
	for (int iteration = 0; iteration < THUMB_IK_ITERATIONS; iteration++) {
		// 第二関節（MP）、付け根（CM）の順に回す。
		for (int j = 1; j >= 0; j--) {
			const Vector3 pivot = BonePosition(skeleton, thumb[j]);
			Vector3 toTip = thumbTip() - pivot;
			Vector3 toTarget = target - pivot;
			if (toTip.LengthSq() <= FLT_EPSILON || toTarget.LengthSq() <= FLT_EPSILON) {
				continue;
			}
			toTip.Normalize();
			toTarget.Normalize();
			Vector3 axis;
			axis.Cross(toTip, toTarget);
			if (axis.LengthSq() <= FLT_EPSILON) {
				continue;
			}
			const float deg = acosf(max(-1.0f, min(1.0f, toTip.Dot(toTarget)))) * 57.2958f;
			// 握り具合のぶんだけ近づける（手を開くときは、元の親指の形へ戻る）。
			RotateChain(skeleton, thumb, j, axis, deg * grip);
		}
	}
	if (const char* path = getenv("FP_TEST_THUMBLOG")) { // FP_TEST_TEMP
		static int s_count = 0; if (++s_count == 240) { FILE* fp = nullptr; fopen_s(&fp, path, "a"); if (fp) {
			const Vector3 tip = thumbTip(); const Vector3 hand = BonePosition(skeleton, m_handBone[side]);
			Vector3 palmFwd = BonePosition(skeleton, middle[0]) - hand; const float palmLen = palmFwd.Length(); palmFwd.Normalize();
			fprintf(fp, "side%d tip-target=%.2f tip-indexMid=%.2f tip-palmCenter=%.2f target-palmCenter=%.2f palmLen=%.2f thumb1-tip=%.2f\n", side,
				(tip - target).Length(), (tip - indexMid).Length(), (tip - palmCenter).Length(), (target - palmCenter).Length(), palmLen,
				(tip - BonePosition(skeleton, thumb[0])).Length());
			fclose(fp); } }
	}
}

Vector3 FirstPersonArms::BonePosition(Skeleton& skeleton, int boneNo)
{
	const Matrix& m = skeleton.GetBone(boneNo)->GetWorldMatrix();
	return Vector3(m.m[3][0], m.m[3][1], m.m[3][2]);
}

void FirstPersonArms::RotateChain(Skeleton& skeleton, const int* chain, int from, Vector3 axis, float deg)
{
	// chain[from] の関節を中心に、chain[from] から指先までの骨をまとめて、axis まわりに deg 度回す。
	axis.Normalize();
	Quaternion q;
	q.SetRotationDeg(axis, deg);
	Matrix rotation, toOrigin, toPivot, aroundJoint;
	rotation.MakeRotationFromQuaternion(q);
	const Vector3 pivot = BonePosition(skeleton, chain[from]);
	toOrigin.MakeTranslation(pivot * -1.0f);
	toPivot.MakeTranslation(pivot);
	aroundJoint.Multiply(toOrigin, rotation);
	aroundJoint.Multiply(aroundJoint, toPivot);
	for (int k = from; k < 3; k++) {
		Bone* bone = skeleton.GetBone(chain[k]);
		Matrix world;
		world.Multiply(bone->GetWorldMatrix(), aroundJoint);
		bone->SetWorldMatrix(world);
	}
}

void FirstPersonArms::SpreadArms(Skeleton& skeleton)
{
	ArmPose idlePose = IDLE_POSE, castPose = CAST_POSE; // FP_TEST_TEMP
	if (const char* e = getenv("FP_TEST_SPREAD")) { sscanf_s(e, "%f,%f,%f", &idlePose.spreadDeg, &idlePose.raiseDeg, &idlePose.sideShift); } // FP_TEST_TEMP
	if (const char* e = getenv("FP_TEST_CAST")) { sscanf_s(e, "%f,%f,%f,%f,%f", &castPose.spreadDeg, &castPose.raiseDeg, &castPose.sideShift, &castPose.drop, &castPose.push); } // FP_TEST_TEMP
	const Matrix& cameraRotationMatrix = g_camera3D->GetCameraRotation();
	const Vector3 cameraUp(cameraRotationMatrix.m[1][0], cameraRotationMatrix.m[1][1], cameraRotationMatrix.m[1][2]);
	for (int side = 0; side < 2; side++) {
		if (m_upperArmBone[side] < 0) {
			continue;
		}
		// 構えと撃つときの置き方を、この腕の混ぜ具合で混ぜる。
		const float w = m_castWeight[side];
		const float spreadDeg = idlePose.spreadDeg + (castPose.spreadDeg - idlePose.spreadDeg) * w;
		const float raiseDeg = idlePose.raiseDeg + (castPose.raiseDeg - idlePose.raiseDeg) * w;
		const float sideShift = idlePose.sideShift + (castPose.sideShift - idlePose.sideShift) * w;
		const float drop = idlePose.drop + (castPose.drop - idlePose.drop) * w;
		const float push = idlePose.push + (castPose.push - idlePose.push) * w;
		// 左腕（side 0）は画面の左へ、右腕は右へ回す（カメラの上の軸まわりに、左腕は +、右腕は -）。
		// そのあと、カメラの右の軸まわりに - へ回して持ち上げる。
		Quaternion spread;
		spread.SetRotationDeg(cameraUp, side == 0 ? spreadDeg : -spreadDeg);
		Quaternion raise;
		raise.SetRotationDeg(g_camera3D->GetRight(), -raiseDeg);
		Matrix rotation;
		rotation.MakeRotationFromQuaternion(spread * raise);

		// 肩（上腕の付け根）を中心に回す: 肩を原点へ動かす → 回す → 肩の位置へ戻す。
		const Matrix& upperArm = skeleton.GetBone(m_upperArmBone[side])->GetWorldMatrix();
		const Vector3 pivot(upperArm.m[3][0], upperArm.m[3][1], upperArm.m[3][2]);
		// 最後に、腕ごと画面の外側へずらして下げる（肘を画面の端の外に置き、前腕が端から真ん中へ斜めに伸びるようにする）。
		// このゲームでは左の骨（side 0）が画面の右に映るので、side 0 はカメラの右へ、side 1 は左へずらす。
		Matrix toOrigin, toPivot, toSide, aroundPivot;
		toOrigin.MakeTranslation(pivot * -1.0f);
		toPivot.MakeTranslation(pivot);
		toSide.MakeTranslation(g_camera3D->GetRight() * (side == 0 ? sideShift : -sideShift) - cameraUp * drop + g_camera3D->GetForward() * push);
		aroundPivot.Multiply(toOrigin, rotation);
		aroundPivot.Multiply(aroundPivot, toPivot);
		aroundPivot.Multiply(aroundPivot, toSide);
		for (int boneNo : m_armBones[side]) {
			Bone* bone = skeleton.GetBone(boneNo);
			Matrix world;
			world.Multiply(bone->GetWorldMatrix(), aroundPivot);
			bone->SetWorldMatrix(world);
		}
	}
}

void FirstPersonArms::Reset(const Player& player)
{
	// 三人称の間に撃った分は数えない。
	const MagicCaster& caster = player.GetMagicCaster();
	m_lastMainMagic = caster.GetCurrentMagic();
	m_lastSubMagic = caster.GetSubMagic();
	m_lastMainCastCount = m_lastMainMagic != nullptr ? m_lastMainMagic->GetCastCount() : 0;
	m_lastSubCastCount = m_lastSubMagic != nullptr ? m_lastSubMagic->GetCastCount() : 0;
	m_releaseTimer = 0.0f;
	for (int side = 0; side < 2; side++) {
		m_castWeight[side] = 0.0f;
		m_grip[side] = 1.0f;
	}
	Play(enClip_Idle, 0.0f);
	UpdateTransform();
	m_model.Update();
}

void FirstPersonArms::Update(const Player& player)
{
	if (getenv("FP_TEST_NOANIM")) { UpdateTransform(); m_model.Update(); return; } // FP_TEST_TEMP
	if (const char* c = getenv("FP_TEST_CLIP")) { const char* t = getenv("FP_TEST_TIME"); m_clip = (EnClip)atoi(c); m_releaseTimer = getenv("FP_TEST_RELEASE") ? 1.0f : 0.0f; for (int s = 0; s < 2; s++) { m_castWeight[s] = GetCastWeightTarget(s); m_grip[s] = GetGripTarget(s); } m_model.PlayAnimationAtTime(atoi(c), t ? (float)atof(t) : 0.0f); UpdateTransform(); m_model.Update(); return; } // FP_TEST_TEMP
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
	UpdateClip(player, deltaTime);
	// 構えと撃つときの置き方を、急に変わらないよう、少しずつ混ぜて切り替える。
	const float rate = min(1.0f, deltaTime * POSE_BLEND_SPEED);
	for (int side = 0; side < 2; side++) {
		m_castWeight[side] += (GetCastWeightTarget(side) - m_castWeight[side]) * rate;
		// 手は、開くときは速く（開いた瞬間に魔法が出る）、握り直すときはゆっくり。
		const float gripTarget = GetGripTarget(side);
		const float gripSpeed = gripTarget < m_grip[side] ? GRIP_OPEN_SPEED : GRIP_CLOSE_SPEED;
		m_grip[side] += (gripTarget - m_grip[side]) * min(1.0f, deltaTime * gripSpeed);
	}
	if (const char* path = getenv("FP_TEST_STATE")) { // FP_TEST_TEMP
		static float s_t = 0.0f; static float s_next = 0.0f; s_t += deltaTime;
		if (s_t >= s_next) { s_next = s_t + 0.1f; FILE* fp = nullptr; fopen_s(&fp, path, "a"); if (fp) {
			fprintf(fp, "t=%.2f clip=%d cast=(%.2f,%.2f) grip=(%.2f,%.2f) release=%.2f playing=%d\n", s_t, (int)m_clip, m_castWeight[0], m_castWeight[1], m_grip[0], m_grip[1], m_releaseTimer, (int)m_model.IsPlayingAnimation()); fclose(fp); } }
	}
	UpdateTransform();
	m_model.Update();
}

float FirstPersonArms::GetCastWeightTarget(int side) const
{
	switch (m_clip) {
	case enClip_ChargeStart:
	case enClip_Charge:
		return 1.0f;  // 炎は両手（撃って手を開いている間も、上げたまま）。
	case enClip_CastRight:
		// 氷は右手だけ。このゲームでは左の骨（side 0）が画面の右に映る。
		return side == 0 ? 1.0f : 0.0f;
	default:
		return 0.0f;
	}
}

float FirstPersonArms::GetGripTarget(int side) const
{
	// 炎を撃った直後は両手を開く。溜めている間は握りこぶしのまま。
	if (m_releaseTimer > 0.0f) {
		return 0.0f;
	}
	// 氷を撃つ右手は開く。このゲームでは左の骨（side 0）が画面の右に映る。
	if (m_clip == enClip_CastRight && side == 0) {
		return 0.0f;
	}
	return 1.0f;
}

void FirstPersonArms::Draw(RenderContext& rc)
{
	if (const char* path = getenv("FP_TEST_FINGERS")) { // FP_TEST_TEMP: 関節ごとの曲がり（前の骨と次の骨の向きの角度）
		static int s_frame3 = 0; if (++s_frame3 == 120) { FILE* fp = nullptr; fopen_s(&fp, path, "a"); if (fp) {
			Skeleton& sk = m_model.GetModelRender().GetSkeleton();
			auto pos = [&](int no) { const Matrix& w = sk.GetBone(no)->GetWorldMatrix(); return Vector3(w.m[3][0], w.m[3][1], w.m[3][2]); };
			for (int side = 0; side < 2; side++) { for (int f = 0; f < FINGER_COUNT; f++) {
				const int hand = sk.FindBoneID(side == 0 ? L"J_Bip_L_Hand" : L"J_Bip_R_Hand");
				int chain[4] = { hand, m_fingerBones[side][f][0], m_fingerBones[side][f][1], m_fingerBones[side][f][2] };
				fprintf(fp, "side%d finger%d:", side, f);
				// 関節 j の曲がり = (j-1 → j の向き) と (j → j+1 の向き) の角度。指先は骨の X 軸の向きで代わりにする。
				for (int j = 1; j < 4; j++) {
					Vector3 a = pos(chain[j]) - pos(chain[j - 1]);
					Vector3 b;
					if (j < 3) { b = pos(chain[j + 1]) - pos(chain[j]); }
					else { const Matrix& w = sk.GetBone(chain[3])->GetWorldMatrix(); b = Vector3(w.m[0][0], w.m[0][1], w.m[0][2]); }
					a.Normalize(); b.Normalize();
					fprintf(fp, " j%d=%.0f", j, acosf(max(-1.0f, min(1.0f, a.Dot(b)))) * 57.2958f);
				}
				fprintf(fp, "\n"); } }
			fclose(fp); } }
	}
	if (const char* path = getenv("FP_TEST_HANDS")) { // FP_TEST_TEMP
		static int s_frame2 = 0; if (++s_frame2 == 120) { FILE* fp = nullptr; fopen_s(&fp, path, "a"); if (fp) {
			Skeleton& sk = m_model.GetModelRender().GetSkeleton();
			const Matrix& cr = g_camera3D->GetCameraRotation(); const Vector3 up(cr.m[1][0], cr.m[1][1], cr.m[1][2]);
			for (const wchar_t* n : { L"J_Bip_L_UpperArm", L"J_Bip_L_LowerArm", L"J_Bip_L_Hand", L"J_Bip_R_UpperArm", L"J_Bip_R_LowerArm", L"J_Bip_R_Hand" }) {
				const Matrix& w = sk.GetBone(sk.FindBoneID(n))->GetWorldMatrix();
				Vector3 d(w.m[3][0], w.m[3][1], w.m[3][2]); d -= g_camera3D->GetPosition();
				fprintf(fp, "%ls right %.1f up %.1f fwd %.1f\n", n, d.Dot(g_camera3D->GetRight()), d.Dot(up), d.Dot(g_camera3D->GetForward())); }
			fprintf(fp, "---\n"); fclose(fp); } }
	}
	if (const char* path = getenv("FP_TEST_LOG")) { // FP_TEST_TEMP
		static int s_frame = 0; if (++s_frame % 60 == 0) { FILE* fp = nullptr; fopen_s(&fp, path, "a"); if (fp) {
			Skeleton& sk = m_model.GetModelRender().GetSkeleton();
			const wchar_t* names[] = { L"J_Bip_C_Hips", L"J_Bip_C_Head", L"J_Bip_L_Hand", L"Root" };
			for (auto n : names) { int id = sk.FindBoneID(n); if (id < 0) { fprintf(fp, "%ls missing\n", n); continue; }
				const Matrix& w = sk.GetBone(id)->GetWorldMatrix(); const Matrix* bm = sk.GetBoneMatricesTopAddress();
				fprintf(fp, "%ls id%d world(%.1f %.1f %.1f) scaleX %.3f boneMat t(%.1f %.1f %.1f)\n", n, id, w.m[3][0], w.m[3][1], w.m[3][2], sqrtf(w.m[0][0]*w.m[0][0]+w.m[0][1]*w.m[0][1]+w.m[0][2]*w.m[0][2]), bm[id].m[3][0], bm[id].m[3][1], bm[id].m[3][2]); }
			const Vector3& c = g_camera3D->GetPosition(); fprintf(fp, "camera(%.1f %.1f %.1f) playing %d\n---\n", c.x, c.y, c.z, (int)m_model.IsPlayingAnimation()); fclose(fp); } }
	}
	m_model.Draw(rc);
}

void FirstPersonArms::UpdateClip(const Player& player, float deltaTime)
{
	const MagicCaster& caster = player.GetMagicCaster();
	const MagicBase* mainMagic = caster.GetCurrentMagic();
	const MagicBase* subMagic = caster.GetSubMagic();
	const bool isMainCast = IsCastThisFrame(mainMagic, m_lastMainMagic, m_lastMainCastCount);
	const bool isSubCast = IsCastThisFrame(subMagic, m_lastSubMagic, m_lastSubCastCount);
	const bool isCharging = subMagic != nullptr && subMagic->IsCharging();
	const bool isChargeClip = m_clip == enClip_ChargeStart || m_clip == enClip_Charge;

	// 両手の動き（炎）を、右手の動き（氷）より先に見る。溜めている間に氷を撃っても、両手は上げたまま。
	if (isSubCast) {
		// 炎を撃った: 上げた手をその場で開き（GetGripTarget）、少し見せてから最初の構えに戻る。
		m_releaseTimer = RELEASE_OPEN_SECONDS;
		return;
	}
	if (m_releaseTimer > 0.0f) {
		m_releaseTimer -= deltaTime;
		if (m_releaseTimer <= 0.0f) {
			m_releaseTimer = 0.0f;
			Play(enClip_Idle, TO_IDLE_SECONDS);
		}
		return;
	}
	if (isCharging) {
		if (!isChargeClip) {
			Play(enClip_ChargeStart, TO_CHARGE_START_SECONDS);
		}
		else if (m_clip == enClip_ChargeStart && !m_model.IsPlayingAnimation()) {
			Play(enClip_Charge, TO_CHARGE_SECONDS);
		}
		return;
	}
	if (isChargeClip) {
		// 撃たずに溜めをやめた（凍結した、など）。
		Play(enClip_Idle, TO_IDLE_SECONDS);
		return;
	}
	if (isMainCast) {
		Play(enClip_CastRight, TO_CAST_SECONDS);
		return;
	}
	if (m_clip != enClip_Idle && !m_model.IsPlayingAnimation()) {
		// 突き出すのが終わったら、構えに戻る。
		Play(enClip_Idle, TO_IDLE_SECONDS);
	}
}

void FirstPersonArms::Play(EnClip clip, float interpolateTime)
{
	m_clip = clip;
	m_model.PlayAnimation(clip, interpolateTime);
}

void FirstPersonArms::UpdateTransform()
{
	// カメラと同じ向きにして、カメラから見た決まった位置に置く。上下を向いても、手は画面の同じところに映る。
	const Vector3& cameraPosition = g_camera3D->GetPosition();
	const Matrix& cameraRotationMatrix = g_camera3D->GetCameraRotation();
	const Vector3 cameraUp(cameraRotationMatrix.m[1][0], cameraRotationMatrix.m[1][1], cameraRotationMatrix.m[1][2]);

	Vector3 ofs = ARMS_OFFSET; float yaw = 0.0f; // FP_TEST_TEMP
	if (const char* e = getenv("FP_TEST_OFS")) { sscanf_s(e, "%f,%f,%f,%f", &ofs.x, &ofs.y, &ofs.z, &yaw); } // FP_TEST_TEMP
	Vector3 position = cameraPosition;
	position += g_camera3D->GetRight() * ofs.x;
	position += cameraUp * ofs.y;
	position += g_camera3D->GetForward() * ofs.z;

	Quaternion rotation;
	rotation.SetRotation(cameraRotationMatrix);
	{ Quaternion y; y.SetRotationDegY(yaw); rotation = y * rotation; } // FP_TEST_TEMP
	m_model.SetTransform(position, rotation);
}

bool FirstPersonArms::IsCastThisFrame(const MagicBase* magic, const MagicBase*& lastMagic, int& lastCount)
{
	const int count = magic != nullptr ? magic->GetCastCount() : 0;
	const bool isCast = magic == lastMagic && count > lastCount;
	lastMagic = magic;
	lastCount = count;
	return isCast;
}
