#pragma once

#include "EnemyGroundState.h"

// 入力（AI が決めた方向）どおりに進んでいる。
class EnemyMoveState : public EnemyGroundState
{
public:
	EnemyStateId GetId() const override
	{
		return EnemyStateId::Move;
	}
	const char* GetName() const override
	{
		return "Move";
	}

protected:
	int GetAnimClip() const override
	{
		return enEnemyAnimClip_Run;
	}
};
