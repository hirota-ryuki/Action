#pragma once

#include "EnemyState.h"

// 地面で動くステート（Idle・Move・Run）の共通部分。
// 動き方はどれも入力どおり（CharacterMover::Update。走る入力なら速くなる）で、違いはアニメーションだけ。
//   移動の入力がない → Idle、ある → 走る入力があれば Run、なければ Move
//   fire を押した → Attack（剣を振る）
class EnemyGroundState : public EnemyState
{
public:
	// 入力で行き先を決める。Frozen が解けたあとの行き先にも使う。
	static EnemyStateId SelectByInput(const EnemyStateContext& context, const CharacterInput& input);
	// 死んだら Dead、当たったら（canFlinch のときだけ）Hit を返す。どちらでもなければ Num。
	static EnemyStateId CheckDamage(const EnemyStateContext& context, bool canFlinch);

	void Enter(EnemyStateContext& context, const CharacterInput& input) override;
	EnemyStateId CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const override;
	void Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime) override;

protected:
	virtual int GetAnimClip() const = 0;
	// アニメーションの再生の速さ。ふつうは 1。
	virtual float GetAnimSpeed(const EnemyStateContext& context) const
	{
		return 1.0f;
	}
};
