#include "stdafx.h"
#include "EnemyDeadState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "CharacterModel.h"

namespace
{
	constexpr float DEAD_BLEND_SECONDS = 0.1f;
}

void EnemyDeadState::Enter(EnemyStateContext& context, const CharacterInput& input)
{
	m_timeAfterFall = 0.0f;
	context.model->PlayAnimation(enEnemyAnimClip_Dead, DEAD_BLEND_SECONDS);
	context.model->GetModelRender().SetAnimationSpeed(context.settings.playSpeed);
}

void EnemyDeadState::Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime)
{
	context.mover->Update(CharacterInput(), deltaTime);
	if (context.model->IsPlayingAnimation()) {
		return;
	}
	m_timeAfterFall += deltaTime;
	if (m_timeAfterFall >= context.settings.vanishDelay && context.damage != nullptr) {
		context.damage->isVanished = true;
	}
}
