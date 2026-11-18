#include "stdafx.h"
#include "Player.h"
#include "PlayerConfig.h"
#include "IceMagic.h"
#include "FireMagic.h"
#include "MagicConfig.h"
#include "SpringBones.h"
#include <imgui/imgui.h>

namespace
{
	// ジャンプ中の度合いが 0 から 1（1 から 0）まで変わるのにかかる時間（秒）。アニメーションの切り替えの補間と同じくらいにする。
	constexpr float JUMP_WEIGHT_FADE_SECONDS = 0.2f;
	// 溜め撃ちを構えて動くとき、進む向きとカメラの向きの内積がこれより大きければ、脚を進む向きへ向けて前歩きにする（上半身はひねる）。
	// 小さければ（後ろの方へ動く）、体はカメラの向きのまま後ろ歩きにする。-0.2 は約 100度。
	constexpr float AIM_TURN_LEGS_MIN_DOT = -0.2f;
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
	CharacterInput input = m_input;
	FilterAimingInput(input);
	m_stateMachine.SetAiming(IsThirdPersonAiming());
	m_stateMachine.Update(input, deltaTime);
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

void Player::OnPostBodyLink(Skeleton& skeleton)
{
	TwistUpperBodyToAim(skeleton);
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
	SpringBones::RotateBoneTree(*bone, toOrigin * rotation * back);
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
