#pragma once

#include "PlayerGroundState.h"
#include "CharacterMover.h"

// 歩いている。walkSpeed の速さで、入力の方向へ進む。
// 走りから移ってきて stopAfterRunSeconds のうちに移動の入力を離したら、Idle ではなく Stop（滑って止まる）へ移る。
// 走るボタンと移動キーをほぼ同時に離すと、走るボタンが先に外れて一瞬だけ歩きになるため。
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

	void Enter(PlayerStateContext& context, const CharacterInput& input) override
	{
		m_stopRemaining = context.previousId == PlayerStateId::Run ? context.settings.stopAfterRunSeconds : 0.0f;
		PlayerGroundState::Enter(context, input);
	}
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override
	{
		const PlayerStateId next = PlayerGroundState::CheckTransition(context, input);
		if (next == PlayerStateId::Idle && m_stopRemaining > 0.0f && context.settings.stopSeconds > 0.0f) {
			return PlayerStateId::Stop;
		}
		return next;
	}
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override
	{
		m_stopRemaining -= deltaTime;
		PlayerGroundState::Update(context, input, deltaTime);
	}

private:
	float m_stopRemaining = 0.0f;  // 走りから移ってきたあと、離したら Stop にする残り時間。

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
