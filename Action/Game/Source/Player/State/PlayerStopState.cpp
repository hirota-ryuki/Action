#include "stdafx.h"
#include "PlayerStopState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

namespace
{
	// 1周期 length のループの中で、a と b がどれだけ離れているか（近い方の向きで測る）。
	float LoopDistance(float a, float b, float length)
	{
		float d = fmodf(fabsf(a - b), length);
		return min(d, length - d);
	}
}

void PlayerStopState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	const PlayerStateSettings& s = context.settings;
	const CharacterMoveSettings& move = context.mover->GetSettings();
	m_elapsed = 0.0f;
	m_startSpeed = move.walkSpeed * move.runSpeedMultiplier;
	m_direction = context.mover->GetForward();
	m_direction.y = 0.0f;
	if (m_direction.LengthSq() > FLT_EPSILON) {
		m_direction.Normalize();
	}
	else {
		m_startSpeed = 0.0f;
	}

	// 走りの中で、今いちばん近い「足が前に着くところ」の足で踏ん張る。
	bool isLeft = true;
	if (s.runAnimSeconds > 0.0f) {
		const float time = context.runAnimTime;
		isLeft = LoopDistance(time, s.stopLRunTime, s.runAnimSeconds) <= LoopDistance(time, s.stopRRunTime, s.runAnimSeconds);
	}
	context.nextBlendSeconds = s.stopBlendSeconds;
	context.PlayAnimation(isLeft ? enPlayerAnimClip_StopL : enPlayerAnimClip_StopR);
}

PlayerStateId PlayerStopState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	if (input.dash.isTrigger && context.settings.dashDistance > 0.0f) {
		return PlayerStateId::Dash;
	}
	if (input.jump.isTrigger && context.mover->GetSettings().jumpSpeed > 0.0f && context.mover->IsOnGround()) {
		return PlayerStateId::Air;
	}
	const PlayerStateId next = PlayerGroundState::SelectByInput(context, input);
	if (next != PlayerStateId::Idle) {
		return next;
	}
	return m_elapsed < context.settings.stopSeconds ? PlayerStateId::Stop : PlayerStateId::Idle;
}

void PlayerStopState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	// 滑る速さは、時間の割合に合わせてまっすぐ下げる（フレームの真ん中の時間で測る）。
	const float slide = context.settings.stopSlideSeconds;
	float speed = 0.0f;
	if (slide > 0.0f && m_elapsed < slide) {
		const float mid = min(m_elapsed + deltaTime * 0.5f, slide);
		speed = m_startSpeed * (1.0f - mid / slide);
	}
	m_elapsed += deltaTime;
	context.mover->Move(m_direction * speed, deltaTime);
}

void PlayerStopState::Exit(PlayerStateContext& context)
{
	// 起き上がったポーズから、次の動きへゆっくりつなぐ。
	context.nextBlendSeconds = context.settings.stopExitBlendSeconds;
}
