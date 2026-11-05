#pragma once

#include "PlayerGroundState.h"
#include "CharacterMover.h"

// 走っている。walkSpeed × runSpeedMultiplier の速さで、入力の方向へ進む。
class PlayerRunState : public PlayerGroundState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Run;
	}
	const char* GetName() const override
	{
		return "Run";
	}

protected:
	int GetAnimClip() const override
	{
		return enPlayerAnimClip_Run;
	}
	float GetMoveSpeed(const PlayerStateContext& context) const override
	{
		const CharacterMoveSettings& settings = context.mover->GetSettings();
		return settings.walkSpeed * settings.runSpeedMultiplier;
	}
};
