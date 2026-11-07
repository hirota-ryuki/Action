#include "stdafx.h"
#include "StatusEffects.h"

namespace
{
	// 凍結中の色。乗算で赤を落として青を上げ、さらに水色を足して暗い部分（髪・服）も水色にする。
	const Vector4 FROZEN_MUL_COLOR = Vector4(0.30f, 0.78f, 1.45f, 1.0f);
	const Vector4 FROZEN_ADD_COLOR = Vector4(0.06f, 0.22f, 0.34f, 0.0f);
	// 燃焼中の色。赤を上げて青を落とし、橙色を足す。
	const Vector4 BURNING_MUL_COLOR = Vector4(1.45f, 0.62f, 0.38f, 1.0f);
	const Vector4 BURNING_ADD_COLOR = Vector4(0.30f, 0.08f, 0.0f, 0.0f);
	const Vector4 NORMAL_MUL_COLOR = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	const Vector4 NORMAL_ADD_COLOR = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
	constexpr float UNFREEZE_FADE_SECONDS = 0.4f;  // 凍結が解ける直前、この秒数をかけて元の色へ戻す。
	constexpr float BURN_FADE_SECONDS = 0.4f;      // 火が消える直前、この秒数をかけて元の色へ戻す。
	constexpr float BURN_FLICKER_SPEED = 14.0f;    // 炎の揺らめきの速さ（ラジアン/秒）。
	constexpr float BURN_FLICKER_DEPTH = 0.3f;     // 揺らめきで色の強さが下がる割合。

	// a(t=0) から b(t=1) へ線形補間する。
	Vector4 LerpColor(float t, const Vector4& a, const Vector4& b)
	{
		return Vector4(
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
			a.w + (b.w - a.w) * t);
	}
}

void StatusEffects::Init(ModelRender* modelRender)
{
	m_modelRender = modelRender;
}

void StatusEffects::Update(float deltaTime)
{
	if (IsFrozen()) {
		m_freezeTimer -= deltaTime;
		if (m_freezeTimer <= 0.0f) {
			Unfreeze();
			return;
		}
		// 解ける直前は水色から元の色へなめらかに戻す。
		if (m_freezeTimer < UNFREEZE_FADE_SECONDS) {
			SetLook(m_freezeTimer / UNFREEZE_FADE_SECONDS, FROZEN_MUL_COLOR, FROZEN_ADD_COLOR);  // 1→0
		}
		return;
	}

	if (IsBurning()) {
		m_burnTimer -= deltaTime;
		m_burnElapsed += deltaTime;
		if (m_burnTimer <= 0.0f) {
			m_burnTimer = 0.0f;
			SetLook(0.0f, BURNING_MUL_COLOR, BURNING_ADD_COLOR);
			return;
		}
		// 色の強さを揺らして炎が当たっているように見せ、消える直前は元の色へなめらかに戻す。
		const float flicker = 1.0f - BURN_FLICKER_DEPTH * (0.5f + 0.5f * sinf(m_burnElapsed * BURN_FLICKER_SPEED));
		const float fade = m_burnTimer < BURN_FADE_SECONDS ? m_burnTimer / BURN_FADE_SECONDS : 1.0f;
		SetLook(flicker * fade, BURNING_MUL_COLOR, BURNING_ADD_COLOR);
	}
}

void StatusEffects::Freeze(float seconds)
{
	if (seconds <= m_freezeTimer) {
		return;
	}
	// 凍らせると火は消える。
	m_burnTimer = 0.0f;
	if (!IsFrozen() && m_modelRender != nullptr) {
		// 凍った瞬間のポーズで固める。
		m_modelRender->SetAnimationSpeed(0.0f);
	}
	SetLook(1.0f, FROZEN_MUL_COLOR, FROZEN_ADD_COLOR);
	m_freezeTimer = seconds;
}

void StatusEffects::Burn(float seconds)
{
	// 燃やすと解凍される。
	if (IsFrozen()) {
		Unfreeze();
	}
	if (!IsBurning()) {
		m_burnElapsed = 0.0f;
		SetLook(1.0f, BURNING_MUL_COLOR, BURNING_ADD_COLOR);
	}
	if (seconds > m_burnTimer) {
		m_burnTimer = seconds;
	}
}

void StatusEffects::Unfreeze()
{
	m_freezeTimer = 0.0f;
	if (m_modelRender != nullptr) {
		m_modelRender->SetAnimationSpeed(1.0f);
	}
	SetLook(0.0f, FROZEN_MUL_COLOR, FROZEN_ADD_COLOR);
}

void StatusEffects::SetLook(float rate, const Vector4& mulColor, const Vector4& addColor)
{
	if (m_modelRender == nullptr) {
		return;
	}
	m_modelRender->SetMulColor(LerpColor(rate, NORMAL_MUL_COLOR, mulColor));
	m_modelRender->SetAddColor(LerpColor(rate, NORMAL_ADD_COLOR, addColor));
}
