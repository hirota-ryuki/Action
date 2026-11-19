#include "stdafx.h"
#include "FirstPersonArms.h"

#include "FirstPersonBob.h"
#include "Player.h"

namespace
{
	// プレイヤーの体から、袖の肘から先を消したモデル（tools/FirstPerson/make_first_person_arms_tkm.py で作る）。
	// 骨は SK_Player と同じ。腕（上腕から先）以外の骨を大きさ 0 につぶして、腕と手だけを映す。
	const char* const MODEL_PATH = "Assets/modelData/player/Model/SK_PlayerFp.tkm";
	// 腕と手の形を焼き込んだクリップ（FirstPersonArmsPoser で作る）。
	const char* const CLIP_DIR = "Assets/modelData/player/Animation/FirstPerson/Baked/";
	constexpr float MODEL_SCALE = 1.128f;  // プレイヤーの体と同じ大きさ。
	// 腕の色に掛ける色。ボス戦のステージの月の光（Game.cpp の BOSS_MOON_COLOR）が強い青で、目の前に大きく映る腕が
	// 青白く見えるので、腕だけ青と緑を減らして打ち消す。月の光が当たる前腕は青く、当たらない手の甲は
	// 夜空からの明かりで青緑になるので、その間をとって、どちらも肌の色合いに近づくようにした。
	// 青と緑を減らすだけ（明るくはしない）なので、少し暗くなる。
	const Vector4 ARMS_MUL_COLOR = Vector4(1.0f, 0.62f, 0.48f, 1.0f);
	// この骨とその下の骨だけを残す。0 が左の骨（画面の右に映る）。
	const wchar_t* const ARM_ROOT_BONES[] = { L"J_Bip_L_UpperArm", L"J_Bip_R_UpperArm" };
	// 歩き・走りで腕を振り切ったときに、構えに足す量（FirstPersonBob::GetArmSwing が 1 のとき前、-1 のとき後ろ）。
	// 前は腕を上げて前へ出し、後ろは少しだけ下げて手前へ引く。後ろを小さくして、手が画面の下へ外れないようにする。
	struct ArmSwing
	{
		float raiseDeg;  // 肩を中心に上げる角度（度）。
		float push;      // 腕ごと前へ押し出す量。
	};
	const ArmSwing SWING_FORWARD = { 3.3f, 2.7f };
	const ArmSwing SWING_BACK = { -1.3f, -1.3f };
	// 氷を撃ったあと、手を開いて突き出したまま見せる時間（秒）。そのあと構えに戻る。
	constexpr float CAST_OPEN_SECONDS = 0.2f;
	// 炎を撃ったあと、両手を開いたまま見せる時間（秒）。そのあと最初の構えに戻る。
	constexpr float RELEASE_OPEN_SECONDS = 0.3f;
	// 氷を撃つ形の層の重みを変える速さ。撃った瞬間に伸びきるよう、速くする。
	constexpr float CAST_PUSH_SPEED = 30.0f;
	// 撃つ形にしている度合い（揺れを弱める）を変える速さ。
	constexpr float HOLD_BLEND_SPEED = 10.0f;

	// カメラから見て、腕のモデルの足元を置く位置（x: 右、y: 上、z: 前）。
	// 腕のモデルの目の高さ（約 157）より下げ、手前に寄せて、手が大きく画面の下の方に映るようにする。
	const Vector3 ARMS_OFFSET = Vector3(0.0f, -183.0f, -18.0f);

	// アニメーションを切り替えるときの補間（秒）。
	constexpr float TO_IDLE_SECONDS = 0.2f;
	constexpr float TO_CHARGE_START_SECONDS = 0.2f;
	constexpr float TO_CHARGE_SECONDS = 0.1f;
	constexpr float TO_RELEASE_SECONDS = 0.05f;  // 撃った瞬間に両腕が伸びきるよう、短くする。
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
	desc.clips[enClip_CastL] = { dir + "FpCastL.fbx", true };
	desc.clips[enClip_CastR] = { dir + "FpCastR.fbx", true };
	desc.clips[enClip_ChargeStart] = { dir + "FpChargeStart.fbx", false };
	desc.clips[enClip_Charge] = { dir + "FpCharge.fbx", true };
	desc.clips[enClip_Release] = { dir + "FpRelease.fbx", true };
	m_model.Init(desc);
	ModelRender& modelRender = m_model.GetModelRender();
	modelRender.SetMulColor(ARMS_MUL_COLOR);

	// 氷を撃つ腕は、その腕の骨だけに重みを付けた層（0 が左の骨、1 が右の骨）で、構えの上に重ねる。
	for (int side = 0; side < 2; side++) {
		modelRender.InitAnimationLayer({ { ARM_ROOT_BONES[side], 1.0f } }, side);
		modelRender.PlayLayerAnimationFromTime(side == 0 ? enClip_CastL : enClip_CastR, 0.0f, 0.0f, side);
		modelRender.SetLayerWeight(0.0f, side);
	}
	m_clip = enClip_Idle;
	InitBones();

	// 骨の調整の層。腕以外を見せない → 歩き・走りの腕の揺れ。どちらもゲームの中の動きで変わるので、焼き込まない。
	m_poseLayers.Init(modelRender);
	m_poseLayers.Add([this](Skeleton& skeleton) { HideBones(skeleton); }, false);
	m_poseLayers.Add([this](Skeleton& skeleton) { SwingArms(skeleton); }, false);
}

void FirstPersonArms::InitBones()
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
}

void FirstPersonArms::HideBones(Skeleton& skeleton)
{
	// 体・頭・髪の頂点は骨の位置の1点に集まって見えなくなる。肩の近くで腕と体の両方に重みがある頂点は、
	// 体の骨の側へ引っ張られるが、一人称では画面の外になる。
	for (int boneNo : m_hiddenBones) {
		Bone* bone = skeleton.GetBone(boneNo);
		Matrix collapsed = bone->GetWorldMatrix();
		for (int r = 0; r < 3; r++) {
			collapsed.m[r][0] = 0.0f;
			collapsed.m[r][1] = 0.0f;
			collapsed.m[r][2] = 0.0f;
		}
		bone->SetWorldMatrix(collapsed);
	}
}

void FirstPersonArms::SwingArms(Skeleton& skeleton)
{
	const Matrix& cameraRotationMatrix = g_camera3D->GetCameraRotation();
	const Vector3 cameraUp(cameraRotationMatrix.m[1][0], cameraRotationMatrix.m[1][1], cameraRotationMatrix.m[1][2]);
	for (int side = 0; side < 2; side++) {
		if (m_upperArmBone[side] < 0) {
			continue;
		}
		// 前に振るときと後ろに振るときで、足す量を変える。
		const float swing = m_bobSwing[side];
		const ArmSwing& swingPose = swing >= 0.0f ? SWING_FORWARD : SWING_BACK;
		const float raiseDeg = swingPose.raiseDeg * fabsf(swing);
		const float push = swingPose.push * fabsf(swing);
		const Vector3& bob = m_bobOffset[side];
		// 肩（上腕の付け根）を中心に、カメラの右の軸まわりに - へ回して持ち上げ、腕ごと揺れの分だけ動かす。
		Quaternion raise;
		raise.SetRotationDeg(g_camera3D->GetRight(), -raiseDeg);
		Matrix rotation, toOrigin, toPivot, toSide, aroundPivot;
		rotation.MakeRotationFromQuaternion(raise);
		const Vector3 pivot = PoseLayers::BonePosition(skeleton, m_upperArmBone[side]);
		toOrigin.MakeTranslation(pivot * -1.0f);
		toPivot.MakeTranslation(pivot);
		toSide.MakeTranslation(g_camera3D->GetRight() * bob.x + cameraUp * bob.y + g_camera3D->GetForward() * (push + bob.z));
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
		m_holdWeight[side] = 0.0f;
		m_bobOffset[side] = Vector3::Zero;
		m_bobSwing[side] = 0.0f;
		m_model.GetModelRender().SetLayerWeight(0.0f, side);
	}
	Play(enClip_Idle, 0.0f);
	UpdateTransform();
	m_model.Update();
}

void FirstPersonArms::Update(const Player& player, const FirstPersonBob& bob)
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
	UpdateClip(player, deltaTime);
	const bool isBothHands = IsChargeClip() || m_clip == enClip_Release;
	for (int side = 0; side < 2; side++) {
		// 氷を撃つ腕の層の重み。撃った瞬間に伸びきるよう、速く変える。
		const float castTarget = m_castTimer[side] > 0.0f ? 1.0f : 0.0f;
		m_castWeight[side] += (castTarget - m_castWeight[side]) * min(1.0f, deltaTime * CAST_PUSH_SPEED);
		m_model.GetModelRender().SetLayerWeight(m_castWeight[side], side);
		// 氷を突き出している腕と、炎を溜めている・放つ両腕は、揺らさない（手を撃つ位置・火の玉からずらさない）。
		const float holdTarget = isBothHands ? 1.0f : m_castWeight[side];
		m_holdWeight[side] += (holdTarget - m_holdWeight[side]) * min(1.0f, deltaTime * HOLD_BLEND_SPEED);
		const float hold = max(m_holdWeight[side], m_castWeight[side]);
		m_bobOffset[side] = bob.GetArmsOffset(side) * (1.0f - hold);
		m_bobSwing[side] = bob.GetArmSwing(side) * (1.0f - hold);
	}
	UpdateTransform();
	m_model.Update();
}

bool FirstPersonArms::IsChargeClip() const
{
	return m_clip == enClip_ChargeStart || m_clip == enClip_Charge;
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
		// 炎を撃った: 両腕を前へ伸ばして手を開き、少し見せてから最初の構えに戻る。
		m_releaseTimer = RELEASE_OPEN_SECONDS;
		Play(enClip_Release, TO_RELEASE_SECONDS);
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
