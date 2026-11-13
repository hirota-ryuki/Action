#pragma once

#include "EnemyState.h"

// 攻撃を受けてひるんでいる。Hit のアニメーションを1回流し、その間は動かない（重力はかかる）。
// 攻撃中・凍結中に受けた攻撃ではここに来ない（攻撃はひるまずに続ける）。ひるんでいる間に受けた攻撃では、やり直さない。
//   HP がなくなった → Dead
//   アニメーションが終わった → 入力に合わせて Idle / Move / Run
class EnemyHitState : public EnemyState
{
public:
	EnemyStateId GetId() const override
	{
		return EnemyStateId::Hit;
	}
	const char* GetName() const override
	{
		return "Hit";
	}

	void Enter(EnemyStateContext& context, const CharacterInput& input) override;
	EnemyStateId CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const override;
	void Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime) override;
};
