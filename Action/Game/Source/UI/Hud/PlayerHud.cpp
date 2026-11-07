#include "stdafx.h"
#include "PlayerHud.h"

#include "Player.h"
#include "UISprite.h"

namespace
{
	// 画面の左端・上端から内側へ空ける幅。上は FPS の表示（左上）と重ならないように広めに空ける。
	constexpr float kMarginLeft = 60.0f;
	constexpr float kMarginTop = 130.0f;
	constexpr float kHpWidth = 480.0f;
	constexpr float kHpHeight = 24.0f;
	// HPゲージの左端の中央。
	const Vector2 kHpPosition = {
		-static_cast<float>(UI_SPACE_WIDTH) * 0.5f + kMarginLeft,
		static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - kMarginTop
	};

	// 状態異常のアイコン（画像ができるまでは色付きの四角）。HPゲージの右に並べる。
	constexpr float kIconSize = 32.0f;
	constexpr float kIconGap = 12.0f;
	const Vector4 kFrozenColor = { 0.4f, 0.85f, 1.0f, 1.0f };
	const Vector4 kBurningColor = { 1.0f, 0.45f, 0.1f, 1.0f };
}

void PlayerHud::Init()
{
	Gauge::Settings hp;
	hp.width = kHpWidth;
	hp.height = kHpHeight;
	m_hp.Init(hp);
	m_hp.SetPosition(kHpPosition);

	const float iconY = kHpPosition.y;
	const float frozenX = kHpPosition.x + kHpWidth + kIconGap + kIconSize * 0.5f;
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

	const StatusEffects& effects = m_player->GetStatusEffects();
	m_frozen.SetVisible(effects.IsFrozen());
	m_burning.SetVisible(effects.IsBurning());

	m_hp.Update(deltaTime);
	m_frozen.Update(deltaTime);
	m_burning.Update(deltaTime);
}

void PlayerHud::Draw(RenderContext& rc)
{
	if (m_player == nullptr) {
		return;
	}
	m_hp.Draw(rc);
	m_frozen.Draw(rc);
	m_burning.Draw(rc);
}
