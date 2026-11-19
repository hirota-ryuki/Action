#include "stdafx.h"
#include "FirstPersonArmsPoser.h"

#include "CharacterModel.h"
#include "PoseBaker.h"
#include "PoseLayers.h"

// 焼き込みは開発用なので、Debug ビルドだけに入れる。
#ifdef _DEBUG

namespace
{
	// 一人称の腕のモデル（FirstPersonArms と同じもの）。
	const char* const MODEL_PATH = "Assets/modelData/player/Model/SK_PlayerFp.tkm";
	constexpr float MODEL_SCALE = 1.128f;
	// 形を重ねる前のクリップ（tools/BlenderScripts/first_person_arms_anims.json で作ったもの）と、焼き込んだクリップの置き場所。
	const char* const SOURCE_CLIP_DIR = "Assets/modelData/player/Animation/FirstPerson/";
	const char* const BAKED_CLIP_DIR = "Assets/modelData/player/Animation/FirstPerson/Baked/";
	// 焼き込んだ姿勢の JSON の置き場所（ゲームの作業フォルダ Action/Game から見たパス）。
	const char* const BAKE_JSON_DIR = "../tools/FirstPerson/BakedPose/";
	// この骨とその下の骨が腕。
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
	// 握りこぶしの、関節ごとの曲がりの角度（度。付け根・第二関節・第一関節の順。まっすぐで 0）。
	// 人の握りこぶしは、付け根 85〜90°、第二関節 100° くらい、第一関節 60〜70° くらい。
	// 第一関節は浅め（45°）にする。このモデルは深く曲げると、人差し指の先が親指の上へはみ出して見えるため。
	const float FIST_JOINT_DEGS[3] = { 88.0f, 100.0f, 45.0f };
	// 親指: IK で合わせるときの決まり。親指の第一関節を人差し指の中節の上に、親指の先を中指の中節の上に置き、指を横切らせる。
	constexpr float THUMB_OUTSIDE_OFFSET = 1.2f;  // 目標を指の中節から外へ出す量（指と親指の太さの半分ずつくらい。骨格の単位）。
	// 目標を、指の中節のどこに置くか（0 で第二関節、1 で第一関節）。親指には太さがあるので、第二関節に置くと、
	// 見た目では親指が中節の上に重なり、人差し指の先も親指の下に隠れる（第一関節の側へ寄せるほど、親指が拳の下へずれる）。
	constexpr float THUMB_ON_PHALANX_RATE = 0.0f;
	constexpr float THUMB_IP_RADIAL_RATE = 0.3f;  // 親指の第一関節の目標を、人差し指の中節から、中指と反対の側（外）へずらす量（人差し指と中指の間の何倍か）。
	constexpr float THUMB_TIP_LENGTH_RATE = 0.9f; // 親指の先の骨の長さを、ひとつ手前の骨の長さの何倍とみなすか。
	constexpr int THUMB_IK_ITERATIONS = 6;

	enum EnSourceClip
	{
		enSource_Idle,         // FpIdle: 両手を胸の下で構える。
		enSource_ChargeStart,  // FpChargeStart: 両手を前へ上げる。
		enSource_Charge,       // FpCharge: 両手を上げたまま溜める。
		enSource_Num,
	};

	// 焼き込む1本分。もとのクリップに、weights の形を重ねる。
	struct BakeJob
	{
		const char* name;
		EnSourceClip source;
		bool isLoop;
		FirstPersonArmsPoser::Weights weights;
	};

	std::vector<BakeJob> MakeBakeJobs()
	{
		std::vector<BakeJob> jobs;
		FirstPersonArmsPoser::Weights w;
		// 構え: 両手を握りこぶしにして、画面の下の左右に置く。
		jobs.push_back({ "FpIdle", enSource_Idle, true, w });
		// 氷を撃つ（左の骨 = 画面の右の手、右の骨 = 画面の左の手）: 撃つ腕だけ、手を開いて前へ突き出す。
		// ゲームでは、撃つ腕の骨だけに重みを付けた層で流すので、もう片方の腕は構えのまま。
		for (int side = 0; side < 2; side++) {
			FirstPersonArmsPoser::Weights cast;
			cast.cast[side] = 1.0f;
			cast.grip[side] = 0.0f;
			jobs.push_back({ side == 0 ? "FpCastL" : "FpCastR", enSource_Idle, true, cast });
		}
		// 炎を溜める: 両手を開いて、火の玉の左右に添える（溜め始め・溜め）。
		FirstPersonArmsPoser::Weights charge;
		for (int side = 0; side < 2; side++) {
			charge.charge[side] = 1.0f;
			charge.grip[side] = 0.0f;
		}
		jobs.push_back({ "FpChargeStart", enSource_ChargeStart, false, charge });
		jobs.push_back({ "FpCharge", enSource_Charge, true, charge });
		// 炎を放つ: 溜めの姿勢から、両腕を前へ伸ばして手のひらを前へ向ける。
		FirstPersonArmsPoser::Weights release = charge;
		for (int side = 0; side < 2; side++) {
			release.release[side] = 1.0f;
		}
		jobs.push_back({ "FpRelease", enSource_Charge, true, release });
		return jobs;
	}

	// 一人称の腕のモデルを読み、カメラと同じ向きに置く（腕の置き方はカメラの向きで作るので、ゲームと同じ置き方にする）。
	void InitArmsModel(CharacterModel& model, const std::vector<AnimClipDesc>& clips)
	{
		CharacterModelDesc desc;
		desc.modelPath = MODEL_PATH;
		desc.upAxis = enModelUpAxisZ;
		desc.scale = MODEL_SCALE;
		desc.isShadowCaster = false;
		desc.clips = clips;
		model.Init(desc);
		Quaternion rotation;
		rotation.SetRotation(g_camera3D->GetCameraRotation());
		model.SetTransform(g_camera3D->GetPosition(), rotation);
		model.Update();
	}

	void WriteLog(FILE* fp, const char* format, const char* name, float value)
	{
		if (fp != nullptr) {
			fprintf(fp, format, name, value);
			fflush(fp);
		}
	}
}

void FirstPersonArmsPoser::SetWeights(const Weights& weights)
{
	for (int side = 0; side < 2; side++) {
		m_castWeight[side] = weights.cast[side];
		m_chargeWeight[side] = weights.charge[side];
		m_releaseWeight[side] = weights.release[side];
		m_grip[side] = weights.grip[side];
	}
}

void FirstPersonArmsPoser::Apply(Skeleton& skeleton)
{
	CurlFingers(skeleton);
	SpreadArms(skeleton);
	AimHands(skeleton);
}

bool FirstPersonArmsPoser::BakeAll(const char* logPath)
{
	PoseBaker::MakeDirectories(BAKE_JSON_DIR);
	FILE* log = nullptr;
	fopen_s(&log, logPath, "w");
	std::vector<AnimClipDesc> clips(enSource_Num);
	const std::string dir = SOURCE_CLIP_DIR;
	clips[enSource_Idle] = { dir + "FpIdle.fbx", true };
	clips[enSource_ChargeStart] = { dir + "FpChargeStart.fbx", false };
	clips[enSource_Charge] = { dir + "FpCharge.fbx", true };
	CharacterModel model;
	InitArmsModel(model, clips);

	FirstPersonArmsPoser poser;
	poser.Init(model.GetModelRender().GetSkeleton());
	PoseLayers layers;
	layers.Init(model.GetModelRender());
	layers.Add([&poser](Skeleton& skeleton) { poser.Apply(skeleton); });

	bool isAllOk = true;
	for (const BakeJob& job : MakeBakeJobs()) {
		poser.SetWeights(job.weights);
		const std::string path = std::string(BAKE_JSON_DIR) + job.name + ".json";
		const bool isOk = PoseBaker::Bake(model, layers, job.source, job.name, job.isLoop, path);
		WriteLog(log, isOk ? "baked %s (%.0f)\n" : "FAILED %s (%.0f)\n", job.name, model.GetClipLength(job.source) * PoseBaker::BAKE_FPS);
		isAllOk = isAllOk && isOk;
	}
	if (log != nullptr) {
		fclose(log);
	}
	return isAllOk;
}

bool FirstPersonArmsPoser::VerifyAll(const char* logPath)
{
	PoseBaker::MakeDirectories(BAKE_JSON_DIR);
	FILE* log = nullptr;
	fopen_s(&log, logPath, "w");
	const std::vector<BakeJob> jobs = MakeBakeJobs();
	std::vector<AnimClipDesc> clips;
	for (const BakeJob& job : jobs) {
		clips.push_back({ std::string(BAKED_CLIP_DIR) + job.name + ".fbx", job.isLoop });
	}
	CharacterModel model;
	InitArmsModel(model, clips);
	PoseLayers layers;
	layers.Init(model.GetModelRender());

	bool isAllOk = true;
	for (int i = 0; i < static_cast<int>(jobs.size()); i++) {
		const std::string path = std::string(BAKE_JSON_DIR) + jobs[i].name + ".json";
		const float error = PoseBaker::Compare(model, layers, i, path);
		WriteLog(log, "%s max position error %.4f\n", jobs[i].name, error);
		isAllOk = isAllOk && error >= 0.0f;
	}
	if (log != nullptr) {
		fclose(log);
	}
	return isAllOk;
}

void FirstPersonArmsPoser::Init(const Skeleton& skeleton)
{
	const int numBones = skeleton.GetNumBones();
	for (int side = 0; side < 2; side++) {
		m_armBones[side].clear();
		m_upperArmBone[side] = skeleton.FindBoneID(ARM_ROOT_BONES[side]);
	}
	for (int boneNo = 0; boneNo < numBones; boneNo++) {
		for (int no = boneNo; no >= 0; no = skeleton.GetBone(no)->GetParentBoneNo()) {
			const wchar_t* name = skeleton.GetBone(no)->GetName();
			const int side = wcscmp(name, ARM_ROOT_BONES[0]) == 0 ? 0 : (wcscmp(name, ARM_ROOT_BONES[1]) == 0 ? 1 : -1);
			if (side >= 0) {
				m_armBones[side].push_back(boneNo);
				break;
			}
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

Vector3 FirstPersonArmsPoser::AimHand(Skeleton& skeleton, int side, const Vector3& palmTarget, const Vector3& fingersTarget, float w)
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

void FirstPersonArmsPoser::AimHands(Skeleton& skeleton)
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

Vector3 FirstPersonArmsPoser::FingerSegmentDirection(Skeleton& skeleton, const int* chain, int j)
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

void FirstPersonArmsPoser::ShapeReleaseFingers(Skeleton& skeleton, int side, Vector3 palmNormal, float w)
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

void FirstPersonArmsPoser::CurlFingers(Skeleton& skeleton)
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
		// 人差し指〜小指: 付け根の関節から順に、関節の曲がりが FIST_JOINT_DEGS になるよう、その関節より先の骨をまとめて回す。
		// 付け根の関節の曲がりは、手首から指の付け根への向き（手の甲の骨の向き）と、指の根元の骨の向きの間の角度で測る。
		// アニメーションの指の形に足して曲げると、付け根の関節が浅いまま（50〜70°）で、指だけが曲がった形になるため、
		// 関節ごとの角度を決めて合わせる。
		// 曲げる軸は、その関節で今曲がっている面の軸（手前の骨の向きと先の骨の向きの外積）。
		// 骨の行列の軸をそのまま使うと、指の曲がる向きとずれて、曲げた分の多くがねじれになるため。
		// まっすぐで軸が決まらない関節は、ひとつ手前の関節と同じ面で曲げる。
		for (int f = 0; f < THUMB; f++) {
			const int* chain = m_fingerBones[side][f];
			if (chain[0] < 0 || chain[1] < 0 || chain[2] < 0) {
				continue;
			}
			Vector3 axis = Vector3::Zero;
			for (int j = 0; j < 3; j++) {
				Vector3 before = j == 0
					? BonePosition(skeleton, chain[0]) - BonePosition(skeleton, m_handBone[side])
					: FingerSegmentDirection(skeleton, chain, j - 1);
				const Vector3 after = FingerSegmentDirection(skeleton, chain, j);
				before.Normalize();
				Vector3 bendAxis;
				bendAxis.Cross(before, after);
				if (bendAxis.LengthSq() > FLT_EPSILON) {
					axis = bendAxis;
				}
				if (axis.LengthSq() <= FLT_EPSILON) {
					continue;
				}
				const float currentDeg = acosf(max(-1.0f, min(1.0f, before.Dot(after)))) * 57.2958f;
				RotateChain(skeleton, chain, j, axis, (FIST_JOINT_DEGS[j] - currentDeg) * w);
			}
		}
		PlaceThumb(skeleton, side, w);
	}
}

void FirstPersonArmsPoser::PlaceThumb(Skeleton& skeleton, int side, float grip)
{
	// 握りこぶしの親指は、手のひらの中に入れず、曲げた人差し指と中指の中節（第二関節と第一関節の間）の外側に、指を横切るように重ねる。
	// 付け根側の関節から順に回して親指の第一関節を人差し指の上へ合わせ、親指の第一関節で回して親指の先を中指の上へ向ける（CCD の IK）。
	// 親指の先だけを目標に合わせると、親指が指に沿って手首の側から伸び、指を横切らないため、関節と先の2か所に目標を置く。
	const int* thumb = m_fingerBones[side][THUMB];
	const int* index = m_fingerBones[side][0];
	const int* middle = m_fingerBones[side][1];
	if (thumb[0] < 0 || thumb[1] < 0 || thumb[2] < 0 || index[2] < 0 || middle[2] < 0) {
		return;
	}
	// 目標は、指の中節の上の点（THUMB_ON_PHALANX_RATE）を、中節に垂直に、手のひらの中心から離れる向きへ外へ出した位置。
	// 親指の第一関節は人差し指の中節の外寄り、親指の先は中指の中節に置く（親指が人差し指と中指を横切る）。
	const Vector3 palmCenter = (BonePosition(skeleton, m_handBone[side]) + BonePosition(skeleton, middle[0])) * 0.5f;
	// 指の中節の上の点を、中節に垂直に、手のひらの中心から離れる向きへ、指の太さの分だけ出した位置。
	auto onMiddlePhalanx = [&](const int* chain) {
		const Vector3 pip = BonePosition(skeleton, chain[1]);
		Vector3 phalanx = BonePosition(skeleton, chain[2]) - pip;
		const Vector3 position = pip + phalanx * THUMB_ON_PHALANX_RATE;
		phalanx.Normalize();
		Vector3 outward = position - palmCenter;
		outward -= phalanx * outward.Dot(phalanx);
		if (outward.LengthSq() <= FLT_EPSILON) {
			return position;
		}
		outward.Normalize();
		return position + outward * THUMB_OUTSIDE_OFFSET;
	};
	const Vector3 indexPoint = onMiddlePhalanx(index);
	const Vector3 target = onMiddlePhalanx(middle);
	const Vector3 ipTarget = indexPoint - (target - indexPoint) * THUMB_IP_RADIAL_RATE;

	// 親指の先: 先の骨の向きに、先の骨の長さのぶん伸ばした位置（先の骨の先端の骨が無いので、こうして決める）。
	// 先の骨の向きは、先の骨の行列から取る（第一関節で回すと変わるように）。
	const float tipLength = (BonePosition(skeleton, thumb[2]) - BonePosition(skeleton, thumb[1])).Length() * THUMB_TIP_LENGTH_RATE;
	auto thumbTip = [&]() {
		return BonePosition(skeleton, thumb[2]) + FingerSegmentDirection(skeleton, thumb, 2) * tipLength;
	};
	// pivot の関節（thumb[j]）を中心に回して、effector を goal へ、握り具合のぶんだけ近づける（手を開くときは、元の親指の形へ戻る）。
	auto turnToward = [&](int j, const Vector3& effector, const Vector3& goal) {
		const Vector3 pivot = BonePosition(skeleton, thumb[j]);
		Vector3 toEffector = effector - pivot;
		Vector3 toGoal = goal - pivot;
		if (toEffector.LengthSq() <= FLT_EPSILON || toGoal.LengthSq() <= FLT_EPSILON) {
			return;
		}
		toEffector.Normalize();
		toGoal.Normalize();
		Vector3 axis;
		axis.Cross(toEffector, toGoal);
		if (axis.LengthSq() <= FLT_EPSILON) {
			return;
		}
		const float deg = acosf(max(-1.0f, min(1.0f, toEffector.Dot(toGoal)))) * 57.2958f;
		RotateChain(skeleton, thumb, j, axis, deg * grip);
	};
	for (int iteration = 0; iteration < THUMB_IK_ITERATIONS; iteration++) {
		// 第二関節（MP）、付け根（CM）の順に回して、親指の第一関節（thumb[2]）を ipTarget へ合わせる。
		for (int j = 1; j >= 0; j--) {
			turnToward(j, BonePosition(skeleton, thumb[2]), ipTarget);
		}
		// 親指の第一関節で回して、親指の先を target へ向ける。
		turnToward(2, thumbTip(), target);
	}
}

Vector3 FirstPersonArmsPoser::BonePosition(Skeleton& skeleton, int boneNo)
{
	const Matrix& m = skeleton.GetBone(boneNo)->GetWorldMatrix();
	return Vector3(m.m[3][0], m.m[3][1], m.m[3][2]);
}

void FirstPersonArmsPoser::RotateChain(Skeleton& skeleton, const int* chain, int from, Vector3 axis, float deg)
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

void FirstPersonArmsPoser::SpreadArms(Skeleton& skeleton)
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

#endif
