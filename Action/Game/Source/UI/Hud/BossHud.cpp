#include "stdafx.h"
#include "BossHud.h"

#include "Character.h"

namespace
{
	constexpr float kHpWidth = 800.0f;
	constexpr float kHpHeight = 20.0f;
	// HPゲージの左端の中央。画面下の中央に置く。
	const Vector2 kHpPosition = { -kHpWidth * 0.5f, -static_cast<float>(UI_SPACE_HEIGHT) * 0.5f + 90.0f };
	// 名前は HPゲージの上（文字の中央の位置。下の端がゲージにかからない高さ）。
	const Vector2 kNamePosition = { 0.0f, kHpPosition.y + 55.0f };

	constexpr float kTrailDelay = 0.5f;  // 減ってから、白いバーが縮み始めるまでの秒数。
	constexpr float kTrailSpeed = 0.5f;  // 1秒にゲージ全体の 50% 縮む。
	constexpr float kFadeSeconds = 0.3f; // フェードイン・アウトにかける秒数。
}

void BossHud::Init()
{
	Gauge::Settings hp;
	hp.width = kHpWidth;
	hp.height = kHpHeight;
	hp.useTrail = true;
	hp.trailDelay = kTrailDelay;
	hp.trailSpeed = kTrailSpeed;
	m_hp.Init(hp);
	m_hp.SetPosition(kHpPosition);

	m_name.SetPosition(kNamePosition);

	SetAlpha(0.0f);
}

void BossHud::Show(const Character* boss, const wchar_t* name)
{
	m_boss = boss;
	m_name.SetText(name);
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}
	// 戦い始めに白いバーが減っていく演出にならないよう、今のHPにそろえる。
	m_hp.ResetTrail();
	m_targetAlpha = 1.0f;
}

void BossHud::Hide()
{
	m_boss = nullptr;
	m_targetAlpha = 0.0f;
}

void BossHud::Update(float deltaTime)
{
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}

	// 目標の透明度へ、一定の速さで近づける。
	const float step = deltaTime / kFadeSeconds;
	if (m_alpha < m_targetAlpha) {
		SetAlpha(min(m_targetAlpha, m_alpha + step));
	}
	else if (m_alpha > m_targetAlpha) {
		SetAlpha(max(m_targetAlpha, m_alpha - step));
	}

	m_name.Update(deltaTime);
	m_hp.Update(deltaTime);
}

void BossHud::Draw(RenderContext& rc)
{
	m_name.Draw(rc);
	m_hp.Draw(rc);
}

void BossHud::SetAlpha(float alpha)
{
	m_alpha = alpha;
	// 名前とゲージに同じ透明度を入れる。0 のときは部品の側で描かない。
	m_name.SetAlpha(alpha);
	m_hp.SetAlpha(alpha);
}
