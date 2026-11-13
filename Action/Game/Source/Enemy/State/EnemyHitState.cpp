#include "stdafx.h"
#include "EnemyHitState.h"
#include "EnemyGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "CharacterModel.h"

namespace
{
	constexpr float HIT_BLEND_SECONDS = 0.1f;  // ひるみは素早く切り替える。
}

void EnemyHitState::Enter(EnemyStateContext& context, const CharacterInput& input)
{
	context.model->PlayAnimation(enEnemyAnimClip_Hit, HIT_BLEND_SECONDS);
	context.model->GetModelRender().SetAnimationSpeed(context.settings.playSpeed);
}

EnemyStateId EnemyHitState::CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const
{
	if (EnemyGroundState::CheckDamage(context, false) == EnemyStateId::Dead) {
		return EnemyStateId::Dead;
	}
	if (context.model->IsPlayingAnimation()) {
		return EnemyStateId::Hit;
	}
	return EnemyGroundState::SelectByInput(context, input);
}

void EnemyHitState::Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime)
{
	context.mover->Update(CharacterInput(), deltaTime);
}
