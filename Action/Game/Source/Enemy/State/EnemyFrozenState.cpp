#include "stdafx.h"
#include "EnemyFrozenState.h"
#include "EnemyGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

EnemyStateId EnemyFrozenState::CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const
{
	// 行動できない間は StateMachine がここを呼ばないので、呼ばれたら解けている。
	// 凍っている間に受けた攻撃ではひるまない。HP がなくなっていたら倒れる。
	if (EnemyGroundState::CheckDamage(context, false) == EnemyStateId::Dead) {
		return EnemyStateId::Dead;
	}
	return EnemyGroundState::SelectByInput(context, input);
}

void EnemyFrozenState::Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime)
{
	// 入力を無視し、重力で落ちるだけにする。
	context.mover->Update(CharacterInput(), deltaTime);
}
