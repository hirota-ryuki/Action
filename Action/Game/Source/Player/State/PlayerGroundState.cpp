#include "stdafx.h"
#include "PlayerGroundState.h"
#include "PlayerAttackState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

void PlayerGroundState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	context.animator->Play(GetAnimClip(), GetAnimSpeed(context));
}

PlayerStateId PlayerGroundState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	// ダッシュは足場があるときだけ（空中ではできない）。
	if (input.dash.isTrigger && context.settings.dashDistance > 0.0f && context.mover->IsOnGround()) {
		return PlayerStateId::Dash;
	}
	if (input.jump.isTrigger && context.mover->GetSettings().jumpSpeed > 0.0f && context.mover->IsOnGround()) {
		return PlayerStateId::Air;
	}
	if (PlayerAttackState::CanStart(context, input)) {
		return PlayerStateId::Attack;
	}
	return SelectByInput(context, input);
}

void PlayerGroundState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	context.mover->MoveByInput(input, GetMoveSpeed(context), deltaTime);
}

PlayerStateId PlayerGroundState::SelectByInput(const PlayerStateContext& context, const CharacterInput& input)
{
	if (!context.mover->IsOnGround()) {
		return PlayerStateId::Air;
	}
	if (input.moveDirection.Length() < context.settings.moveThreshold) {
		return PlayerStateId::Idle;
	}
	// ダッシュのあとは、走るボタンを離していても、移動の入力を離すまで走る。
	if ((input.isRun || context.keepRunAfterDash) && context.mover->GetSettings().runSpeedMultiplier != 1.0f) {
		return PlayerStateId::Run;
	}
	return PlayerStateId::Walk;
}
