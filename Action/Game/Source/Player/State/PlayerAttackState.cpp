#include "stdafx.h"
#include "PlayerAttackState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

namespace
{
	// 段の数（Attack1〜Attack3）。
	constexpr int STAGE_COUNT = 3;
}

bool PlayerAttackState::CanStart(const PlayerStateContext& context, const CharacterInput& input)
{
	return input.fire.isTrigger && !input.hasAim && context.mover->IsOnGround();
}

void PlayerAttackState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	StartStage(context, input, 0);
	m_isFirstFrame = true;
}

PlayerStateId PlayerAttackState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	const PlayerStateSettings& s = context.settings;
	if (!context.mover->IsOnGround()) {
		return PlayerStateId::Air;
	}
	if (!IsCancelable(context)) {
		return PlayerStateId::Attack;
	}
	// 撃ち終わりからは、ほかの動きで打ち切れる。
	if (input.dash.isTrigger && s.dashDistance > 0.0f) {
		return PlayerStateId::Dash;
	}
	if (input.jump.isTrigger && context.mover->GetSettings().jumpSpeed > 0.0f) {
		return PlayerStateId::Air;
	}
	if (WillChain(context)) {
		return PlayerStateId::Attack;  // Update で次の段を始める。
	}
	const PlayerStateId next = PlayerGroundState::SelectByInput(context, input);
	if (next != PlayerStateId::Idle) {
		return next;
	}
	return m_clipTime < s.attackEndTime ? PlayerStateId::Attack : PlayerStateId::Idle;
}

void PlayerAttackState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	if (WillChain(context)) {
		StartStage(context, input, m_stage + 1);
	}
	// この段を始めたクリックは数えない。それより後のクリックで、次の段を予約する。
	if (!m_isFirstFrame && input.fire.isTrigger) {
		m_isNextQueued = true;
	}
	m_isFirstFrame = false;

	m_clipTime += deltaTime * context.settings.attackAnimSpeed;
	// 手を突き出したコマで撃つ。Player がこれを見て、このフレームだけ魔法の発射ボタンを押したことにする。
	if (!m_hasFired && m_clipTime >= context.settings.attackFireTime) {
		m_hasFired = true;
		context.attackFire = true;
	}
	// 立ち止まって撃つ。
	context.mover->Move(Vector3::Zero, deltaTime);
}

void PlayerAttackState::Exit(PlayerStateContext& context)
{
	context.animator->SetNextBlend(context.settings.attackExitBlendSeconds);
}

void PlayerAttackState::StartStage(PlayerStateContext& context, const CharacterInput& input, int stage)
{
	m_stage = stage;
	m_clipTime = 0.0f;
	m_hasFired = false;
	m_isNextQueued = false;
	// 撃つ瞬間に、カメラが向いている方向を向く（見た目もすぐに）。
	context.mover->FaceTo(input.attackDirection);
	context.animator->SetNextBlend(context.settings.attackBlendSeconds);
	context.animator->Play(enPlayerAnimClip_Attack1 + stage, context.settings.attackAnimSpeed);
}

bool PlayerAttackState::IsCancelable(const PlayerStateContext& context) const
{
	return m_hasFired && m_clipTime >= context.settings.attackCancelTime;
}

bool PlayerAttackState::WillChain(const PlayerStateContext& context) const
{
	return m_isNextQueued && m_stage + 1 < STAGE_COUNT && IsCancelable(context);
}
