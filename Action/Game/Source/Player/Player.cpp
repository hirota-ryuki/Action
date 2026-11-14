#include "stdafx.h"
#include "Player.h"
#include "PlayerConfig.h"
#include "IceMagic.h"
#include "FireMagic.h"

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
	m_stateMachine.Init(stateContext);  // Idle から始める。

	m_skillController.Register<IceMagic>(ActionButton::Action1);
	m_skillController.Register<FireMagic>(ActionButton::Action2);
}

void Player::FillInputContext(InputSourceContext& context) const
{
	context.isFirstPerson = !IsRenderBody();
}

void Player::UpdateAction(float deltaTime)
{
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	m_stateMachine.Update(m_input, deltaTime);
	UpdateJumpWeight(deltaTime);
	UpdateAttackWeight(deltaTime);
	if (m_fallRespawn.IsFallen(m_mover.GetPosition())) {
		Respawn();
	}
}

void Player::FilterSkillInput(CharacterInput& input) const
{
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
	SetAttackWeight(m_attackWeight);
}

void Player::Respawn()
{
	m_mover.SetPosition(m_fallRespawn.GetRespawnPosition());
	m_mover.Stop();
	m_stateMachine.Reset();
}
