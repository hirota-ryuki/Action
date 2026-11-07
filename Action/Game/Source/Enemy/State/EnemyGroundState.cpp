#include "stdafx.h"
#include "EnemyGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "CharacterModel.h"

EnemyStateId EnemyGroundState::SelectByInput(const EnemyStateContext& context, const CharacterInput& input)
{
	if (input.moveDirection.Length() < context.settings.moveThreshold) {
		return EnemyStateId::Idle;
	}
	return EnemyStateId::Move;
}

void EnemyGroundState::Enter(EnemyStateContext& context, const CharacterInput& input)
{
	context.model->PlayAnimation(GetAnimClip(), context.settings.animBlendSeconds);
}

EnemyStateId EnemyGroundState::CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const
{
	return SelectByInput(context, input);
}

void EnemyGroundState::Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime)
{
	context.mover->Update(input, deltaTime);
}
