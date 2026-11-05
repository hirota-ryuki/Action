#include "stdafx.h"
#include "StatusEffects.h"

namespace
{
	// 凍結中の色。乗算で赤を落として青を上げ、さらに水色を足して暗い部分（髪・服）も水色にする。
	const Vector4 kFrozenMulColor = Vector4(0.30f, 0.78f, 1.45f, 1.0f);
	const Vector4 kFrozenAddColor = Vector4(0.06f, 0.22f, 0.34f, 0.0f);
	const Vector4 kNormalMulColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	const Vector4 kNormalAddColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
	constexpr float kUnfreezeFadeSeconds = 0.4f;  // 凍結が解ける直前、この秒数をかけて元の色へ戻す。

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
	if (!IsFrozen()) {
		return;
	}
	m_freezeTimer -= deltaTime;
	if (m_freezeTimer <= 0.0f) {
		m_freezeTimer = 0.0f;
		if (m_modelRender != nullptr) {
			m_modelRender->SetAnimationSpeed(1.0f);
		}
		SetFrozenLook(0.0f);
		return;
	}
	// 解ける直前は水色から元の色へなめらかに戻す。
	if (m_freezeTimer < kUnfreezeFadeSeconds) {
		SetFrozenLook(m_freezeTimer / kUnfreezeFadeSeconds);  // 1→0
	}
}

void StatusEffects::Freeze(float seconds)
{
	if (seconds <= m_freezeTimer) {
		return;
	}
	if (!IsFrozen() && m_modelRender != nullptr) {
		// 凍った瞬間のポーズで固める。
		m_modelRender->SetAnimationSpeed(0.0f);
	}
	SetFrozenLook(1.0f);
	m_freezeTimer = seconds;
}

void StatusEffects::SetFrozenLook(float frozenRate)
{
	if (m_modelRender == nullptr) {
		return;
	}
	m_modelRender->SetMulColor(LerpColor(frozenRate, kNormalMulColor, kFrozenMulColor));
	m_modelRender->SetAddColor(LerpColor(frozenRate, kNormalAddColor, kFrozenAddColor));
}
