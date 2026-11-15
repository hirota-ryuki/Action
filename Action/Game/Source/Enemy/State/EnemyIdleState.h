#pragma once

#include "EnemyGroundState.h"

// 立ち止まっている。
class EnemyIdleState : public EnemyGroundState
{
public:
	EnemyStateId GetId() const override
	{
		return EnemyStateId::Idle;
	}

protected:
	int GetAnimClip() const override
	{
		return enEnemyAnimClip_Idle;
	}
};
