#include "stdafx.h"
#include "PlayerDashState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

namespace
{
	// 移動入力のデッドゾーン閾値（二乗値）。これ以下なら正面へダッシュする。
	constexpr float kMoveDeadZoneSq = 0.01f;

	// 進んだ割合（0〜1）。時間の割合 t に対して ease-out（最初が速く、最後に止まる）。
	// 速さは 2(1-t) 倍になる：蹴った瞬間は平均の2倍、終わりで0。
	float CalcDashProgress(float t)
	{
		const float rest = 1.0f - t;
		return 1.0f - rest * rest;
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
	if (m_direction.LengthSq() > FLT_EPSILON && context.settings.dashSpeed > 0.0f) {
		m_direction.Normalize();
		m_duration = context.settings.dashDistance / context.settings.dashSpeed;
		context.mover->FaceTo(m_direction);
	}

	// 一蹴りのアニメーションを1回流す（ループしないので、終わると蹴り切ったポーズで止まる）。
	// 蹴る足を毎回入れ替えるので、連打しても別のクリップへの切り替えになり、最初から流れる。
	// DashR の終わりと DashL の始まりは同じ姿勢なので、連打すると走りのようにつながる。
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

	// 進み切っていて予約があれば、そのときの入力の方向へ次のダッシュを始める。
	if (IsFinished() && m_isNextDashQueued) {
		m_isNextDashQueued = false;
		Begin(context, input);
	}

	Vector3 velocity = Vector3::Zero;
	if (!IsFinished() && deltaTime > FLT_EPSILON) {
		// 前のフレームまでに進んだ割合と、今フレームの終わりの割合の差だけ進む。
		// 最後は割合が1で止まるので、フレームレートによらず dashDistance ちょうど進む。
		const float before = CalcDashProgress(m_elapsed / m_duration);
		m_elapsed = min(m_elapsed + deltaTime, m_duration);
		const float after = CalcDashProgress(m_elapsed / m_duration);
		const float distance = (after - before) * context.settings.dashDistance;
		velocity = m_direction * (distance / deltaTime);
	}
	context.mover->Move(velocity, deltaTime);
}

void PlayerDashState::Exit(PlayerStateContext& context)
{
	// アニメーションの速さは、次のステートの Enter で戻す。
	// ここで戻すと、凍結で止めたアニメーションが動き出してしまう。
	m_elapsed = m_duration;
	m_isNextDashQueued = false;
}
