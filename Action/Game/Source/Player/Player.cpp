#include "stdafx.h"
#include "Player.h"
#include "PlayerConfig.h"
#include "IceMagic.h"
#include "FireMagic.h"
#include "MagicConfig.h"

namespace
{
	// ジャンプ中の度合いが 0 から 1（1 から 0）まで変わるのにかかる時間（秒）。アニメーションの切り替えの補間と同じくらいにする。
	constexpr float JUMP_WEIGHT_FADE_SECONDS = 0.2f;
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
}

void Player::OnLateUpdate(float deltaTime)
{
	m_upperBody.Update(
		m_skillController.IsHolding(ActionButton::Action2),
		m_skillController.GetUseCount(ActionButton::Action2),
		m_isThirdPersonCamera,
		deltaTime);
	UpdateChargePosition();
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
	// 構えたら、両手の真ん中の少し前。上半身の層の重みで、胸の前から移す。
	ModelRender& modelRender = m_model.GetModelRender();
	const Matrix& left = modelRender.GetBone(m_handBones[0])->GetWorldMatrix();
	const Matrix& right = modelRender.GetBone(m_handBones[1])->GetWorldMatrix();
	Vector3 handFront = (*(const Vector3*)left.m[3] + *(const Vector3*)right.m[3]) * 0.5f;
	handFront += forward * m_upperBody.GetSettings().ballForward;
	m_chargePosition.Lerp(m_upperBody.GetWeight(), chestFront, handFront);
}

void Player::Respawn()
{
	m_mover.SetPosition(m_fallRespawn.GetRespawnPosition());
	m_mover.Stop();
	m_stateMachine.Reset();
}
