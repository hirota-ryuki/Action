#include "stdafx.h"
#include "Gauge.h"

#include "UISprite.h"

namespace
{
	// 左端の中央を基準にする（ピボットの x は 0 が左端、0.5 が中央）。
	const Vector2 kLeftCenterPivot = { 0.0f, 0.5f };
}

void Gauge::Init(const Settings& settings)
{
	m_settings = settings;
	const float border = m_settings.borderSize;
	m_border.Init(UISprite::kWhitePath, m_settings.width + border * 2.0f, m_settings.height + border * 2.0f);
	m_back.Init(UISprite::kWhitePath, m_settings.width, m_settings.height);
	m_trail.Init(m_settings.barImagePath, m_settings.width, m_settings.height);
	m_bar.Init(m_settings.barImagePath, m_settings.width, m_settings.height);
	m_border.SetPivot(kLeftCenterPivot);
	m_back.SetPivot(kLeftCenterPivot);
	m_trail.SetPivot(kLeftCenterPivot);
	m_bar.SetPivot(kLeftCenterPivot);
}

void Gauge::SetRate(float rate)
{
	rate = min(1.0f, max(0.0f, rate));
	if (rate < m_rate) {
		// 減った。白いバーは今の位置に残し、少し待ってから縮める。
		m_trailWait = m_settings.trailDelay;
	}
	m_rate = rate;
	if (m_trailRate < m_rate) {
		m_trailRate = m_rate;
	}
}

void Gauge::ResetTrail()
{
	m_trailRate = m_rate;
	m_trailWait = 0.0f;
}

void Gauge::Update(float deltaTime)
{
	if (m_settings.useTrail && m_trailRate > m_rate) {
		if (m_trailWait > 0.0f) {
			m_trailWait -= deltaTime;
		}
		else {
			m_trailRate = max(m_rate, m_trailRate - m_settings.trailSpeed * deltaTime);
		}
	}
	else {
		m_trailRate = m_rate;
	}

	// 枠は、中身より borderSize だけ左から始まり、上下左右に borderSize ずつはみ出す。
	UpdateBar(m_border, m_position.x - m_settings.borderSize, 1.0f, m_settings.borderColor);
	UpdateBar(m_back, m_position.x, 1.0f, m_settings.backColor);
	UpdateBar(m_trail, m_position.x, m_trailRate, m_settings.trailColor);
	UpdateBar(m_bar, m_position.x, m_rate, m_settings.barColor);
}

void Gauge::UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color)
{
	sprite.SetPosition(Vector3(x, m_position.y, 0.0f));
	sprite.SetScale(Vector3(rate, 1.0f, 1.0f));
	sprite.SetMulColor(ApplyAlpha(color));
	sprite.Update();
}

void Gauge::OnDraw(RenderContext& rc)
{
	// 後に描いたものが上に重なる。枠 → 空き → 遅れて減るバー → 中身の順。
	if (m_settings.borderSize > 0.0f) {
		m_border.Draw(rc);
	}
	m_back.Draw(rc);
	if (m_settings.useTrail) {
		m_trail.Draw(rc);
	}
	m_bar.Draw(rc);
}
