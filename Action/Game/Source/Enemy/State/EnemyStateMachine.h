#pragma once

#include "EnemyState.h"
#include "StateMachine.h"

// 敵のステートマシン。切り替えの手順と凍結の扱いは共通の StateMachine にある。
// ここでは4つのステートを登録するだけ。
class EnemyStateMachine : public StateMachine<EnemyStateId, EnemyStateContext>
{
public:
	EnemyStateMachine();

	// mover・model・statusEffects は初期化済みのものを渡す。Idle から始める。
	void Init(const EnemyStateContext& context)
	{
		StateMachine::Init(context, EnemyStateId::Idle);
	}
};
