#include "stdafx.h"
#include "PlayerHud.h"

#include "Player.h"
#include "UISprite.h"

namespace
{
	// HPゲージ。画面下の中央に、細く置く。
	constexpr float HP_WIDTH = 360.0f;
	constexpr float HP_HEIGHT = 10.0f;
	constexpr float HP_BORDER = 2.0f;
	constexpr float HP_BOTTOM = 80.0f;  // 画面の下端から、ゲージの中央までの高さ。
	// HPゲージの左端の中央。
	const Vector2 HP_POSITION = { -HP_WIDTH * 0.5f, -static_cast<float>(UI_SPACE_HEIGHT) * 0.5f + HP_BOTTOM };
	const Vector4 HP_COLOR = { 0.55f, 0.88f, 0.35f, 1.0f };      // 明るい緑。
	const Vector4 HP_BACK_COLOR = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 HP_BORDER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 HP_TRAIL_COLOR = { 1.0f, 1.0f, 1.0f, 0.75f };

	// HP の数字。ゲージの下に小さく。
	constexpr float NUMBER_SCALE = 0.42f;
	constexpr float NUMBER_GAP = 18.0f;  // ゲージの中央から、数字の中央までの距離。
	const Vector4 NUMBER_COLOR = { 1.0f, 1.0f, 1.0f, 0.95f };
	const Vector4 NUMBER_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NUMBER_SHADOW_OFFSET = 1.5f;

	// 状態異常のアイコン（画像ができるまでは色付きの四角）。ゲージの右に並べる。
	constexpr float ICON_SIZE = 18.0f;
	constexpr float ICON_GAP = 10.0f;
	const Vector4 FROZEN_COLOR = { 0.4f, 0.85f, 1.0f, 1.0f };
	const Vector4 BURNING_COLOR = { 1.0f, 0.45f, 0.1f, 1.0f };

	// 一人称のレティクル（十字）。中央から RETICLE_GAP あけて、長さ RETICLE_LENGTH の線を上下左右に出す。
	constexpr float RETICLE_GAP = 6.0f;
	constexpr float RETICLE_LENGTH = 12.0f;
	constexpr float RETICLE_THICKNESS = 2.0f;
	constexpr float RETICLE_SHADOW = 1.0f;  // 明るい背景でも見えるように、黒い縁をこの太さだけはみ出させる。
	const Vector4 RETICLE_COLOR = { 1.0f, 1.0f, 1.0f, 0.9f };
	const Vector4 RETICLE_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
}

void PlayerHud::Init()
{
	Gauge::Settings hp;
	hp.width = HP_WIDTH;
	hp.height = HP_HEIGHT;
	hp.borderSize = HP_BORDER;
	hp.borderColor = HP_BORDER_COLOR;
	hp.backColor = HP_BACK_COLOR;
	hp.barColor = HP_COLOR;
	hp.barImagePath = UISprite::GAUGE_PATH;
	hp.useTrail = true;
	hp.trailColor = HP_TRAIL_COLOR;
	m_hp.Init(hp);
	m_hp.SetPosition(HP_POSITION);

	m_hpNumber.SetPosition({ 0.0f, HP_POSITION.y - NUMBER_GAP });
	m_hpNumber.SetScale(NUMBER_SCALE);
	m_hpNumber.SetColor(NUMBER_COLOR);
	m_hpNumber.SetShadow(NUMBER_SHADOW_OFFSET, NUMBER_SHADOW_COLOR);

	const float iconY = HP_POSITION.y;
	const float frozenX = HP_POSITION.x + HP_WIDTH + HP_BORDER + ICON_GAP + ICON_SIZE * 0.5f;
	const float burningX = frozenX + ICON_SIZE + ICON_GAP;

	m_frozen.Init(UISprite::WHITE_PATH, ICON_SIZE, ICON_SIZE);
	m_frozen.SetColor(FROZEN_COLOR);
	m_frozen.SetPosition({ frozenX, iconY });

	m_burning.Init(UISprite::WHITE_PATH, ICON_SIZE, ICON_SIZE);
	m_burning.SetColor(BURNING_COLOR);
	m_burning.SetPosition({ burningX, iconY });

	// 上・下・左・右の順。縦の線は幅が細く、横の線は高さが細い。
	const float offset = RETICLE_GAP + RETICLE_LENGTH * 0.5f;
	const Vector2 positions[4] = { { 0.0f, offset }, { 0.0f, -offset }, { -offset, 0.0f }, { offset, 0.0f } };
	for (int i = 0; i < 4; i++) {
		const bool isVertical = i < 2;
		const float width = isVertical ? RETICLE_THICKNESS : RETICLE_LENGTH;
		const float height = isVertical ? RETICLE_LENGTH : RETICLE_THICKNESS;
		m_reticleShadow[i].Init(UISprite::WHITE_PATH, width + RETICLE_SHADOW * 2.0f, height + RETICLE_SHADOW * 2.0f);
		m_reticleShadow[i].SetColor(RETICLE_SHADOW_COLOR);
		m_reticleShadow[i].SetPosition(positions[i]);
		m_reticle[i].Init(UISprite::WHITE_PATH, width, height);
		m_reticle[i].SetColor(RETICLE_COLOR);
		m_reticle[i].SetPosition(positions[i]);
	}
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

	// 一人称のとき（体を描いていないとき）だけ出す。
	const bool isFirstPerson = !m_player->IsRenderBody();
	for (int i = 0; i < 4; i++) {
		m_reticleShadow[i].SetVisible(isFirstPerson);
		m_reticle[i].SetVisible(isFirstPerson);
		m_reticleShadow[i].Update(deltaTime);
		m_reticle[i].Update(deltaTime);
	}
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
	for (int i = 0; i < 4; i++) {
		m_reticleShadow[i].Draw(rc);
	}
	for (int i = 0; i < 4; i++) {
		m_reticle[i].Draw(rc);
	}
}
