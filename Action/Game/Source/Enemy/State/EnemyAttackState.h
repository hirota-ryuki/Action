#pragma once

#include "EnemyState.h"

// 剣で攻撃している。AI が Action1 を押したときに入り、input.attackIndex の攻撃のアニメーションを1回流す。
//   攻撃1  左腰に剣を構えてかがみ、そこで溜めてから（秒数は tuning）、一気に踏み込んで横に斬り抜ける。
//          アニメーションの中で腰が進む分（約3.7m）だけ、黒騎士ごと前へ動かす
//   攻撃2  片手剣の重い連続斬り（KinemIQ の Sword - heavy combo）。その場で振る
//   攻撃3  連続斬り（KinemIQ の slash combo）。剣が前を通る5回の振りで、斬撃（SlashMagic）を相手へ飛ばす
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
	bool m_isCharging = false;   // 構えのコマで止めて溜めている。
	bool m_isChargeDone = false; // 溜め終わった。
	float m_chargeLeft = 0.0f;   // 溜めの残りの秒数。
};
