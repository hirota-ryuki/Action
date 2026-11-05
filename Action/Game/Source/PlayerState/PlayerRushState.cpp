#include "stdafx.h"
#include "PlayerRushState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

namespace
{
	// 移動入力のデッドゾーン閾値（二乗値）。これ以下なら正面へラッシュする。
	constexpr float kMoveDeadZoneSq = 0.01f;
}

void PlayerRushState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	m_isNextRushQueued = false;
	m_isFirstFrame = true;
	Begin(context, input);

	// 疾走ポーズで止める。
	context.modelRender->PlayAnimationAtTime(enPlayerAnimClip_Run, context.settings.rushPoseTime);
	context.modelRender->SetAnimationSpeed(0.0f);
}

void PlayerRushState::Begin(PlayerStateContext& context, const CharacterInput& input)
{
	m_direction = input.moveDirection;
	m_direction.y = 0.0f;
	if (m_direction.LengthSq() <= kMoveDeadZoneSq) {
		m_direction = context.mover->GetForward();
		m_direction.y = 0.0f;
	}
	m_remainingDistance = 0.0f;
	if (m_direction.LengthSq() > FLT_EPSILON) {
		m_direction.Normalize();
		m_remainingDistance = context.settings.rushDistance;
		context.mover->FaceTo(m_direction);
	}
}

PlayerStateId PlayerRushState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	// 進み切っていない、または次のラッシュを予約している間はラッシュのまま。
	if (m_remainingDistance > 0.0f || m_isNextRushQueued || input.rush.isTrigger) {
		return PlayerStateId::Rush;
	}
	return PlayerGroundState::SelectByInput(context, input);
}

void PlayerRushState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	// 入ったフレームの押下は、このラッシュを始めた押下なので数えない。
	if (input.rush.isTrigger && !m_isFirstFrame && context.settings.rushDistance > 0.0f) {
		m_isNextRushQueued = true;
	}
	m_isFirstFrame = false;

	// 進み切っていて予約があれば、そのときの入力の方向へ次のラッシュを始める。
	if (m_remainingDistance <= 0.0f && m_isNextRushQueued) {
		m_isNextRushQueued = false;
		Begin(context, input);
	}

	Vector3 velocity = Vector3::Zero;
	if (m_remainingDistance > 0.0f && deltaTime > FLT_EPSILON) {
		// 今フレームの移動距離が残り距離を超えないよう、小さい方の値を使う。
		const float maxDistance = context.settings.rushSpeed * deltaTime;
		const float distance = maxDistance < m_remainingDistance ? maxDistance : m_remainingDistance;
		velocity = m_direction * (distance / deltaTime);
		m_remainingDistance -= distance;
	}
	context.mover->Move(velocity, deltaTime);
}

void PlayerRushState::Exit(PlayerStateContext& context)
{
	// アニメーションの速さは、次のステートの Enter で戻す。
	// ここで戻すと、凍結で止めたアニメーションが動き出してしまう。
	m_remainingDistance = 0.0f;
	m_isNextRushQueued = false;
}
