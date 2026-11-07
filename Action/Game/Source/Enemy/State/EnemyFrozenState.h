#pragma once

#include "EnemyState.h"

// 凍結などで行動できない。入力を無視し、重力で落ちるだけ。
// 行動できなくなったら、どのステートからでも StateMachine がここへ移す。
// アニメーションの停止と色の変化は StatusEffects が行うので、ここでは触らない。
//   行動できるようになった → 入力に合わせて Idle / Move
class EnemyFrozenState : public EnemyState
{
public:
	EnemyStateId GetId() const override
	{
		return EnemyStateId::Frozen;
	}
	const char* GetName() const override
	{
		return "Frozen";
	}

	EnemyStateId CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const override;
	void Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime) override;
};
