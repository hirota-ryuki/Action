#include "stdafx.h"
#include "PlayerDashState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

namespace
{
	// 移動入力のデッドゾーン閾値（二乗値）。これ以下なら正面へダッシュする。
	constexpr float kMoveDeadZoneSq = 0.01f;
}

void PlayerDashState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	m_isNextDashQueued = false;
	m_isFirstFrame = true;
	Begin(context, input);

	// 体全体を前へ倒した走りを、速めに流す。補間を短くして、一気に前へ出たように見せる。
	// 連打で続けて出るときはステートを出ないので、ここは通らず、同じモーションが途切れずに続く。
	context.modelRender->PlayAnimation(enPlayerAnimClip_Dash, context.settings.dashAnimBlendSeconds);
	context.modelRender->SetAnimationSpeed(context.settings.dashAnimSpeed);
}

void PlayerDashState::Begin(PlayerStateContext& context, const CharacterInput& input)
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
		m_remainingDistance = context.settings.dashDistance;
		context.mover->FaceTo(m_direction);
	}
}

PlayerStateId PlayerDashState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	// 進み切っていない、または次のダッシュを予約している間はダッシュのまま。
	if (m_remainingDistance > 0.0f || m_isNextDashQueued || input.dash.isTrigger) {
		return PlayerStateId::Dash;
	}
	return PlayerGroundState::SelectByInput(context, input);
}

void PlayerDashState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	// 入ったフレームの押下は、このダッシュを始めた押下なので数えない。
	if (input.dash.isTrigger && !m_isFirstFrame && context.settings.dashDistance > 0.0f) {
		m_isNextDashQueued = true;
	}
	m_isFirstFrame = false;

	// 進み切っていて予約があれば、そのときの入力の方向へ次のダッシュを始める。
	if (m_remainingDistance <= 0.0f && m_isNextDashQueued) {
		m_isNextDashQueued = false;
		Begin(context, input);
	}

	Vector3 velocity = Vector3::Zero;
	if (m_remainingDistance > 0.0f && deltaTime > FLT_EPSILON) {
		// 今フレームの移動距離が残り距離を超えないよう、小さい方の値を使う。
		const float maxDistance = context.settings.dashSpeed * deltaTime;
		const float distance = maxDistance < m_remainingDistance ? maxDistance : m_remainingDistance;
		velocity = m_direction * (distance / deltaTime);
		m_remainingDistance -= distance;
	}
	context.mover->Move(velocity, deltaTime);
}

void PlayerDashState::Exit(PlayerStateContext& context)
{
	// アニメーションの速さは、次のステートの Enter で戻す。
	// ここで戻すと、凍結で止めたアニメーションが動き出してしまう。
	m_remainingDistance = 0.0f;
	m_isNextDashQueued = false;
}
