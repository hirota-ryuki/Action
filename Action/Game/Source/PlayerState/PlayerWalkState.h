#pragma once

#include "PlayerGroundState.h"
#include "CharacterMover.h"

// 歩いている。walkSpeed の速さで、入力の方向へ進む。
class PlayerWalkState : public PlayerGroundState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Walk;
	}
	const char* GetName() const override
	{
		return "Walk";
	}

protected:
	int GetAnimClip() const override
	{
		return enPlayerAnimClip_Walk;
	}
	float GetMoveSpeed(const PlayerStateContext& context) const override
	{
		return context.mover->GetSettings().walkSpeed;
	}
	// 足が地面を送る速さと、実際に進む速さをそろえる。
	float GetAnimSpeed(const PlayerStateContext& context) const override
	{
		const float animMoveSpeed = context.settings.walkAnimMoveSpeed;
		if (animMoveSpeed <= 0.0f) {
			return 1.0f;
		}
		return GetMoveSpeed(context) / animMoveSpeed;
	}
};
