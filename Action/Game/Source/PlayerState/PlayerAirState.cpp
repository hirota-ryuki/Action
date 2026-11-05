#include "stdafx.h"
#include "PlayerAirState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

void PlayerAirState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	if (input.jump.isTrigger) {
		// 地面にいなければ（足場から落ちて入ったときなど）跳ばない。
		context.mover->Jump(context.mover->GetSettings().jumpSpeed);
	}
	context.PlayAnimation(enPlayerAnimClip_Jump);
}

PlayerStateId PlayerAirState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	if (input.rush.isTrigger && context.settings.rushDistance > 0.0f) {
		return PlayerStateId::Rush;
	}
	if (context.mover->IsOnGround()) {
		return PlayerGroundState::SelectByInput(context, input);
	}
	return PlayerStateId::Air;
}

void PlayerAirState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	const CharacterMoveSettings& settings = context.mover->GetSettings();
	const float speed = input.isRun ? settings.walkSpeed * settings.runSpeedMultiplier : settings.walkSpeed;
	context.mover->MoveByInput(input, speed, deltaTime);
}
