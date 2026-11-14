#include "stdafx.h"
#include "FirstPersonArms.h"

#include "Player.h"

namespace
{
	// プレイヤーの体と同じモデル。腕（上腕から先）以外の骨を大きさ 0 につぶして、腕と手だけを映す。
	const char* const MODEL_PATH = "Assets/modelData/player/Model/SK_Player.FBX";
	const char* const CLIP_DIR = "Assets/modelData/player/Animation/FirstPerson/";
	constexpr float MODEL_SCALE = 1.128f;  // プレイヤーの体と同じ大きさ。
	// この骨とその下の骨だけを残す。
	const wchar_t* const ARM_ROOT_BONES[] = { L"J_Bip_L_UpperArm", L"J_Bip_R_UpperArm" };
	// 袖口を細くする前腕の骨と、その太さの割合。
	const wchar_t* const LOWER_ARM_BONES[] = { L"J_Bip_L_LowerArm", L"J_Bip_R_LowerArm" };
	constexpr float SLEEVE_SCALE = 0.4f;
	// 両腕を、肩を中心に外へ開く角度と、上げる角度（度）。手を画面の左右の端に置いて、自分の手で見ている感じを出す。
	constexpr float ARM_SPREAD_DEG = 25.0f;
	constexpr float ARM_RAISE_DEG = 20.0f;

	// カメラから見て、腕のモデルの足元を置く位置（x: 右、y: 上、z: 前）。
	// 腕のモデルの目の高さ（約 157）より下げて、両手を上げたときに手が画面の下の方に映るようにする。
	const Vector3 ARMS_OFFSET = Vector3(0.0f, -182.0f, -8.0f);

	// アニメーションを切り替えるときの補間（秒）。
	constexpr float TO_IDLE_SECONDS = 0.15f;
	constexpr float TO_CAST_SECONDS = 0.05f;
	constexpr float TO_CHARGE_START_SECONDS = 0.08f;
	constexpr float TO_CHARGE_SECONDS = 0.1f;
	constexpr float TO_RELEASE_SECONDS = 0.05f;
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
	desc.clips[enClip_Release] = { dir + "FpRelease.fbx", false };
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
	m_lowerArmBones.clear();
	for (const wchar_t* name : LOWER_ARM_BONES) {
		const int boneNo = skeleton.FindBoneID(name);
		if (boneNo >= 0) {
			m_lowerArmBones.push_back(boneNo);
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
		SpreadArms(sk);
		// 袖口を細くする。広がった袖口が手より大きく、後ろ（目の位置）から見ると手が隠れるため。
		// 前腕の骨の、骨の向き（X）以外の2つの軸を縮める。袖口は前腕の骨で動いているので細くなり、手の骨はそのまま。
		for (int boneNo : m_lowerArmBones) {
			Bone* bone = sk.GetBone(boneNo);
			Matrix narrowed = bone->GetWorldMatrix();
			for (int r = 1; r < 3; r++) {
				narrowed.m[r][0] *= SLEEVE_SCALE;
				narrowed.m[r][1] *= SLEEVE_SCALE;
				narrowed.m[r][2] *= SLEEVE_SCALE;
			}
			bone->SetWorldMatrix(narrowed);
		}
	});
}

void FirstPersonArms::SpreadArms(Skeleton& skeleton)
{
	float spreadDeg = ARM_SPREAD_DEG, raiseDeg = ARM_RAISE_DEG; // FP_TEST_TEMP
	if (const char* e = getenv("FP_TEST_SPREAD")) { sscanf_s(e, "%f,%f", &spreadDeg, &raiseDeg); } // FP_TEST_TEMP
	const Matrix& cameraRotationMatrix = g_camera3D->GetCameraRotation();
	const Vector3 cameraUp(cameraRotationMatrix.m[1][0], cameraRotationMatrix.m[1][1], cameraRotationMatrix.m[1][2]);
	for (int side = 0; side < 2; side++) {
		if (m_upperArmBone[side] < 0) {
			continue;
		}
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
		Matrix toOrigin, toPivot, aroundPivot;
		toOrigin.MakeTranslation(pivot * -1.0f);
		toPivot.MakeTranslation(pivot);
		aroundPivot.Multiply(toOrigin, rotation);
		aroundPivot.Multiply(aroundPivot, toPivot);
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
	Play(enClip_Idle, 0.0f);
	UpdateTransform();
	m_model.Update();
}

void FirstPersonArms::Update(const Player& player)
{
	if (getenv("FP_TEST_NOANIM")) { UpdateTransform(); m_model.Update(); return; } // FP_TEST_TEMP
	if (const char* c = getenv("FP_TEST_CLIP")) { const char* t = getenv("FP_TEST_TIME"); m_model.PlayAnimationAtTime(atoi(c), t ? (float)atof(t) : 0.0f); UpdateTransform(); m_model.Update(); return; } // FP_TEST_TEMP
	UpdateClip(player);
	UpdateTransform();
	m_model.Update();
}

void FirstPersonArms::Draw(RenderContext& rc)
{
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

void FirstPersonArms::UpdateClip(const Player& player)
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
		Play(enClip_Release, TO_RELEASE_SECONDS);
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
		// 突き出す・下ろすが終わったら、構えに戻る。
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
