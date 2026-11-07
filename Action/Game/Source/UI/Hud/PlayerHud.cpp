#include "stdafx.h"
#include "PlayerHud.h"

#include "Player.h"
#include "UISprite.h"

namespace
{
	// HPゲージ。画面下の中央に、細く置く。
	constexpr float kHpWidth = 360.0f;
	constexpr float kHpHeight = 10.0f;
	constexpr float kHpBorder = 2.0f;
	constexpr float kHpBottom = 80.0f;  // 画面の下端から、ゲージの中央までの高さ。
	// HPゲージの左端の中央。
	const Vector2 kHpPosition = { -kHpWidth * 0.5f, -static_cast<float>(UI_SPACE_HEIGHT) * 0.5f + kHpBottom };
	const Vector4 kHpColor = { 0.55f, 0.88f, 0.35f, 1.0f };      // 明るい緑。
	const Vector4 kHpBackColor = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 kHpBorderColor = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 kHpTrailColor = { 1.0f, 1.0f, 1.0f, 0.75f };

	// HP の数字。ゲージの下に小さく。
	constexpr float kNumberScale = 0.42f;
	constexpr float kNumberGap = 18.0f;  // ゲージの中央から、数字の中央までの距離。
	const Vector4 kNumberColor = { 1.0f, 1.0f, 1.0f, 0.95f };
	const Vector4 kNumberShadowColor = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float kNumberShadowOffset = 1.5f;

	// 状態異常のアイコン（画像ができるまでは色付きの四角）。ゲージの右に並べる。
	constexpr float kIconSize = 18.0f;
	constexpr float kIconGap = 10.0f;
	const Vector4 kFrozenColor = { 0.4f, 0.85f, 1.0f, 1.0f };
	const Vector4 kBurningColor = { 1.0f, 0.45f, 0.1f, 1.0f };
}

void PlayerHud::Init()
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
	m_hp.Init(hp);
	m_hp.SetPosition(kHpPosition);

	m_hpNumber.SetPosition({ 0.0f, kHpPosition.y - kNumberGap });
	m_hpNumber.SetScale(kNumberScale);
	m_hpNumber.SetColor(kNumberColor);
	m_hpNumber.SetShadow(kNumberShadowOffset, kNumberShadowColor);

	const float iconY = kHpPosition.y;
	const float frozenX = kHpPosition.x + kHpWidth + kHpBorder + kIconGap + kIconSize * 0.5f;
	const float burningX = frozenX + kIconSize + kIconGap;

	m_frozen.Init(UISprite::kWhitePath, kIconSize, kIconSize);
	m_frozen.SetColor(kFrozenColor);
	m_frozen.SetPosition({ frozenX, iconY });

	m_burning.Init(UISprite::kWhitePath, kIconSize, kIconSize);
	m_burning.SetColor(kBurningColor);
	m_burning.SetPosition({ burningX, iconY });
}

void PlayerHud::Update(float deltaTime)
{
	if (m_player == nullptr) {
		return;
	}
	const PlayerStatus& status = m_player->GetStatus();
	m_hp.SetRate(status.GetHpRate());

	// 小数は切り上げる（0.3 残っているのに 0 と出さないように）。
	wchar_t number[64];
	swprintf_s(number, L"%d / %d",
		static_cast<int>(ceilf(status.GetHp())),
		static_cast<int>(ceilf(status.GetMaxHp())));
	m_hpNumber.SetText(number);

	const StatusEffects& effects = m_player->GetStatusEffects();
	m_frozen.SetVisible(effects.IsFrozen());
	m_burning.SetVisible(effects.IsBurning());

	m_hp.Update(deltaTime);
	m_hpNumber.Update(deltaTime);
	m_frozen.Update(deltaTime);
	m_burning.Update(deltaTime);
}

void PlayerHud::Draw(RenderContext& rc)
{
	if (m_player == nullptr) {
		return;
	}
	m_hp.Draw(rc);
	m_hpNumber.Draw(rc);
	m_frozen.Draw(rc);
	m_burning.Draw(rc);
}
