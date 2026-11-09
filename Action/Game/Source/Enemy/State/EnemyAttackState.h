#pragma once

#include "EnemyState.h"

// 剣を振っている。AI が fire（撃つ）を押したときに入り、Attack のアニメーションを1回流す。
// 振っている間は立ち止まる（重力で落ちるだけ）。剣が当たったときのダメージはまだない。
//   アニメーションが終わった → 入力に合わせて Idle / Move
class EnemyAttackState : public EnemyState
{
public:
	EnemyStateId GetId() const override
	{
		return EnemyStateId::Attack;
	}
	const char* GetName() const override
	{
		return "Attack";
	}

	void Enter(EnemyStateContext& context, const CharacterInput& input) override;
	EnemyStateId CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const override;
	void Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime) override;
};
