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
	if (input.isRun) {
		return EnemyStateId::Run;
	}
	return EnemyStateId::Move;
}

void EnemyGroundState::Enter(EnemyStateContext& context, const CharacterInput& input)
{
	context.model->PlayAnimation(GetAnimClip(), context.settings.animBlendSeconds);
	context.model->GetModelRender().SetAnimationSpeed(GetAnimSpeed(context) * context.settings.playSpeed);
}

EnemyStateId EnemyGroundState::CheckDamage(const EnemyStateContext& context, bool canFlinch)
{
	if (context.damage == nullptr) {
		return EnemyStateId::Num;
	}
	if (context.damage->IsDead()) {
		return EnemyStateId::Dead;
	}
	// 当たったことは、ひるまないときも下ろしておく（攻撃が終わったあとに遅れてひるまないように）。
	if (context.damage->ConsumeHit() && canFlinch) {
		return EnemyStateId::Hit;
	}
	return EnemyStateId::Num;
}

EnemyStateId EnemyGroundState::CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const
{
	const EnemyStateId damaged = CheckDamage(context, true);
	if (damaged != EnemyStateId::Num) {
		return damaged;
	}
	if (input.fire.isTrigger) {
		return EnemyStateId::Attack;
	}
	return SelectByInput(context, input);
}

void EnemyGroundState::Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime)
{
	context.mover->Update(input, deltaTime);
}
