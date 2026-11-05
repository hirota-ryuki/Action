#include "stdafx.h"
#include "PlayerFrozenState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

PlayerStateId PlayerFrozenState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	// 行動できない間は PlayerStateMachine がここを呼ばないので、呼ばれたら解けている。
	return PlayerGroundState::SelectByInput(context, input);
}

void PlayerFrozenState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	context.mover->Move(Vector3::Zero, deltaTime);
}
