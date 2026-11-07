#include "stdafx.h"
#include "PlayerDashState.h"
#include "PlayerGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"

namespace
{
	// 移動入力のデッドゾーン閾値（二乗値）。これ以下なら正面へダッシュする。
	constexpr float MOVE_DEAD_ZONE_SQ = 0.01f;

	// 進んだ割合（0〜1）。時間の割合 t に対して ease-out（最初が速く、だんだん遅くなる）。
	// endRatio は、終わりの速さ ÷ 平均の速さ（0〜1）。速さは平均の (2 - endRatio) 倍から endRatio 倍へ、まっすぐ下がる。
	// endRatio が 0 なら、蹴った瞬間は平均の2倍で、終わりで止まる。
	float CalcDashProgress(float t, float endRatio)
	{
		return endRatio * t + (1.0f - endRatio) * (2.0f * t - t * t);
	}

	// 走りのクリップの位置（秒）を、0〜1周期の長さの中に収める。長さが分からなければ0より前にしない。
	float WrapRunAnimTime(float time, float length)
	{
		if (length <= FLT_EPSILON) {
			return max(0.0f, time);
		}
		time = fmodf(time, length);
		return time < 0.0f ? time + length : time;
	}
}

void PlayerDashState::Enter(PlayerStateContext& context, const CharacterInput& input)
{
	m_isNextDashQueued = false;
	m_isFirstFrame = true;
	context.keepRunAfterDash = context.settings.dashKeepRun;
	m_crouchRemaining = 0.0f;
	Begin(context, input);
	// 地面から出すときは、蹴る前に一度しゃがみ込む。
	const PlayerStateSettings& s = context.settings;
	if (s.dashCrouchSeconds > 0.0f && m_duration > 0.0f && context.mover->IsOnGround()) {
		m_crouchRemaining = s.dashCrouchSeconds;
		PlayKick(context, s.dashCrouchBlendSeconds, 0.0f);
		return;
	}
	// しゃがみを使わない設定のとき（dashCrouchSeconds が 0）は、しゃがむところを飛ばして蹴るところから流す。
	PlayKick(context, s.dashAnimBlendSeconds, s.dashKickStartTime);
}

void PlayerDashState::Begin(PlayerStateContext& context, const CharacterInput& input)
{
	m_direction = input.moveDirection;
	m_direction.y = 0.0f;
	if (m_direction.LengthSq() <= MOVE_DEAD_ZONE_SQ) {
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
}

void PlayerDashState::PlayKick(PlayerStateContext& context, float blendSeconds, float startTime)
{
	// ダッシュの通しのクリップを1回流す（ループしないので、終わると着地して走り出すポーズで止まる）。
	// 連打で続けて出すとき（dashCanChain）は同じクリップを流し直すので、補間されずに startTime のポーズへ飛ぶ。
	context.animator->PlayFrom(enPlayerAnimClip_Dash, blendSeconds, startTime, context.settings.dashAnimSpeed);
}

PlayerStateId PlayerDashState::CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const
{
	// 着地まで終わるまでは、何を押してもダッシュのまま（入力を受け付けない）。
	// 連打で続けて出せるときは、次のダッシュを予約している間もダッシュのまま。
	if (!IsFinished()) {
		return PlayerStateId::Dash;
	}
	// 続けて出すのも足場があるときだけ（ダッシュで足場から出たら、空中では出さない）。
	if (context.settings.dashCanChain && (m_isNextDashQueued || input.dash.isTrigger) && context.mover->IsOnGround()) {
		return PlayerStateId::Dash;
	}
	const PlayerStateId next = PlayerGroundState::SelectByInput(context, input);
	// 移動の入力がなければ、すぐ立ち止まらずに、勢いで滑りながら踏ん張って止まる（走りを離したときと同じ）。
	if (next == PlayerStateId::Idle && context.settings.dashEndWithStop && context.settings.stopSeconds > 0.0f) {
		return PlayerStateId::Stop;
	}
	return next;
}

void PlayerDashState::Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime)
{
	// 入ったフレームの押下は、このダッシュを始めた押下なので数えない。
	if (context.settings.dashCanChain && input.dash.isTrigger && !m_isFirstFrame && context.settings.dashDistance > 0.0f) {
		m_isNextDashQueued = true;
	}
	m_isFirstFrame = false;
	// ダッシュの間は、移動の入力がなくても（正面へダッシュしたときも）走り続ける印を立てておく。
	// 終わったとき移動の入力がなければ、PlayerStateMachine が下ろす（そのときは Stop へ移る）。
	context.keepRunAfterDash = context.settings.dashKeepRun;

	// 飛び終わっていて予約があれば（着地中でも）、そのときの入力の方向へ次のダッシュを始める。空中では始めない。
	if (IsKickFinished() && m_isNextDashQueued && context.mover->IsOnGround()) {
		m_isNextDashQueued = false;
		Begin(context, input);
		// 続けて出すときは、しゃがむところを飛ばして蹴るところから流す。
		PlayKick(context, context.settings.dashAnimBlendSeconds, context.settings.dashKickStartTime);
	}

	const PlayerStateSettings& s = context.settings;
	Vector3 velocity = Vector3::Zero;
	if (IsCrouching()) {
		// しゃがむ間は、だんだん遅くなって止まる。しゃがみ終わったら蹴り出す（アニメーションは通しで流れ続ける）。
		const float rate = s.dashCrouchSeconds > 0.0f ? m_crouchRemaining / s.dashCrouchSeconds : 0.0f;
		velocity = m_direction * (s.dashCrouchSpeed * rate);
		m_crouchRemaining -= deltaTime;
		if (!IsCrouching()) {
			m_crouchRemaining = 0.0f;
		}
	}
	else if (!IsKickFinished() && deltaTime > FLT_EPSILON) {
		// 前のフレームまでに進んだ割合と、今フレームの終わりの割合の差だけ進む。
		// 最後は割合が1で止まるので、フレームレートによらず dashDistance ちょうど進む。
		const float endRatio = s.dashSpeed > 0.0f ? max(0.0f, min(1.0f, s.dashEndSpeed / s.dashSpeed)) : 0.0f;
		const float before = CalcDashProgress(m_elapsed / m_duration, endRatio);
		m_elapsed = min(m_elapsed + deltaTime, m_duration);
		const float after = CalcDashProgress(m_elapsed / m_duration, endRatio);
		const float distance = (after - before) * s.dashDistance;
		velocity = m_direction * (distance / deltaTime);
		if (IsKickFinished() && !m_isNextDashQueued) {
			BeginLanding(context, input);
		}
	}
	else if (m_isLanding && s.dashLandSeconds > 0.0f) {
		// 着地の間は、終わりの速さのまま進む。
		velocity = m_direction * s.dashEndSpeed;
		m_landElapsed += deltaTime;
		m_runAnimTime += deltaTime * s.dashAnimSpeed;
		if (m_landElapsed >= s.dashLandSeconds) {
			m_isLanding = false;
		}
	}
	context.mover->Move(velocity, deltaTime);
}

void PlayerDashState::BeginLanding(PlayerStateContext& context, const CharacterInput& input)
{
	// アニメーションは通しのクリップのまま流れ続け、着地のところへ進む。ここでは進み方だけを変える。
	m_isLanding = true;
	m_landElapsed = 0.0f;
	m_isRunBlended = false;
	const PlayerStateSettings& s = context.settings;
	// 着地の最後と前後の足が同じになる、走りの位置。
	const float runEnd = s.dashToRunStartTime;
	m_runAnimTime = runEnd;
	// 走るボタンを押したままなら、着地の間に走りのアニメーションへ先に補間しておく（見た目だけ。動きは着地のまま）。
	// 着地を流し切ってから補間すると、1回きりのダッシュのクリップが最後のポーズで止まり、
	// 止まったポーズから走りへ補間することになって、一瞬ピタッと止まって見える。
	// 着地の間はダッシュと走りの両方が動き、補間が終わるのと同時にダッシュのクリップも終わる。
	if (!s.dashLandIntoRun || s.dashLandSeconds <= 0.0f) {
		return;
	}
	if (PlayerGroundState::SelectByInput(context, input) != PlayerStateId::Run) {
		return;
	}
	// 補間が終わったとき（着地の最後）に runEnd に来るよう、着地の長さの分だけ前から流す。
	m_runAnimTime = WrapRunAnimTime(runEnd - s.dashLandSeconds * s.dashAnimSpeed, s.runAnimSeconds);
	context.animator->PlayFrom(enPlayerAnimClip_Run, s.dashLandSeconds, m_runAnimTime, s.dashAnimSpeed);
	m_isRunBlended = true;
}

void PlayerDashState::Exit(PlayerStateContext& context)
{
	// アニメーションの速さは、次のステートの Enter で戻す。
	// ここで戻すと、凍結で止めたアニメーションが動き出してしまう。
	// 次のアニメーションへは長めに補間して、飛び出したポーズが少し残るようにする。
	const PlayerStateSettings& s = context.settings;
	context.animator->SetNextBlend(s.dashExitBlendSeconds);
	// 走り・歩きは、ダッシュの最後と前後の足が同じところから始める。
	// 着地の間に走りへ補間してあれば、走りはもう流れているので、そのまま続ける（始める位置を指定すると巻き戻る）。
	if (!m_isRunBlended) {
		context.animator->SetNextStartTime(enPlayerAnimClip_Run, s.dashToRunStartTime);
	}
	// 走り・止まる（Stop が踏ん張る足を決める）のために、走りの位置を渡す。
	context.animator->SetRunTime(m_runAnimTime);
	context.animator->SetNextStartTime(enPlayerAnimClip_Walk, s.dashToWalkStartTime);
	m_elapsed = m_duration;
	m_isLanding = false;
	m_crouchRemaining = 0.0f;
	m_isNextDashQueued = false;
	m_isRunBlended = false;
}
