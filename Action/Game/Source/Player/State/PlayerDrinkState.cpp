#include "stdafx.h"
#include "PlayerDrinkState.h"
#include "PlayerGroundState.h"
#include "PlayerStatus.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

bool PlayerDrinkState::CanDrink(const PlayerStateContext& context)
{
	const PlayerStatus* status = context.status;
	return status != nullptr && status->GetPotionCount() > 0 && status->GetHp() < status->GetMaxHp();
}

void PlayerDrinkState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	m_elapsed = 0.0f;
	m_clip = -1;
	// 飲んだ瞬間にまとめて回復する。
	if (context.status != nullptr && context.status->UsePotion()) {
		context.status->Heal(context.itemSettings.potionHealAmount);
	}
}

PlayerStateId PlayerDrinkState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	if (!context.mover->IsOnGround()) {
		return PlayerStateId::Air;
	}
	if (m_elapsed < context.itemSettings.drinkSeconds) {
		return PlayerStateId::Drink;
	}
	return PlayerGroundState::SelectByInput(context, input);
}

void PlayerDrinkState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	const float walkSpeed = context.mover->GetSettings().walkSpeed;
	const float speed = walkSpeed * context.itemSettings.drinkMoveSpeedRate;
	context.mover->MoveByInput(input, speed, deltaTime);

	// 動いているかでクリップを替える。歩きのクリップは、足が滑らないよう速さに合わせて遅く流す。
	const bool isMoving = input.moveDirection.Length() >= context.settings.moveThreshold;
	const int clip = isMoving ? enPlayerAnimClip_Walk : enPlayerAnimClip_Idle;
	if (clip != m_clip) {
		float animSpeed = 1.0f;
		if (isMoving && context.settings.walkAnimMoveSpeed > 0.0f) {
			animSpeed = speed / context.settings.walkAnimMoveSpeed;
		}
		context.animator->Play(clip, animSpeed);
		m_clip = clip;
	}
	m_elapsed += deltaTime;
}
