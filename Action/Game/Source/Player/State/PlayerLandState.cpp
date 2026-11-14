#include "stdafx.h"
#include "PlayerLandState.h"
#include "PlayerGroundState.h"
#include "PlayerAttackState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

PlayerStateId PlayerLandState::SelectOnLand(const PlayerStateContext& context, const CharacterInput& input)
{
	const PlayerStateSettings& s = context.settings;
	const PlayerStateId next = PlayerGroundState::SelectByInput(context, input);
	if (next == PlayerStateId::Air) {
		return next;
	}
	// 動きながら着地したら、しゃがまずにそのまま歩く・走る。
	if (next != PlayerStateId::Idle && s.landCancelByMove) {
		return next;
	}
	if (s.landEndTime <= s.landStartTime) {
		return next;
	}
	return PlayerStateId::Land;
}

void PlayerLandState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	const PlayerStateSettings& s = context.settings;
	m_clipTime = s.landStartTime;
	context.animator->SetNextBlend(s.landBlendSeconds);
	context.animator->SetNextStartTime(enPlayerAnimClip_JumpLand, s.landStartTime);
	context.animator->Play(enPlayerAnimClip_JumpLand, s.landAnimSpeed);
}

PlayerStateId PlayerLandState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	const PlayerStateSettings& s = context.settings;
	if (input.dash.isTrigger && s.dashDistance > 0.0f && context.mover->IsOnGround()) {
		return PlayerStateId::Dash;
	}
	if (input.jump.isTrigger && context.mover->GetSettings().jumpSpeed > 0.0f && context.mover->IsOnGround()) {
		return PlayerStateId::Air;
	}
	if (PlayerAttackState::CanStart(context, input)) {
		return PlayerStateId::Attack;
	}
	const PlayerStateId next = PlayerGroundState::SelectByInput(context, input);
	if (next == PlayerStateId::Air) {
		return next;
	}
	if (next != PlayerStateId::Idle && s.landCancelByMove) {
		return next;
	}
	return m_clipTime < s.landEndTime ? PlayerStateId::Land : next;
}

void PlayerLandState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	m_clipTime += deltaTime * context.settings.landAnimSpeed;
	context.mover->Move(Vector3::Zero, deltaTime);
}

void PlayerLandState::Exit(PlayerStateContext& context)
{
	// しゃがんだ・立ち上がりかけのポーズから、次の動きへゆっくりつなぐ。
	context.animator->SetNextBlend(context.settings.landExitBlendSeconds);
}
