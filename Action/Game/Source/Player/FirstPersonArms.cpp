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
	// 腕の色に掛ける色。ボス戦のステージの月の光（Game.cpp の BOSS_MOON_COLOR）が強い青で、目の前に大きく映る腕が
	// 青白く見えるので、腕だけ青と緑を減らして打ち消す。月の光が当たる前腕は青く、当たらない手の甲は
	// 夜空からの明かりで青緑になるので、その間をとって、どちらも肌の色合いに近づくようにした。
	// 青と緑を減らすだけ（明るくはしない）なので、少し暗くなる。
	const Vector4 ARMS_MUL_COLOR = Vector4(1.0f, 0.62f, 0.48f, 1.0f);
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
	// a と b の置き方を、rate（0 で a、1 で b）で混ぜる。
	ArmPose BlendPose(const ArmPose& a, const ArmPose& b, float rate)
	{
		auto lerp = [rate](float x, float y) { return x + (y - x) * rate; };
		return {
			lerp(a.spreadDeg, b.spreadDeg), lerp(a.raiseDeg, b.raiseDeg),
			lerp(a.sideShift, b.sideShift), lerp(a.drop, b.drop), lerp(a.push, b.push),
		};
	}
	// 構え: 手を画面の下 1/3 の左右に置く。
	const ArmPose IDLE_POSE = { -25.0f, 55.0f, 12.0f, 0.0f, 0.0f };
	// 氷を撃つとき（撃つ腕だけ。右と左で交互）: 炎を放つときと同じく腕を画面の端から内へ向けて前へ伸ばし、
	// 開いた手を、撃つ位置（PlayerInputSource の FIRST_PERSON_HAND_OFFSET）のあたりに置く。
	const ArmPose CAST_POSE = { -10.0f, 46.0f, 12.0f, 0.0f, 30.0f };
	// 氷を撃ったあと、手を開いて突き出したまま見せる時間（秒）。そのあと構えに戻る。
	constexpr float CAST_OPEN_SECONDS = 0.2f;
	// 構えの拳の手のひらを、内側へ向けた向きから、カメラの方へ傾ける角度（度）。
	constexpr float FIST_TOWARD_CAMERA_DEG = 15.0f;
	// 構えの拳の上を、内側（画面の下の真ん中の HP バーの方）へ倒す角度（度）。
	constexpr float FIST_ROLL_DEG = 45.0f;
	// 手のひらの向きの裏表。人差し指と小指の付け根で張る面の法線（手首から人差し指 × 手首から小指）に掛ける。
	// 左右の手は鏡写しなので逆になる（ゲームで炎を放つ手を見て決めた。左の骨（side 0）が画面の右の手）。
	const float PALM_SIGN[2] = { -1.0f, 1.0f };
	// 炎を溜めるとき（溜め始め・溜め・放つ。両腕）: 画面の下に沈めた火の玉（PlayerInputSource の FIRST_PERSON_CHARGE_OFFSET）を
	// 左右から開いた手で挟むように、手を火の玉の左右の、画面の下 1/3 に置く（氷を撃つときより低く、少し外へ開く）。
	const ArmPose CHARGE_POSE = { 2.0f, 18.0f, 0.0f, 0.0f, 10.0f };
	// 炎を放つとき（両腕）: 構えと同じく腕を画面の左右の外から内へ向け、前へ伸ばして、両手を照準のそばへ寄せて火の玉を押し出す。
	// 手のひらは AimHands で前へ向け、両手の間は RELEASE_HAND_GAP にそろえる。
	const ArmPose RELEASE_POSE = { -30.0f, 32.0f, 12.0f, 0.0f, 32.0f };
	// 炎を放つときの、両手の内側の端の間（握りこぶしの半分くらい。骨格の単位）。
	// 指の骨の位置で測るので、指先の肉の分を足してある。
	constexpr float RELEASE_HAND_GAP = 10.0f;
	// 炎を放つときに、指先を真上から内側へ傾ける角度（度）。
	constexpr float RELEASE_FINGER_TILT_DEG = 25.0f;
	// 炎を放つときの指の形（ShapeReleaseFingers）。
	// 関節ごとに、手のひらの側へ曲げる角度（度。付け根・第二関節・第一関節の順。手前の関節の分に足していく）。
	const float RELEASE_FINGER_CURL_DEGS[3] = { 8.0f, 12.0f, 8.0f };
	constexpr float RELEASE_FINGER_CLOSE_RATE = 0.5f;  // 人差し指〜小指の開きを、中指の向きへ寄せる割合（1 で全部そろう）。
	constexpr float RELEASE_THUMB_SPREAD_DEG = 45.0f;  // 親指を、人差し指の向きから親指の側へ開く角度（度。手のひらの面の上で）。
	constexpr float RELEASE_LITTLE_SPREAD_DEG = 10.0f;  // 小指を、中指から離れる側（外側）へさらに開く角度（度）。
	// 付け根の関節だけ、指ごとにさらに曲げる角度（度。人差し指・中指・薬指・小指の順）。
	// このモデルは小指の付け根が薬指と同じ高さにあり、指先の高さがそろって見えるので、薬指と小指を多めに曲げて指先を下げる
	// （力を抜いて開いた人の手も、小指の側ほど曲がっている）。
	const float RELEASE_FINGER_BASE_EXTRA_CURL_DEGS[4] = { 0.0f, 0.0f, 5.0f, 15.0f };
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
	constexpr float RELEASE_OPEN_SECONDS = 0.3f;
	// 構えと撃つときの置き方を混ぜる速さ（大きいほど速く切り替わる）。
	constexpr float POSE_BLEND_SPEED = 10.0f;
	// 炎を放つときに、両腕を前へ伸ばす速さ。撃った瞬間に伸びきるよう、ほかの置き方より速くする。
	constexpr float RELEASE_PUSH_SPEED = 30.0f;

	// カメラから見て、腕のモデルの足元を置く位置（x: 右、y: 上、z: 前）。
	// 腕のモデルの目の高さ（約 157）より下げ、手前に寄せて、手が大きく画面の下の方に映るようにする。
	const Vector3 ARMS_OFFSET = Vector3(0.0f, -183.0f, -18.0f);

	// アニメーションを切り替えるときの補間（秒）。
	constexpr float TO_IDLE_SECONDS = 0.15f;
	constexpr float TO_CHARGE_START_SECONDS = 0.08f;
	constexpr float TO_CHARGE_SECONDS = 0.1f;
}

void FirstPersonArms::Init()
{
	CharacterModelDesc desc;
	desc.modelPath = MODEL_PATH;
	desc.upAxis = enModelUpAxisZ;
	desc.scale = MODEL_SCALE;
	desc.isShadowCaster = false;  // 体を描いていないので、腕だけの影が床に落ちないようにする。
	desc.clips.resize(enClip_Num);
	const std::string dir = CLIP_DIR;
	desc.clips[enClip_Idle] = { dir + "FpIdle.fbx", true };
	desc.clips[enClip_ChargeStart] = { dir + "FpChargeStart.fbx", false };
	desc.clips[enClip_Charge] = { dir + "FpCharge.fbx", true };
	m_model.Init(desc);
	m_model.GetModelRender().SetMulColor(ARMS_MUL_COLOR);
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
		AimHands(sk);
	});

	// 手首から先の骨（手と、その子の指の骨）。手首を中心に手の向きを変えるのに使う。
	for (int side = 0; side < 2; side++) {
		m_handTreeBones[side].clear();
		if (m_handBone[side] < 0) {
			continue;
		}
		for (int boneNo : m_armBones[side]) {
			for (int no = boneNo; no >= 0; no = skeleton.GetBone(no)->GetParentBoneNo()) {
				if (no == m_handBone[side]) {
					m_handTreeBones[side].push_back(boneNo);
					break;
				}
			}
		}
	}
}

Vector3 FirstPersonArms::AimHand(Skeleton& skeleton, int side, const Vector3& palmTarget, const Vector3& fingersTarget, float w)
{
	const int* index = m_fingerBones[side][0];
	const int* middle = m_fingerBones[side][1];
	const int* little = m_fingerBones[side][LITTLE];
	if (w <= 0.0f || m_handBone[side] < 0 || index[0] < 0 || middle[0] < 0 || little[0] < 0) {
		return palmTarget;
	}
	const Vector3 wrist = BonePosition(skeleton, m_handBone[side]);
	// 手のひらの向き: 人差し指と小指の付け根で張る面の法線（指の付け根は手の骨に付いているので、指の曲げ方では変わらない）。
	Vector3 palm;
	palm.Cross(BonePosition(skeleton, index[0]) - wrist, BonePosition(skeleton, little[0]) - wrist);
	if (palm.LengthSq() <= FLT_EPSILON) {
		return palmTarget;
	}
	palm.Normalize();
	palm *= PALM_SIGN[side];
	// ① 手のひらを palmTarget へ向ける。
	Quaternion q = Quaternion::Identity;
	Vector3 axis;
	axis.Cross(palm, palmTarget);
	if (axis.LengthSq() > FLT_EPSILON) {
		axis.Normalize();
		const float deg = acosf(max(-1.0f, min(1.0f, palm.Dot(palmTarget)))) * 57.2958f;
		q.SetRotationDeg(axis, deg * w);
	}
	// ② 続けて、手のひらの向きを軸に回し、手首から中指の付け根への向きを fingersTarget へ向ける。
	Vector3 fingers = BonePosition(skeleton, middle[0]) - wrist;
	q.Apply(fingers);
	fingers -= palmTarget * fingers.Dot(palmTarget);
	Vector3 target = fingersTarget - palmTarget * fingersTarget.Dot(palmTarget);
	if (fingers.LengthSq() > FLT_EPSILON && target.LengthSq() > FLT_EPSILON) {
		fingers.Normalize();
		target.Normalize();
		Vector3 twistAxis;
		twistAxis.Cross(fingers, target);
		if (twistAxis.LengthSq() > FLT_EPSILON) {
			twistAxis.Normalize();
			const float twistDeg = acosf(max(-1.0f, min(1.0f, fingers.Dot(target)))) * 57.2958f;
			Quaternion twist;
			twist.SetRotationDeg(twistAxis, twistDeg * w);
			q.Multiply(q, twist);
		}
	}
	// 手首を中心に、手首から先の骨をまとめて回す。
	Matrix rotation, toOrigin, toPivot, aroundWrist;
	rotation.MakeRotationFromQuaternion(q);
	toOrigin.MakeTranslation(wrist * -1.0f);
	toPivot.MakeTranslation(wrist);
	aroundWrist.Multiply(toOrigin, rotation);
	aroundWrist.Multiply(aroundWrist, toPivot);
	for (int boneNo : m_handTreeBones[side]) {
		Bone* bone = skeleton.GetBone(boneNo);
		Matrix world;
		world.Multiply(bone->GetWorldMatrix(), aroundWrist);
		bone->SetWorldMatrix(world);
	}
	q.Apply(palm);
	return palm;
}

void FirstPersonArms::AimHands(Skeleton& skeleton)
{
	const Vector3 cameraRight = g_camera3D->GetRight();
	const Vector3 cameraForward = g_camera3D->GetForward();
	const Matrix& cameraRotationMatrix = g_camera3D->GetCameraRotation();
	const Vector3 cameraUp(cameraRotationMatrix.m[1][0], cameraRotationMatrix.m[1][1], cameraRotationMatrix.m[1][2]);
	for (int side = 0; side < 2; side++) {
		// このゲームでは左の骨（side 0）が画面の右に映るので、内側はカメラの左。
		const Vector3 inward = cameraRight * (side == 0 ? -1.0f : 1.0f);
		const float openWeight = max(m_castWeight[side], m_releaseWeight[side]);
		// ① 構えの拳: ファイティングポーズのように、手のひらを内側（もう片方の拳の方）へ向け、指の付け根の関節を上へ向ける。
		// こうすると、親指の側（親指の第一関節）がカメラの方を向く。手のひらは、少しだけカメラの方へも傾ける。
		// そのまま、カメラの前の向きを軸に、拳の上を内側（画面の下の真ん中の HP バーの方）へ FIST_ROLL_DEG だけ傾ける。
		const float fistWeight = m_grip[side] * (1.0f - max(openWeight, m_chargeWeight[side]));
		if (fistWeight > 0.0f) {
			const float rad = FIST_TOWARD_CAMERA_DEG / 57.2958f;
			Vector3 palmTarget = inward * cosf(rad) - cameraForward * sinf(rad);
			Vector3 fingersTarget = cameraUp;
			Vector3 rollAxis;
			rollAxis.Cross(cameraUp, inward);  // この軸まわりに + へ回すと、上が内側へ倒れる。
			Quaternion roll;
			roll.SetRotationDeg(rollAxis, FIST_ROLL_DEG);
			roll.Apply(palmTarget);
			roll.Apply(fingersTarget);
			AimHand(skeleton, side, palmTarget, fingersTarget, fistWeight);
		}
		// ② 手を開いて撃つとき: 手のひらを前（照準の方）へ向け、指先を上へ（少し内側へ傾けて）向けて、指の形を作り直す。
		if (openWeight > 0.0f) {
			const float tilt = RELEASE_FINGER_TILT_DEG / 57.2958f;
			const Vector3 fingersTarget = cameraUp * cosf(tilt) + inward * sinf(tilt);
			const Vector3 palmNormal = AimHand(skeleton, side, cameraForward, fingersTarget, openWeight);
			ShapeReleaseFingers(skeleton, side, palmNormal, openWeight);
		}
	}

	// ② 両手の内側の端（指の骨でいちばん内にあるもの）の間を RELEASE_HAND_GAP にするよう、腕ごと左右へずらす。
	// このゲームでは左の骨（side 0）が画面の右に映る。
	const float w = min(m_releaseWeight[0], m_releaseWeight[1]);
	if (w <= 0.0f || m_handTreeBones[0].empty() || m_handTreeBones[1].empty()) {
		return;
	}
	float innerOfRight = FLT_MAX;   // 画面の右の手（side 0）の、いちばん左の位置。
	float innerOfLeft = -FLT_MAX;   // 画面の左の手（side 1）の、いちばん右の位置。
	for (int boneNo : m_handTreeBones[0]) {
		innerOfRight = min(innerOfRight, BonePosition(skeleton, boneNo).Dot(cameraRight));
	}
	for (int boneNo : m_handTreeBones[1]) {
		innerOfLeft = max(innerOfLeft, BonePosition(skeleton, boneNo).Dot(cameraRight));
	}
	const float shift = (RELEASE_HAND_GAP - (innerOfRight - innerOfLeft)) * 0.5f * w;
	for (int side = 0; side < 2; side++) {
		Matrix move;
		move.MakeTranslation(cameraRight * (side == 0 ? shift : -shift));
		for (int boneNo : m_armBones[side]) {
			Bone* bone = skeleton.GetBone(boneNo);
			Matrix world;
			world.Multiply(bone->GetWorldMatrix(), move);
			bone->SetWorldMatrix(world);
		}
	}
}

Vector3 FirstPersonArms::FingerSegmentDirection(Skeleton& skeleton, const int* chain, int j)
{
	if (j < 2) {
		Vector3 direction = BonePosition(skeleton, chain[j + 1]) - BonePosition(skeleton, chain[j]);
		direction.Normalize();
		return direction;
	}
	// 指先の骨は先に骨が無いので、ひとつ手前の骨で「骨の向き」になっている軸を探し、指先の骨の同じ軸を使う。
	const Vector3 before = FingerSegmentDirection(skeleton, chain, 1);
	const Matrix& m1 = skeleton.GetBone(chain[1])->GetWorldMatrix();
	const Matrix& m2 = skeleton.GetBone(chain[2])->GetWorldMatrix();
	int bestAxis = 0;
	float bestDot = 0.0f;
	for (int axis = 0; axis < 3; axis++) {
		Vector3 v(m1.m[axis][0], m1.m[axis][1], m1.m[axis][2]);
		v.Normalize();
		const float dot = v.Dot(before);
		if (fabsf(dot) > fabsf(bestDot)) {
			bestDot = dot;
			bestAxis = axis;
		}
	}
	Vector3 direction(m2.m[bestAxis][0], m2.m[bestAxis][1], m2.m[bestAxis][2]);
	direction.Normalize();
	return bestDot < 0.0f ? direction * -1.0f : direction;
}

void FirstPersonArms::ShapeReleaseFingers(Skeleton& skeleton, int side, Vector3 palmNormal, float w)
{
	// 溜めのクリップの開いた手は、指が手の甲の側へ反り、大きく開いている。そのままでは人の手にならないので、
	// 手のひらの面の上で、人差し指〜小指の向きを中指の向きへ寄せ（開きを小さくし）、
	// 関節ごとに手のひらの側へ少し曲げた向き（RELEASE_FINGER_CURL_DEGS）へ、付け根の関節から順に向け直す。
	const int* middle = m_fingerBones[side][1];
	if (middle[0] < 0 || middle[1] < 0) {
		return;
	}
	palmNormal.Normalize();
	auto onPalmPlane = [&palmNormal](Vector3 v) {
		v -= palmNormal * v.Dot(palmNormal);
		v.Normalize();
		return v;
	};
	const Vector3 middleDirection = onPalmPlane(FingerSegmentDirection(skeleton, middle, 0));
	// 向き from を向き to へ、rate の割合だけ回して、chain[j] から指先までを回す。
	auto turnChain = [&skeleton](const int* chain, int j, const Vector3& from, const Vector3& to, float rate) {
		Vector3 axis;
		axis.Cross(from, to);
		if (axis.LengthSq() <= FLT_EPSILON) {
			return;
		}
		const float deg = acosf(max(-1.0f, min(1.0f, from.Dot(to)))) * 57.2958f;
		RotateChain(skeleton, chain, j, axis, deg * rate);
	};
	for (int f = 0; f < THUMB; f++) {
		const int* chain = m_fingerBones[side][f];
		if (chain[0] < 0 || chain[1] < 0 || chain[2] < 0) {
			continue;
		}
		Vector3 base = onPalmPlane(FingerSegmentDirection(skeleton, chain, 0));
		base = base * (1.0f - RELEASE_FINGER_CLOSE_RATE) + middleDirection * RELEASE_FINGER_CLOSE_RATE;
		base.Normalize();
		if (f == LITTLE) {
			// 小指は、中指から離れる側（外側）へ、さらに RELEASE_LITTLE_SPREAD_DEG だけ開く。
			Vector3 turn;
			turn.Cross(middleDirection, base);
			Quaternion spread;
			spread.SetRotationDeg(palmNormal, turn.Dot(palmNormal) >= 0.0f ? RELEASE_LITTLE_SPREAD_DEG : -RELEASE_LITTLE_SPREAD_DEG);
			spread.Apply(base);
		}
		float curlDeg = 0.0f;
		for (int j = 0; j < 3; j++) {
			curlDeg += RELEASE_FINGER_CURL_DEGS[j] + (j == 0 ? RELEASE_FINGER_BASE_EXTRA_CURL_DEGS[f] : 0.0f);
			const float rad = curlDeg / 57.2958f;
			const Vector3 target = base * cosf(rad) + palmNormal * sinf(rad);
			turnChain(chain, j, FingerSegmentDirection(skeleton, chain, j), target, w);
		}
	}
	// 親指: 手のひらの面の上で、人差し指の向きから親指の側へ RELEASE_THUMB_SPREAD_DEG だけ開いた向きにする
	// （クリップのままだと、親指だけ手のひらの前へ出る）。付け根と第二関節を、順にその向きへ向ける。
	const int* thumb = m_fingerBones[side][THUMB];
	const int* index = m_fingerBones[side][0];
	if (thumb[0] >= 0 && thumb[1] >= 0 && thumb[2] >= 0 && index[0] >= 0 && index[1] >= 0) {
		const Vector3 indexDirection = onPalmPlane(FingerSegmentDirection(skeleton, index, 0));
		const Vector3 thumbOnPlane = onPalmPlane(FingerSegmentDirection(skeleton, thumb, 0));
		// 親指の側: 人差し指から親指へ回る向き（手のひらの向きを軸にした回転の正負）。
		Vector3 turn;
		turn.Cross(indexDirection, thumbOnPlane);
		const float sign = turn.Dot(palmNormal) >= 0.0f ? 1.0f : -1.0f;
		Quaternion spread;
		spread.SetRotationDeg(palmNormal, RELEASE_THUMB_SPREAD_DEG * sign);
		Vector3 target = indexDirection;
		spread.Apply(target);
		for (int j = 0; j < 2; j++) {
			turnChain(thumb, j, FingerSegmentDirection(skeleton, thumb, j), target, w);
		}
	}
}

void FirstPersonArms::CurlFingers(Skeleton& skeleton)
{
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
				RotateChain(skeleton, chain, j, axis, FINGER_CURL_DEGS[j] * w);
			}
		}
		PlaceThumb(skeleton, side, w);
	}
}

void FirstPersonArms::PlaceThumb(Skeleton& skeleton, int side, float grip)
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
			RotateChain(skeleton, thumb, 2, axis, THUMB_CURL_DEG * grip);
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
	const Matrix& cameraRotationMatrix = g_camera3D->GetCameraRotation();
	const Vector3 cameraUp(cameraRotationMatrix.m[1][0], cameraRotationMatrix.m[1][1], cameraRotationMatrix.m[1][2]);
	for (int side = 0; side < 2; side++) {
		if (m_upperArmBone[side] < 0) {
			continue;
		}
		// 構え → 氷を撃つとき → 炎を溜めるとき → 炎を放つとき、の順に、この腕の混ぜ具合で混ぜる。
		ArmPose pose = BlendPose(IDLE_POSE, CAST_POSE, m_castWeight[side]);
		pose = BlendPose(pose, CHARGE_POSE, m_chargeWeight[side]);
		pose = BlendPose(pose, RELEASE_POSE, m_releaseWeight[side]);
		const float spreadDeg = pose.spreadDeg;
		const float raiseDeg = pose.raiseDeg;
		const float sideShift = pose.sideShift;
		const float drop = pose.drop;
		const float push = pose.push;
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
	const SkillController& skills = player.GetSkillController();
	m_lastMainFireCount = skills.GetUseCount(MAIN_BUTTON);
	m_lastSubFireCount = skills.GetUseCount(SUB_BUTTON);
	m_releaseTimer = 0.0f;
	for (int side = 0; side < 2; side++) {
		m_castTimer[side] = 0.0f;
		m_castWeight[side] = 0.0f;
		m_chargeWeight[side] = 0.0f;
		m_releaseWeight[side] = 0.0f;
		m_grip[side] = 1.0f;
	}
	Play(enClip_Idle, 0.0f);
	UpdateTransform();
	m_model.Update();
}

void FirstPersonArms::Update(const Player& player)
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
	UpdateClip(player, deltaTime);
	// 構えと撃つときの置き方を、急に変わらないよう、少しずつ混ぜて切り替える。
	const float rate = min(1.0f, deltaTime * POSE_BLEND_SPEED);
	for (int side = 0; side < 2; side++) {
		// 突き出すのは、撃った瞬間に伸びきるよう、炎を放つときと同じ速さにする。
		m_castWeight[side] += (GetCastWeightTarget(side) - m_castWeight[side]) * min(1.0f, deltaTime * RELEASE_PUSH_SPEED);
		m_chargeWeight[side] += (GetChargeWeightTarget() - m_chargeWeight[side]) * rate;
		const float releaseTarget = m_releaseTimer > 0.0f ? 1.0f : 0.0f;
		m_releaseWeight[side] += (releaseTarget - m_releaseWeight[side]) * min(1.0f, deltaTime * RELEASE_PUSH_SPEED);
		// 手は、開くときは速く（開いた瞬間に魔法が出る）、握り直すときはゆっくり。
		const float gripTarget = GetGripTarget(side);
		const float gripSpeed = gripTarget < m_grip[side] ? GRIP_OPEN_SPEED : GRIP_CLOSE_SPEED;
		m_grip[side] += (gripTarget - m_grip[side]) * min(1.0f, deltaTime * gripSpeed);
	}
	UpdateTransform();
	m_model.Update();
}

float FirstPersonArms::GetCastWeightTarget(int side) const
{
	return m_castTimer[side] > 0.0f ? 1.0f : 0.0f;
}

float FirstPersonArms::GetChargeWeightTarget() const
{
	// 炎は両手（撃って手を開いている間も、溜めたときの置き方のまま）。
	return IsChargeClip() ? 1.0f : 0.0f;
}

bool FirstPersonArms::IsChargeClip() const
{
	return m_clip == enClip_ChargeStart || m_clip == enClip_Charge;
}

float FirstPersonArms::GetGripTarget(int side) const
{
	// 炎を溜めている間と撃った直後は、握らずに、クリップの手の形（火の玉を包む手）のままにする。
	// 溜めのクリップの指はもとから曲がっているので、握りこぶしの曲げを足すと、指があちこちへ曲がってしまう。
	if (IsChargeClip() || m_releaseTimer > 0.0f) {
		return 0.0f;
	}
	// 氷を撃っている手は開く。
	if (m_castTimer[side] > 0.0f) {
		return 0.0f;
	}
	return 1.0f;
}

void FirstPersonArms::Draw(RenderContext& rc)
{
	m_model.Draw(rc);
}

void FirstPersonArms::UpdateClip(const Player& player, float deltaTime)
{
	const SkillController& skills = player.GetSkillController();
	const bool isMainCast = IsFiredThisFrame(skills, MAIN_BUTTON, m_lastMainFireCount);
	const bool isSubCast = IsFiredThisFrame(skills, SUB_BUTTON, m_lastSubFireCount);
	const bool isCharging = skills.IsHolding(SUB_BUTTON);
	const bool isChargeClip = IsChargeClip();
	for (float& timer : m_castTimer) {
		timer = max(0.0f, timer - deltaTime);
	}

	// 両手の動き（炎）を、片手の動き（氷）より先に見る。溜めている間に氷を撃っても、両手は上げたまま。
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
		// 氷は、右手と左手で交互に撃つ。n 回目（0 から数える）が偶数なら画面の右の手（左の骨、side 0）、奇数なら左の手。
		// 撃つ位置も、PlayerInputSource が同じ決まりで右と左に分けている。
		const int shotNo = m_lastMainFireCount - 1;
		m_castTimer[shotNo % 2 == 0 ? 0 : 1] = CAST_OPEN_SECONDS;
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

	Vector3 position = cameraPosition;
	position += g_camera3D->GetRight() * ARMS_OFFSET.x;
	position += cameraUp * ARMS_OFFSET.y;
	position += g_camera3D->GetForward() * ARMS_OFFSET.z;

	Quaternion rotation;
	rotation.SetRotation(cameraRotationMatrix);
	m_model.SetTransform(position, rotation);
}

bool FirstPersonArms::IsFiredThisFrame(const SkillController& skills, ActionButton button, int& lastCount)
{
	const int count = skills.GetUseCount(button);
	const bool isFired = count > lastCount;
	lastCount = count;
	return isFired;
}
