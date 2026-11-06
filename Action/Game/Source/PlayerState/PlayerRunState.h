#pragma once

#include "PlayerGroundState.h"
#include "CharacterMover.h"

// 走っている。walkSpeed × runSpeedMultiplier の速さで、入力の方向へ進む。
// 移動の入力を離したら、すぐ立ち止まらずに Stop（勢いで滑りながら踏ん張って止まる）へ移る。
// 走りのアニメーションの位置を context.runAnimTime に書いておく（Stop が踏ん張る足を決めるのに使う）。
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

	void Enter(PlayerStateContext& context, const CharacterInput& input) override
	{
		// 再生を始める位置は、PlayAnimation が使ったら消すので、先に読んでおく。
		const float startTime = context.nextStartTimes[enPlayerAnimClip_Run];
		context.runAnimTime = startTime >= 0.0f ? startTime : 0.0f;
		PlayerGroundState::Enter(context, input);
	}
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override
	{
		const PlayerStateId next = PlayerGroundState::CheckTransition(context, input);
		if (next == PlayerStateId::Idle && context.settings.stopSeconds > 0.0f) {
			return PlayerStateId::Stop;
		}
		return next;
	}
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override
	{
		PlayerGroundState::Update(context, input, deltaTime);
		context.runAnimTime += deltaTime * GetAnimSpeed(context);
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
