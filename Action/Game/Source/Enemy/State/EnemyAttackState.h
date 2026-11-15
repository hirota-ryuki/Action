#pragma once

#include "EnemyState.h"

// 剣で攻撃している。AI が Action1〜Action3 を押したときに入る。
// 体を動かすのは攻撃（AttackBase の派生。SkillController がボタンに合わせて作る）で、このステートは攻撃が終わるまで待つだけ。
//   攻撃が終わった（SkillController が空いた） → 入力に合わせて Idle / Move / Run
//   死んだ → Dead（出るときに攻撃を止める）
// 攻撃中に当たってもひるまない。
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

	EnemyStateId CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const override;
	// 何もしない。体を動かす（重力をかける Move を呼ぶ）のは攻撃。
	void Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime) override
	{
	}
	void Exit(EnemyStateContext& context) override;
};
