#include "stdafx.h"
#include "EnemyAttackState.h"
#include "EnemyGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "CharacterModel.h"

void EnemyAttackState::Enter(EnemyStateContext& context, const CharacterInput& input)
{
	context.model->PlayAnimation(enEnemyAnimClip_Attack, context.settings.animBlendSeconds);
}

EnemyStateId EnemyAttackState::CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const
{
	if (context.model->IsPlayingAnimation()) {
		return EnemyStateId::Attack;
	}
	return EnemyGroundState::SelectByInput(context, input);
}

void EnemyAttackState::Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime)
{
	// 振っている間は進まない。
	context.mover->Update(CharacterInput(), deltaTime);
}
