#pragma once

#include "EnemyState.h"

// 剣で攻撃している。AI が fire を押したときに入り、input.attackIndex の攻撃のアニメーションを1回流す。
//   攻撃1  突進して突き → 回転斬り。突きのあいだ、前へ速く進む
//   攻撃2  左上→右下、右上→左下と斬り下ろし → 横一閃。その場で振る
//   攻撃3  斜めの斬り下ろしを5回。振るたびに、斬撃（SlashBullet）を相手へ飛ばす
// 入ったときに相手の方を向き、あとは向きを変えない。剣が当たったときのダメージはまだない。
//   アニメーションが終わった → 入力に合わせて Idle / Move / Run
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

private:
	void FireSlash(const EnemyStateContext& context, const CharacterInput& input, float rollDeg) const;

	int m_attackIndex = 0;     // 今している攻撃（0〜2）。
	float m_time = 0.0f;       // 攻撃を始めてからの秒数。
	int m_nextSlash = 0;       // 次に飛ばす斬撃の番号。
};
