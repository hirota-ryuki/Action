#include "stdafx.h"
#include "BossHud.h"

#include "Character.h"
#include "UISprite.h"

namespace
{
	// HPゲージ。画面上の中央に、横長に置く（Player のゲージは画面下なので重ならない）。
	constexpr float kHpWidth = 720.0f;
	constexpr float kHpHeight = 12.0f;
	constexpr float kHpBorder = 2.0f;
	constexpr float kHpTop = 100.0f;  // 画面の上端から、ゲージの中央までの距離。
	// HPゲージの左端の中央。
	const Vector2 kHpPosition = { -kHpWidth * 0.5f, static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - kHpTop };
	const Vector4 kHpColor = { 0.9f, 0.28f, 0.22f, 1.0f };      // 赤。
	const Vector4 kHpBackColor = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 kHpBorderColor = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 kHpTrailColor = { 1.0f, 0.92f, 0.8f, 0.8f };  // 少し暖かい白。

	// 名前は HPゲージの上に小さく。
	constexpr float kNameScale = 0.55f;
	constexpr float kNameGap = 24.0f;  // ゲージの中央から、名前の中央までの距離。
	const Vector2 kNamePosition = { 0.0f, kHpPosition.y + kNameGap };
	const Vector4 kNameShadowColor = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float kNameShadowOffset = 1.5f;

	constexpr float kTrailDelay = 0.5f;  // 減ってから、遅れて減るバーが縮み始めるまでの秒数。
	constexpr float kTrailSpeed = 0.5f;  // 1秒にゲージ全体の 50% 縮む。
	constexpr float kFadeSeconds = 0.3f; // フェードイン・アウトにかける秒数。
}

void BossHud::Init()
{
	Gauge::Settings hp;
	hp.width = kHpWidth;
	hp.height = kHpHeight;
	hp.borderSize = kHpBorder;
	hp.borderColor = kHpBorderColor;
	hp.backColor = kHpBackColor;
	hp.barColor = kHpColor;
	hp.barImagePath = UISprite::kGaugePath;
	hp.useTrail = true;
	hp.trailColor = kHpTrailColor;
	hp.trailDelay = kTrailDelay;
	hp.trailSpeed = kTrailSpeed;
	m_hp.Init(hp);
	m_hp.SetPosition(kHpPosition);

	m_name.SetPosition(kNamePosition);
	m_name.SetScale(kNameScale);
	m_name.SetShadow(kNameShadowOffset, kNameShadowColor);

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
