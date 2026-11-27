#include "stdafx.h"
#include "Player.h"
#include "PlayerConfig.h"
#include "IceMagic.h"
#include "FireMagic.h"
#include "MagicConfig.h"
#include "PoseLayers.h"
#include "PlayerDashState.h"
#include <imgui/imgui.h>

namespace
{
	// ジャンプ中の度合いが 0 から 1（1 から 0）まで変わるのにかかる時間（秒）。アニメーションの切り替えの補間と同じくらいにする。
	constexpr float JUMP_WEIGHT_FADE_SECONDS = 0.2f;
	// 溜め撃ちを構えて動くとき、進む向きとカメラの向きの内積がこれより大きければ、脚を進む向きへ向けて前歩きにする（上半身はひねる）。
	// 小さければ（後ろの方へ動く）、体はカメラの向きのまま後ろ歩きにする。-0.2 は約 100度。
	constexpr float AIM_TURN_LEGS_MIN_DOT = -0.2f;
	// 比べる用: ダッシュのアニメーションの1秒のコマ数と、最後のコマ（Dash は 17コマ = 0〜16コマ目）。
	constexpr float DEBUG_DASH_FPS = 30.0f;
	constexpr float DEBUG_DASH_LAST_FRAME = 16.0f;
	// 上半身をひねる角度の上限（度）と、背骨・胸・上の胸に分ける割合。
	constexpr float AIM_TWIST_MAX_DEG = 100.0f;
	constexpr float AIM_TWIST_SHARES[3] = { 0.3f, 0.3f, 0.4f };
	// 攻撃中の度合いが 0 から 1（1 から 0）まで変わるのにかかる時間（秒）。攻撃に入るときの補間（attack.blendSeconds）と同じにする。
	// これより長いと、入ってすぐの数コマで、前の動き（前の段の終わりなど）で横を向いていた顔が見えてしまう。
	constexpr float ATTACK_WEIGHT_FADE_SECONDS = 0.1f;
}

Player::Player()
	: Character(Faction::Player)
{
}

CharacterConfig Player::MakeConfig() const
{
	return PlayerConfig::MakeCharacterConfig();
}

void Player::OnStart()
{
	m_fallRespawn.Init(GetPosition(), PlayerConfig::RESPAWN_HEIGHT);

	PlayerStateContext stateContext;
	stateContext.mover = &m_mover;
	stateContext.animator = &m_animator;
	stateContext.statusEffects = &m_statusEffects;
	stateContext.settings = PlayerConfig::MakeStateSettings();
	m_animator.Init(&m_model.GetModelRender(), stateContext.settings.animBlendSeconds);
	// 上半身の層。背骨の根元は半分だけ効かせ、胸から上（首・頭・腕も）は全部、上半身の動きにする。
	ModelRender& modelRender = m_model.GetModelRender();
	modelRender.InitAnimationLayer({ { L"J_Bip_C_Spine", 0.5f }, { L"J_Bip_C_Chest", 1.0f } });
	m_upperBody.Init(&modelRender, PlayerConfig::MakeChargeSettings());
	m_handBones[0] = modelRender.FindBoneID(L"J_Bip_L_Hand");
	m_handBones[1] = modelRender.FindBoneID(L"J_Bip_R_Hand");
	m_twistBones[0] = modelRender.FindBoneID(L"J_Bip_C_Spine");
	m_twistBones[1] = modelRender.FindBoneID(L"J_Bip_C_Chest");
	m_twistBones[2] = modelRender.FindBoneID(L"J_Bip_C_UpperChest");
	m_rightUpperArmBone = modelRender.FindBoneID(L"J_Bip_R_UpperArm");
	m_neckBone = modelRender.FindBoneID(L"J_Bip_C_Neck");
	m_stateMachine.Init(stateContext);  // Idle から始める。

	m_skillController.Register<IceMagic>(ActionButton::Action1, MagicConfig::GetIce().cooldown);
	m_skillController.Register<FireMagic>(ActionButton::Action2, MagicConfig::GetFire().cooldown);
}

void Player::FillInputContext(InputSourceContext& context) const
{
	context.isFirstPerson = !IsRenderBody();
	context.action1UseCount = m_skillController.GetUseCount(ActionButton::Action1);
	context.isThirdPersonAim = IsThirdPersonAiming();
	context.chargePosition = m_chargePosition;
}

void Player::UpdateAction(float deltaTime)
{
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	UpdateDebugMeasure(deltaTime);
	CharacterInput input = m_input;
	FilterAimingInput(input);
	if (m_isDebugDashRequested) {
		// F1 の窓のボタンで、ダッシュを1回出す（窓を開いている間はキーの入力が来ないため）。
		m_isDebugDashRequested = false;
		input.dash.isTrigger = true;
	}
	m_stateMachine.SetAiming(IsThirdPersonAiming());
	m_stateMachine.Update(input, deltaTime);
	if (m_debugPoseClip >= 0) {
		// 比べる用: その場で（進まずに）ダッシュのアニメーションだけを流す。最後のコマまで行ったら最初へ戻す。
		if (!m_isDebugPoseStopped) {
			m_debugPoseFrame += deltaTime * DEBUG_DASH_FPS;
			if (m_debugPoseFrame > DEBUG_DASH_LAST_FRAME) {
				m_debugPoseFrame = 0.0f;
			}
		}
		m_model.GetModelRender().PlayAnimationAtTime(m_debugPoseClip, m_debugPoseFrame / DEBUG_DASH_FPS);
	}
	UpdateJumpWeight(deltaTime);
	UpdateAttackWeight(deltaTime);
	if (m_fallRespawn.IsFallen(m_mover.GetPosition())) {
		Respawn();
	}
}

void Player::FilterSkillInput(CharacterInput& input) const
{
	// 三人称では、溜め撃ちは地上でだけ始められる（押しっぱなしで着地しても始めない。押し直す）。
	if (m_isThirdPersonCamera && !m_mover.IsOnGround()) {
		input.Action(ActionButton::Action2).isTrigger = false;
	}
	// 溜め撃ちを構えている間（撃ったあとの下ろす動きも）は、氷魔法を撃たない（3段攻撃にも入らない）。
	if (IsThirdPersonAiming()) {
		input.Action(ActionButton::Action1) = ButtonState();
		return;
	}
	// 攻撃の間は、クリックした瞬間には撃たず、手を突き出したコマ（Attack が立てる）でだけ Action1 を押したことにする。
	// ButtonState::Update は通さない（押した瞬間の判定に使う m_input の前フレームの値を変えないため）。
	if (m_stateMachine.GetCurrentId() != PlayerStateId::Attack) {
		return;
	}
	const bool isFire = m_stateMachine.IsAttackFire();
	ButtonState& button = input.Action(ActionButton::Action1);
	button.isTrigger = isFire;
	button.isPress = isFire;
	button.isRelease = false;
}

void Player::UpdateJumpWeight(float deltaTime)
{
	const PlayerStateId id = m_stateMachine.GetCurrentId();
	const float target = (id == PlayerStateId::Air || id == PlayerStateId::Land) ? 1.0f : 0.0f;
	const float step = deltaTime / JUMP_WEIGHT_FADE_SECONDS;
	m_jumpWeight = target > m_jumpWeight ? min(target, m_jumpWeight + step) : max(target, m_jumpWeight - step);
	SetJumpWeight(m_jumpWeight);
}

void Player::UpdateAttackWeight(float deltaTime)
{
	const float target = m_stateMachine.GetCurrentId() == PlayerStateId::Attack ? 1.0f : 0.0f;
	const float step = deltaTime / ATTACK_WEIGHT_FADE_SECONDS;
	m_attackWeight = target > m_attackWeight ? min(target, m_attackWeight + step) : max(target, m_attackWeight - step);
	// 溜め撃ちで上半身を構えている間も、攻撃中と同じに扱う（走りの腕の振りを外し、顔を前へそろえる）。
	SetAttackWeight(max(m_attackWeight, m_upperBody.GetWeight()));
}

void Player::FilterAimingInput(CharacterInput& input)
{
	if (!IsThirdPersonAiming()) {
		return;
	}
	if (input.dash.isTrigger) {
		// ダッシュしたら、溜めをやめる（撃たない）。
		m_skillController.CancelAll();
		m_upperBody.Stop();
		return;
	}
	input.isRun = false;
	input.jump = ButtonState();
	input.Action(ActionButton::Action1) = ButtonState();
	input.moveDirection *= m_upperBody.GetSettings().moveSpeedRate;
	// 前・横へ動くときは、脚を進む向きへ向けて前歩きにする（上半身は TwistUpperBodyToAim でカメラの向きへひねる）。
	// 後ろの方へ動くときは、体はカメラの向き（入力元が入れた lookDirection）のまま、後ろ歩きにする。
	Vector3 move = input.moveDirection;
	move.y = 0.0f;
	if (move.LengthSq() > FLT_EPSILON) {
		move.Normalize();
		if (move.Dot(input.lookDirection) > AIM_TURN_LEGS_MIN_DOT) {
			input.lookDirection = move;
		}
		else {
			// 後ろ歩きは、前・横よりさらにゆっくりにする。
			input.moveDirection *= m_upperBody.GetSettings().backMoveSpeedRate;
		}
	}
}

void Player::OnAddPoseLayers(PoseLayers& layers)
{
	// 照準の向きはカメラで変わるので、焼き込まない。
	layers.Add([this](Skeleton& skeleton) { TwistUpperBodyToAim(skeleton); }, false);
}

void Player::TwistUpperBodyToAim(Skeleton& skeleton)
{
	const float weight = m_upperBody.GetWeight();
	if (!m_isThirdPersonCamera || weight <= 0.0f || m_twistBones[0] < 0 || m_twistBones[1] < 0 || m_twistBones[2] < 0) {
		return;
	}
	// 体（見た目）の前と、カメラの前（どちらも水平）。
	Vector3 bodyForward = Vector3::AxisZ;
	m_mover.GetVisualRotation().Apply(bodyForward);
	bodyForward.y = 0.0f;
	Vector3 aimForward = g_camera3D->GetForward();
	aimForward.y = 0.0f;
	if (bodyForward.LengthSq() <= FLT_EPSILON || aimForward.LengthSq() <= FLT_EPSILON) {
		return;
	}
	bodyForward.Normalize();
	aimForward.Normalize();
	// 体の前からカメラの前までの、縦の軸まわりの角度。
	Vector3 cross;
	cross.Cross(bodyForward, aimForward);
	float angle = atan2f(cross.y, bodyForward.Dot(aimForward));
	const float maxAngle = Math::DegToRad(AIM_TWIST_MAX_DEG);
	angle = max(-maxAngle, min(maxAngle, angle));
	// 回す向きを、行列で回して確かめる（右手系・左手系の取り違えを防ぐ）。回してカメラの前から遠ざかるなら、逆に回す。
	Matrix check;
	check.MakeRotationAxis(Vector3::AxisY, angle);
	Vector3 rotated = bodyForward;
	check.Apply(rotated);
	if (rotated.Dot(aimForward) < bodyForward.Dot(aimForward)) {
		angle = -angle;
	}
	// 構える・戻すのに合わせて、少しずつひねる（smoothstep）。
	const float smoothWeight = weight * weight * (3.0f - 2.0f * weight);
	angle *= smoothWeight;
	// 背骨・胸・上の胸に分けて、その骨から先をまとめて回す。
	for (int i = 0; i < 3; i++) {
		RotateBoneTreeY(skeleton, m_twistBones[i], angle * AIM_TWIST_SHARES[i]);
	}

	// 右肩を後ろへ引く（上の胸をさらにひねる）。首は引いた分だけ戻して、狙う向きのままにする。
	// 右腕は引いた分を戻したうえで、さらに外（画面の右）へ開く（真っすぐ奥へ向けると、体に隠れて見えないため）。
	// 回す向きを確かめる。体の右（狙う向きの右）が、回したあと後ろ（狙う向きの反対）へ行く向きを正にする。
	Vector3 aimRight;
	aimRight.Cross(Vector3::AxisY, aimForward);
	Matrix shoulderCheck;
	shoulderCheck.MakeRotationAxis(Vector3::AxisY, 1.0f);
	Vector3 rotatedRight = aimRight;
	shoulderCheck.Apply(rotatedRight);
	const float sign = rotatedRight.Dot(aimForward) < 0.0f ? 1.0f : -1.0f;
	const PlayerChargeSettings& settings = m_upperBody.GetSettings();
	const float shoulderAngle = sign * Math::DegToRad(settings.shoulderBackDeg) * smoothWeight;
	const float armOutAngle = sign * Math::DegToRad(settings.armOutDeg) * smoothWeight;
	RotateBoneTreeY(skeleton, m_twistBones[2], shoulderAngle);
	RotateBoneTreeY(skeleton, m_rightUpperArmBone, armOutAngle - shoulderAngle);
	RotateBoneTreeY(skeleton, m_neckBone, -shoulderAngle);
}

void Player::RotateBoneTreeY(Skeleton& skeleton, int boneId, float angle)
{
	if (boneId < 0) {
		return;
	}
	Bone* bone = skeleton.GetBone(boneId);
	const Matrix& world = bone->GetWorldMatrix();
	const Vector3 center(world.m[3][0], world.m[3][1], world.m[3][2]);
	Matrix toOrigin, rotation, back;
	toOrigin.MakeTranslation(center * -1.0f);
	rotation.MakeRotationAxis(Vector3::AxisY, angle);
	back.MakeTranslation(center);
	PoseLayers::RotateBoneTree(*bone, toOrigin * rotation * back);
}

void Player::OnLateUpdate(float deltaTime)
{
	m_upperBody.Update(
		m_skillController.IsHolding(ActionButton::Action2),
		m_skillController.GetUseCount(ActionButton::Action2),
		m_isThirdPersonCamera,
		deltaTime);
	UpdateChargePosition();
	if (DebugUI::IsVisible()) {
		DrawChargeDebugUI();
	}
}

void Player::DrawChargeDebugUI()
{
	PlayerChargeSettings& settings = m_upperBody.GetSettingsForEdit();
	ImGui::Begin(u8"溜め撃ちの構え");
	float holdFrame = settings.holdTime * 30.0f;
	if (ImGui::SliderFloat(u8"止めるコマ（Attack1）", &holdFrame, 0.0f, 16.0f, "%.0f")) {
		settings.holdTime = holdFrame / 30.0f;
	}
	ImGui::SliderFloat(u8"火の玉の前への距離（cm）", &settings.ballForward, 0.0f, 80.0f, "%.0f");
	ImGui::SliderFloat(u8"右肩を引く角度（度）", &settings.shoulderBackDeg, 0.0f, 60.0f, "%.0f");
	ImGui::SliderFloat(u8"右腕を外へ開く角度（度）", &settings.armOutDeg, -30.0f, 60.0f, "%.0f");
	ImGui::SliderFloat(u8"構えて歩く速さ（倍）", &settings.moveSpeedRate, 0.2f, 1.0f, "%.2f");
	ImGui::SliderFloat(u8"後ろ歩きにさらに掛ける（倍）", &settings.backMoveSpeedRate, 0.2f, 1.0f, "%.2f");
	if (ImGui::Button(u8"最初の値に戻す")) {
		settings = PlayerConfig::MakeChargeSettings();
	}
	ImGui::End();

	// 比べる用: 写真のポーズにする前（main）と今のダッシュを、ボタンで出し分ける。
	ImGui::Begin(u8"ダッシュの比較");
	if (ImGui::SliderFloat(u8"再生速度（倍）", &m_debugPlaySpeed, 0.05f, 1.0f, "%.2f")) {
		// ゲーム全体の1フレームの時間を変える（動きもアニメーションも一緒に遅くなる）。
		if (m_debugPlaySpeed < 1.0f) {
			g_gameTime->EnableFixedFrameDeltaTime(m_debugPlaySpeed / 60.0f);
		}
		else {
			g_gameTime->DisableFixedFrameDeltaTime();
		}
	}

	ImGui::Text(u8"ダッシュ（進む）");
	const auto stopPose = [this]() {
		if (m_debugPoseClip >= 0) {
			m_debugPoseClip = -1;
			m_animator.Play(enPlayerAnimClip_Idle);
		}
	};
	if (ImGui::Button(u8"前のダッシュ")) {
		stopPose();
		PlayerDashState::SetUseOldClip(true);
		m_isDebugDashRequested = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"今のダッシュ")) {
		stopPose();
		PlayerDashState::SetUseOldClip(false);
		m_isDebugDashRequested = true;
	}

	ImGui::Text(u8"ポーズだけ（その場で、進まない）");
	if (ImGui::Button(u8"前のポーズ")) {
		m_debugPoseClip = enPlayerAnimClip_DashOld;
		m_debugPoseFrame = 0.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"今のポーズ")) {
		m_debugPoseClip = enPlayerAnimClip_Dash;
		m_debugPoseFrame = 0.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"ポーズをやめる")) {
		stopPose();
	}
	ImGui::Checkbox(u8"止める", &m_isDebugPoseStopped);
	ImGui::SliderFloat(u8"コマ", &m_debugPoseFrame, 0.0f, DEBUG_DASH_LAST_FRAME, "%.1f");
	ImGui::End();
}

void Player::UpdateChargePosition()
{
	Vector3 forward = m_mover.GetForward();
	forward.y = 0.0f;
	forward.Normalize();
	// 構える前は、ふだん撃つ位置（胸の前）。
	Vector3 chestFront = m_mover.GetPosition();
	chestFront.y += GetCastSettings().castHeight;
	chestFront += forward * GetCastSettings().castForward;
	if (m_handBones[0] < 0 || m_handBones[1] < 0) {
		m_chargePosition = chestFront;
		return;
	}
	// 構えたら、右手の少し前（狙う向き）。上半身の層の重みで、胸の前から移す。
	ModelRender& modelRender = m_model.GetModelRender();
	const Matrix& right = modelRender.GetBone(m_handBones[1])->GetWorldMatrix();
	Vector3 handFront = *(const Vector3*)right.m[3];
	Vector3 aimForward = m_isThirdPersonCamera ? g_camera3D->GetForward() : forward;
	aimForward.Normalize();
	handFront += aimForward * m_upperBody.GetSettings().ballForward;
	m_chargePosition.Lerp(m_upperBody.GetWeight(), chestFront, handFront);
}

void Player::Respawn()
{
	m_mover.SetPosition(m_fallRespawn.GetRespawnPosition());
	m_mover.Stop();
	m_stateMachine.Reset();
}

// ───────── 【仮・測る用】ここから（コミットしない） ─────────
namespace
{
	float SideAngleDeg(const Vector3& v, const Vector3& forward)
	{
		// 横から見た角度。真下を 0°、後ろを正、前を負（BodyLink::LimitArmSwing と同じ決まり）。
		return Math::RadToDeg(atan2f(-v.Dot(forward), -v.y));
	}
	float AngleBetweenDeg(Vector3 a, Vector3 b)
	{
		a.Normalize();
		b.Normalize();
		return Math::RadToDeg(acosf(min(1.0f, max(-1.0f, a.Dot(b)))));
	}
}

void Player::UpdateDebugMeasure(float deltaTime)
{
	if (m_measureTimer == 0.0f && m_measureFrame < 0 && !m_isMeasureMode) {
		char buf[8] = {};
		size_t len = 0;
		if (getenv_s(&len, buf, sizeof(buf), "DASH_MEASURE") != 0 || len == 0) {
			m_measureTimer = -1.0f;  // 測らない。
			return;
		}
		m_isMeasureMode = true;
		g_gameTime->EnableFixedFrameDeltaTime(1.0f / 60.0f);
		fopen_s(&m_measureFile, "dash_measure.csv", "w");
		Skeleton& skeleton = m_model.GetModelRender().GetSkeleton();
		for (int i = 0; i < skeleton.GetNumBones(); i++) {
			Bone* bone = skeleton.GetBone(i);
			const std::wstring name = bone->GetName();
			if (name.rfind(L"J_Sec_", 0) != 0 || name.find(L"Skirt") == std::wstring::npos || bone->GetChildren().empty()) {
				continue;
			}
			const int child = bone->GetChildren().front()->GetNo();
			if ((PoseLayers::BonePosition(skeleton, child) - PoseLayers::BonePosition(skeleton, i)).Length() < 0.5f) {
				continue;  // 長さ 0 の骨は、向きが決まらないので外す。
			}
			m_skirtBones.push_back({ i, child });
		}
		m_skirtAnimDir.resize(m_skirtBones.size());
		m_skirtPrevAnimDir.resize(m_skirtBones.size());
		m_skirtPrevFinalDir.resize(m_skirtBones.size());
		if (m_measureFile) {
			fprintf(m_measureFile, "frame,stage,state,posZ,"
				"L_upper,L_lower,L_elbowBend,L_elbowOut,L_handOut,"
				"R_upper,R_lower,R_elbowBend,R_elbowOut,R_handOut,"
				"L_thigh,L_shin,R_thigh,R_shin,"
				"skirt_devMax,skirt_devMean,skirt_jitterMax,skirt_jitterMean,skirt_animJitterMax,skirt_liftMax,skirtCount\n");
		}
	}
	if (!m_isMeasureMode) {
		return;
	}
	m_measureTimer += deltaTime;
	if (m_measureTimer < 1000.0f) {
		if (m_measureTimer >= 1.0f) {
			m_measureFrame = -60;  // 立っている間を 60 フレーム測ってから、ダッシュを出す。
			m_measureTimer = 1000.0f;
		}
		return;
	}
	m_measureFrame++;
	if (m_measureFrame == 0) {
		PlayerDashState::SetUseOldClip(true);
		m_isDebugDashRequested = true;
	}
	if (m_measureFrame > 80) {
		if (m_measureFile) {
			fclose(m_measureFile);
		}
		ExitProcess(0);
	}
}

void Player::OnDebugMeasurePose(Skeleton& skeleton, int stage)
{
	if (m_measureFile == nullptr || m_measureFrame < -60) {
		return;
	}
	ModelRender& model = m_model.GetModelRender();
	auto pos = [&](const wchar_t* name) { return PoseLayers::BonePosition(skeleton, model.FindBoneID(name)); };
	Vector3 forward = m_mover.GetForward();
	forward.y = 0.0f;
	forward.Normalize();
	const Vector3 chest = pos(L"J_Bip_C_UpperChest");

	fprintf(m_measureFile, "%d,%d,%d,%.1f", m_measureFrame, stage, static_cast<int>(m_stateMachine.GetCurrentId()), m_mover.GetPosition().z);
	const wchar_t* arms[2][3] = {
		{ L"J_Bip_L_UpperArm", L"J_Bip_L_LowerArm", L"J_Bip_L_Hand" },
		{ L"J_Bip_R_UpperArm", L"J_Bip_R_LowerArm", L"J_Bip_R_Hand" },
	};
	for (auto& arm : arms) {
		const Vector3 s = pos(arm[0]);
		const Vector3 e = pos(arm[1]);
		const Vector3 h = pos(arm[2]);
		// 体の外向き（胸の真ん中から、その肩の方へ。水平で、前向きに直角）。
		Vector3 out = s - chest;
		out -= forward * out.Dot(forward);
		out.y = 0.0f;
		out.Normalize();
		fprintf(m_measureFile, ",%.1f,%.1f,%.1f,%.1f,%.1f",
			SideAngleDeg(e - s, forward), SideAngleDeg(h - e, forward), AngleBetweenDeg(e - s, h - e),
			(e - s).Dot(out), (h - s).Dot(out));
	}
	const wchar_t* legs[2][3] = {
		{ L"J_Bip_L_UpperLeg", L"J_Bip_L_LowerLeg", L"J_Bip_L_Foot" },
		{ L"J_Bip_R_UpperLeg", L"J_Bip_R_LowerLeg", L"J_Bip_R_Foot" },
	};
	for (auto& leg : legs) {
		const Vector3 hip = pos(leg[0]);
		const Vector3 knee = pos(leg[1]);
		const Vector3 foot = pos(leg[2]);
		fprintf(m_measureFile, ",%.1f,%.1f", SideAngleDeg(knee - hip, forward), SideAngleDeg(foot - knee, forward));
	}

	// スカート: 0 でアニメーションの向きを覚え、2 で揺れ物のあとの向きと比べる。
	float devMax = 0.0f, devSum = 0.0f, jitMax = 0.0f, jitSum = 0.0f, animJitMax = 0.0f, liftMax = 0.0f;
	for (size_t i = 0; i < m_skirtBones.size(); i++) {
		Vector3 dir = PoseLayers::BonePosition(skeleton, m_skirtBones[i].second) - PoseLayers::BonePosition(skeleton, m_skirtBones[i].first);
		dir.Normalize();
		if (stage == 1) {
			if (m_measureFrame > -60) {
				animJitMax = max(animJitMax, AngleBetweenDeg(dir, m_skirtPrevAnimDir[i]));
			}
			m_skirtPrevAnimDir[i] = dir;
			m_skirtAnimDir[i] = dir;
		}
		const float dev = AngleBetweenDeg(dir, m_skirtAnimDir[i]);
		devMax = max(devMax, dev);
		devSum += dev;
		if (stage == 2) {
			if (m_measureFrame > -60) {
				const float jit = AngleBetweenDeg(dir, m_skirtPrevFinalDir[i]);
				if (m_measureFrame == -10 || m_measureFrame == 4 || m_measureFrame == 14 || m_measureFrame == 24 || m_measureFrame == 40) {
					// 骨1本ずつ: 名前・長さ・アニメーション（体の連動のあと）からのずれ・前のフレームからの動き。
					FILE* dump = nullptr;
					fopen_s(&dump, "dash_measure_bones.csv", "a");
					if (dump) {
						const float length = (PoseLayers::BonePosition(skeleton, m_skirtBones[i].second) - PoseLayers::BonePosition(skeleton, m_skirtBones[i].first)).Length();
						fwprintf(dump, L"%d,%ls,%.2f,%.1f,%.1f\n", m_measureFrame, skeleton.GetBone(m_skirtBones[i].first)->GetName(), length, AngleBetweenDeg(dir, m_skirtAnimDir[i]), jit);
						fclose(dump);
					}
				}
				jitMax = max(jitMax, jit);
				jitSum += jit;
			}
			m_skirtPrevFinalDir[i] = dir;
		}
		liftMax = max(liftMax, AngleBetweenDeg(dir, Vector3(0.0f, -1.0f, 0.0f)));
	}
	const float n = m_skirtBones.empty() ? 1.0f : static_cast<float>(m_skirtBones.size());
	fprintf(m_measureFile, ",%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%d\n", devMax, devSum / n, jitMax, jitSum / n, animJitMax, liftMax, static_cast<int>(m_skirtBones.size()));
}
// ───────── 【仮・測る用】ここまで ─────────
