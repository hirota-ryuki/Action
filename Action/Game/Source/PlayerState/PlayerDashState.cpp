#include "stdafx.h"
#include "PlayerDashState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

namespace
{
	// 移動入力のデッドゾーン閾値（二乗値）。これ以下なら正面へダッシュする。
	constexpr float kMoveDeadZoneSq = 0.01f;

	// 進んだ割合（0〜1）。時間の割合 t に対して ease-out（最初が速く、だんだん遅くなる）。
	// endRatio は、終わりの速さ ÷ 平均の速さ（0〜1）。速さは平均の (2 - endRatio) 倍から endRatio 倍へ、まっすぐ下がる。
	// endRatio が 0 なら、蹴った瞬間は平均の2倍で、終わりで止まる。
	float CalcDashProgress(float t, float endRatio)
	{
		return endRatio * t + (1.0f - endRatio) * (2.0f * t - t * t);
	}
}

void PlayerDashState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	m_isNextDashQueued = false;
	m_isFirstFrame = true;
	Begin(context, input);
}

void PlayerDashState::Begin(PlayerStateContext& context, const CharacterInput& input)
{
	m_direction = input.moveDirection;
	m_direction.y = 0.0f;
	if (m_direction.LengthSq() <= kMoveDeadZoneSq) {
		m_direction = context.mover->GetForward();
		m_direction.y = 0.0f;
	}
	m_elapsed = 0.0f;
	m_duration = 0.0f;
	m_isLanding = false;
	if (m_direction.LengthSq() > FLT_EPSILON && context.settings.dashSpeed > 0.0f) {
		m_direction.Normalize();
		m_duration = context.settings.dashDistance / context.settings.dashSpeed;
		context.mover->FaceTo(m_direction);
	}

	// 一蹴りのアニメーションを1回流す（ループしないので、終わると宙へ飛び出したポーズで止まる）。
	// 蹴る足を毎回入れ替えるので、連打しても別のクリップへの切り替えになり、最初から流れる。
	// 連打すると、飛び出したポーズから次の蹴りの着地へ dashAnimBlendSeconds で移り、左右交互に蹴って進む。
	m_isLeftKick = !m_isLeftKick;
	const int clip = m_isLeftKick ? enPlayerAnimClip_DashL : enPlayerAnimClip_DashR;
	context.modelRender->PlayAnimation(clip, context.settings.dashAnimBlendSeconds);
	context.modelRender->SetAnimationSpeed(context.settings.dashAnimSpeed);
}

PlayerStateId PlayerDashState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	// 進み切っていない、または次のダッシュを予約している間はダッシュのまま。
	if (!IsFinished() || m_isNextDashQueued || input.dash.isTrigger) {
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

	// 飛び終わっていて予約があれば（着地中でも）、そのときの入力の方向へ次のダッシュを始める。
	if (IsKickFinished() && m_isNextDashQueued) {
		m_isNextDashQueued = false;
		Begin(context, input);
	}

	const PlayerStateSettings& s = context.settings;
	Vector3 velocity = Vector3::Zero;
	if (!IsKickFinished() && deltaTime > FLT_EPSILON) {
		// 前のフレームまでに進んだ割合と、今フレームの終わりの割合の差だけ進む。
		// 最後は割合が1で止まるので、フレームレートによらず dashDistance ちょうど進む。
		const float endRatio = s.dashSpeed > 0.0f ? max(0.0f, min(1.0f, s.dashEndSpeed / s.dashSpeed)) : 0.0f;
		const float before = CalcDashProgress(m_elapsed / m_duration, endRatio);
		m_elapsed = min(m_elapsed + deltaTime, m_duration);
		const float after = CalcDashProgress(m_elapsed / m_duration, endRatio);
		const float distance = (after - before) * s.dashDistance;
		velocity = m_direction * (distance / deltaTime);
		if (IsKickFinished() && !m_isNextDashQueued && !input.dash.isTrigger) {
			BeginLanding(context);
		}
	}
	else if (m_isLanding) {
		// 着地の間は、終わりの速さのまま進む。
		velocity = m_direction * s.dashEndSpeed;
		m_landElapsed += deltaTime;
		if (m_landElapsed >= s.dashLandSeconds) {
			m_isLanding = false;
		}
	}
	context.mover->Move(velocity, deltaTime);
}

void PlayerDashState::BeginLanding(PlayerStateContext& context)
{
	if (context.settings.dashLandSeconds <= 0.0f) {
		return;
	}
	m_isLanding = true;
	m_landElapsed = 0.0f;
	// 蹴った足と反対の足（前に出している足）で、膝を曲げて着地する。
	const int clip = m_isLeftKick ? enPlayerAnimClip_DashLandL : enPlayerAnimClip_DashLandR;
	context.modelRender->PlayAnimation(clip, context.settings.dashLandBlendSeconds);
	context.modelRender->SetAnimationSpeed(1.0f);
}

void PlayerDashState::Exit(PlayerStateContext& context)
{
	// アニメーションの速さは、次のステートの Enter で戻す。
	// ここで戻すと、凍結で止めたアニメーションが動き出してしまう。
	// 次のアニメーションへは長めに補間して、飛び出したポーズが少し残るようにする。
	context.nextBlendSeconds = context.settings.dashExitBlendSeconds;
	// 走り・歩きは、最後に蹴った足に合わせて、前後の足が同じところから始める。
	const PlayerStateSettings& s = context.settings;
	context.nextStartTimes[enPlayerAnimClip_Run] = m_isLeftKick ? s.dashLToRunStartTime : s.dashRToRunStartTime;
	context.nextStartTimes[enPlayerAnimClip_Walk] = m_isLeftKick ? s.dashLToWalkStartTime : s.dashRToWalkStartTime;
	m_elapsed = m_duration;
	m_isLanding = false;
	m_isNextDashQueued = false;
}
