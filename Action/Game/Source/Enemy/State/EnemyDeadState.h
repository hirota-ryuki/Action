#pragma once

#include "EnemyState.h"

// HP がなくなって倒れる。Dead のアニメーションを1回流し、倒れ終わってから settings.vanishDelay 秒で消える
// （damage->isVanished を立てる。体を隠して当たり判定を外すのは持ち主の Enemy）。入力は無視する。
// ここからはどこへも移らない。
class EnemyDeadState : public EnemyState
{
public:
	EnemyStateId GetId() const override
	{
		return EnemyStateId::Dead;
	}
	const char* GetName() const override
	{
		return "Dead";
	}

	void Enter(EnemyStateContext& context, const CharacterInput& input) override;
	EnemyStateId CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const override
	{
		return EnemyStateId::Dead;
	}
	void Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime) override;

private:
	float m_timeAfterFall = 0.0f;  // 倒れ終わってからの秒数。
};
