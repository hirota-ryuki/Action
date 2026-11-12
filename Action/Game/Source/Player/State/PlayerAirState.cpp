#include "stdafx.h"
#include "PlayerAirState.h"
#include "PlayerGroundState.h"
#include "PlayerLandState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

void PlayerAirState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	const PlayerStateSettings& s = context.settings;
	// 地面にいなければ（足場から落ちて入ったときなど）跳ばない。
	m_isJumpStart = input.jump.isTrigger && context.mover->Jump(context.mover->GetSettings().jumpSpeed);
	if (m_isJumpStart) {
		m_clipTime = s.jumpStartTime;
		context.animator->SetNextBlend(s.jumpStartBlendSeconds);
		context.animator->SetNextStartTime(enPlayerAnimClip_JumpStart, s.jumpStartTime);
		context.animator->Play(enPlayerAnimClip_JumpStart, s.jumpAnimSpeed);
	}
	else {
		context.animator->SetNextBlend(s.jumpLoopBlendSeconds);
		context.animator->Play(enPlayerAnimClip_JumpLoop);
	}
}

PlayerStateId PlayerAirState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	// 空中ではダッシュできない（着地してから）。
	if (context.mover->IsOnGround()) {
		return PlayerLandState::SelectOnLand(context, input);
	}
	return PlayerStateId::Air;
}

void PlayerAirState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	const PlayerStateSettings& s = context.settings;
	if (m_isJumpStart) {
		m_clipTime += deltaTime * s.jumpAnimSpeed;
		if (m_clipTime >= s.jumpToLoopTime) {
			// JumpStart の最後は JumpLoop の最初と同じポーズなので、JumpLoop は最初から流す。
			m_isJumpStart = false;
			context.animator->SetNextBlend(s.jumpLoopBlendSeconds);
			context.animator->Play(enPlayerAnimClip_JumpLoop);
		}
	}

	const CharacterMoveSettings& settings = context.mover->GetSettings();
	const bool isRun = input.isRun || context.keepRunAfterDash;
	const float speed = isRun ? settings.walkSpeed * settings.runSpeedMultiplier : settings.walkSpeed;
	context.mover->MoveByInput(input, speed, deltaTime);
}
