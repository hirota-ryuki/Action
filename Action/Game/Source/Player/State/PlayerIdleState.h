#pragma once

#include "PlayerGroundState.h"

// 立ち止まっている。移動はしないが、向きたい方向（一人称の視線など）には向く。
class PlayerIdleState : public PlayerGroundState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Idle;
	}
	const char* GetName() const override
	{
		return "Idle";
	}

protected:
	int GetAnimClip() const override
	{
		return enPlayerAnimClip_Idle;
	}
	float GetMoveSpeed(const PlayerStateContext& context) const override
	{
		return 0.0f;
	}
};
