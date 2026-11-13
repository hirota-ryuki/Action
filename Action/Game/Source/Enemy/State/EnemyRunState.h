#pragma once

#include "EnemyGroundState.h"

// 入力（AI が決めた方向）どおりに走っている。AI が isRun を立てたときに入る。
// 速さは CharacterMoveSettings::runSpeedMultiplier 倍になる（CharacterMover が行う）。
class EnemyRunState : public EnemyGroundState
{
public:
	EnemyStateId GetId() const override
	{
		return EnemyStateId::Run;
	}
	const char* GetName() const override
	{
		return "Run";
	}

protected:
	int GetAnimClip() const override
	{
		return enEnemyAnimClip_Run;
	}
};
