#include "stdafx.h"
#include "PlayerStateMachine.h"
#include "PlayerIdleState.h"
#include "PlayerWalkState.h"
#include "PlayerRunState.h"
#include "PlayerAirState.h"
#include "PlayerDashState.h"
#include "PlayerStopState.h"
#include "PlayerLandState.h"
#include "PlayerFrozenState.h"

PlayerStateMachine::PlayerStateMachine()
{
	Register<PlayerIdleState>();
	Register<PlayerWalkState>();
	Register<PlayerRunState>();
	Register<PlayerAirState>();
	Register<PlayerDashState>();
	Register<PlayerStopState>();
	Register<PlayerLandState>();
	Register<PlayerFrozenState>();
}

void PlayerStateMachine::OnBeforeUpdate(PlayerStateContext& context, const CharacterInput& input)
{
	// 移動の入力を離したら、ダッシュのあとの「走り続ける」を終える。
	if (input.moveDirection.Length() < context.settings.moveThreshold) {
		context.keepRunAfterDash = false;
	}
}
