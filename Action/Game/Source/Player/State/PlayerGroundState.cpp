#include "stdafx.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "PlayerDrinkState.h"
#include "ItemPicker.h"

void PlayerGroundState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	context.animator->Play(GetAnimClip(), GetAnimSpeed(context));
}

PlayerStateId PlayerGroundState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	// ダッシュは足場があるときだけ（空中ではできない）。
	if (input.dash.isTrigger && context.settings.dashDistance > 0.0f && context.mover->IsOnGround()) {
		return PlayerStateId::Dash;
	}
	if (input.jump.isTrigger && context.mover->GetSettings().jumpSpeed > 0.0f && context.mover->IsOnGround()) {
		return PlayerStateId::Air;
	}
	if (input.useItem.isTrigger && context.mover->IsOnGround() && PlayerDrinkState::CanDrink(context)) {
		return PlayerStateId::Drink;
	}
	return SelectByInput(context, input);
}

void PlayerGroundState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	context.mover->MoveByInput(input, GetMoveSpeed(context), deltaTime);
	UpdatePickUp(context, input, deltaTime);
}

void PlayerGroundState::UpdatePickUp(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	if (context.picker == nullptr || context.status == nullptr) {
		return;
	}
	if (!input.interact.isPress) {
		context.pickupHoldSeconds = 0.0f;
		return;
	}
	const PlayerItemSettings& s = context.itemSettings;
	if (input.interact.isTrigger) {
		context.picker->PickUpFirst(*context.status);
		context.pickupHoldSeconds = 0.0f;
		context.nextPickupSeconds = s.pickupHoldDelaySeconds;
		return;
	}
	// 押し続けている。間隔が 0 以下なら、まとめては拾わない。
	if (s.pickupHoldIntervalSeconds <= 0.0f) {
		return;
	}
	context.pickupHoldSeconds += deltaTime;
	while (context.pickupHoldSeconds >= context.nextPickupSeconds && context.picker->HasCandidate()) {
		context.picker->PickUpFirst(*context.status);
		context.nextPickupSeconds += s.pickupHoldIntervalSeconds;
	}
	// 拾えるものがない間は、次に拾う時間を進めておく（あとで近づいたときに、たまった分を一度に拾わないように）。
	if (!context.picker->HasCandidate()) {
		context.nextPickupSeconds = max(context.nextPickupSeconds, context.pickupHoldSeconds);
	}
}

PlayerStateId PlayerGroundState::SelectByInput(const PlayerStateContext& context, const CharacterInput& input)
{
	if (!context.mover->IsOnGround()) {
		return PlayerStateId::Air;
	}
	if (input.moveDirection.Length() < context.settings.moveThreshold) {
		return PlayerStateId::Idle;
	}
	// ダッシュのあとは、走るボタンを離していても、移動の入力を離すまで走る。
	if ((input.isRun || context.keepRunAfterDash) && context.mover->GetSettings().runSpeedMultiplier != 1.0f) {
		return PlayerStateId::Run;
	}
	return PlayerStateId::Walk;
}
