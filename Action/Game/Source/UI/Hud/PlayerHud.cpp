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

	// ポーション。画面の右下に、絵と、その右下に数を出す。
	constexpr const char* POTION_ICON_PATH = "Assets/sprite/item/potion.dds";
	constexpr float POTION_ICON_SIZE = 80.0f;
	constexpr float POTION_RIGHT = 140.0f;   // 画面の右端から、絵の中央までの距離。
	constexpr float POTION_BOTTOM = 90.0f;   // 画面の下端から、絵の中央までの高さ。
	const Vector2 POTION_POSITION = {
		static_cast<float>(UI_SPACE_WIDTH) * 0.5f - POTION_RIGHT,
		-static_cast<float>(UI_SPACE_HEIGHT) * 0.5f + POTION_BOTTOM };
	const Vector2 POTION_COUNT_OFFSET = { 34.0f, -26.0f };  // 絵の中央から、数の左端・中央まで。
	constexpr float POTION_COUNT_SCALE = 0.6f;
	const Vector4 POTION_EMPTY_COLOR = { 0.35f, 0.35f, 0.35f, 0.8f };  // 0 個のとき。
	const Vector4 POTION_EMPTY_TEXT_COLOR = { 0.6f, 0.6f, 0.6f, 0.9f };
	constexpr float POTION_POP_SECONDS = 0.2f;  // 拾ったときに大きくして戻すまでの秒数。
	constexpr float POTION_POP_SCALE = 1.3f;    // 拾った瞬間の大きさ。
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

	m_potionIcon.Init(POTION_ICON_PATH, POTION_ICON_SIZE, POTION_ICON_SIZE);
	m_potionIcon.SetPosition(POTION_POSITION);
	m_potionCount.SetPosition({ POTION_POSITION.x + POTION_COUNT_OFFSET.x, POTION_POSITION.y + POTION_COUNT_OFFSET.y });
	m_potionCount.SetAlign(Text::Align::Left);
	m_potionCount.SetScale(POTION_COUNT_SCALE);
	m_potionCount.SetShadow(NUMBER_SHADOW_OFFSET, NUMBER_SHADOW_COLOR);
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

	UpdatePotion(status.GetPotionCount(), deltaTime);

	m_hp.Update(deltaTime);
	m_hpNumber.Update(deltaTime);
	m_frozen.Update(deltaTime);
	m_burning.Update(deltaTime);
	m_potionIcon.Update(deltaTime);
	m_potionCount.Update(deltaTime);
}

void PlayerHud::UpdatePotion(int count, float deltaTime)
{
	// 増えたら（拾ったら）一瞬大きくして、POTION_POP_SECONDS かけて戻す。最初のフレームは比べない。
	if (m_lastPotionCount >= 0 && count > m_lastPotionCount) {
		m_potionPopSeconds = POTION_POP_SECONDS;
	}
	m_lastPotionCount = count;
	m_potionPopSeconds = max(0.0f, m_potionPopSeconds - deltaTime);
	const float pop = m_potionPopSeconds / POTION_POP_SECONDS;
	m_potionIcon.SetScale(1.0f + (POTION_POP_SCALE - 1.0f) * pop);

	wchar_t text[32];
	swprintf_s(text, L"x%d", count);
	m_potionCount.SetText(text);
	const bool isEmpty = count <= 0;
	m_potionIcon.SetColor(isEmpty ? POTION_EMPTY_COLOR : g_vec4White);
	m_potionCount.SetColor(isEmpty ? POTION_EMPTY_TEXT_COLOR : NUMBER_COLOR);
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
	m_potionIcon.Draw(rc);
	m_potionCount.Draw(rc);
}
