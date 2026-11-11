#include "stdafx.h"
#include "PickupHud.h"

#include "Player.h"
#include "Item.h"
#include "UISprite.h"

namespace
{
	// 行の並べ方。画面の中央から少し右に、上から下へ並べる。
	constexpr float ROW_LEFT = 160.0f;    // 画面の中央から、行の左端まで。
	constexpr float ROW_TOP = 60.0f;      // 画面の中央から、1行目の中央まで（上が +）。
	constexpr float ROW_WIDTH = 300.0f;
	constexpr float ROW_HEIGHT = 46.0f;
	constexpr float ROW_GAP = 6.0f;       // 行と行のすき間。
	const Vector4 ROW_COLOR = { 0.05f, 0.05f, 0.08f, 0.45f };
	const Vector4 ROW_SELECTED_COLOR = { 0.25f, 0.25f, 0.3f, 0.7f };  // 1行目（F で拾うもの）。

	// 行の中のアイテムの絵と名前。
	constexpr float ICON_SIZE = 38.0f;
	constexpr float ICON_LEFT = 8.0f;     // 行の左端から、絵の左端まで。
	constexpr float NAME_LEFT = 56.0f;    // 行の左端から、名前の左端まで。
	constexpr float NAME_SCALE = 0.48f;
	const Vector4 NAME_COLOR = { 1.0f, 1.0f, 1.0f, 0.95f };
	const Vector4 NAME_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NAME_SHADOW_OFFSET = 1.5f;

	// F の印。1行目の左に出す。
	constexpr float KEY_SIZE = 32.0f;
	constexpr float KEY_GAP = 10.0f;      // F の印の右端から、行の左端まで。
	constexpr float KEY_TEXT_SCALE = 0.48f;
	const Vector4 KEY_BACK_COLOR = { 0.95f, 0.95f, 0.95f, 0.95f };
	const Vector4 KEY_TEXT_COLOR = { 0.1f, 0.1f, 0.1f, 1.0f };

	float RowCenterY(int index)
	{
		return ROW_TOP - (ROW_HEIGHT + ROW_GAP) * static_cast<float>(index);
	}
}

void PickupHud::Init()
{
	for (int i = 0; i < MAX_ROWS; ++i) {
		Row& row = m_rows[i];
		const float y = RowCenterY(i);
		row.back.Init(UISprite::WHITE_PATH, ROW_WIDTH, ROW_HEIGHT);
		row.back.SetPosition({ ROW_LEFT + ROW_WIDTH * 0.5f, y });
		row.back.SetColor(i == 0 ? ROW_SELECTED_COLOR : ROW_COLOR);
		row.icon.SetPosition({ ROW_LEFT + ICON_LEFT + ICON_SIZE * 0.5f, y });
		row.name.SetPosition({ ROW_LEFT + NAME_LEFT, y });
		row.name.SetAlign(Text::Align::Left);
		row.name.SetScale(NAME_SCALE);
		row.name.SetColor(NAME_COLOR);
		row.name.SetShadow(NAME_SHADOW_OFFSET, NAME_SHADOW_COLOR);
	}

	const Vector2 keyPosition = { ROW_LEFT - KEY_GAP - KEY_SIZE * 0.5f, RowCenterY(0) };
	m_keyBack.Init(UISprite::WHITE_PATH, KEY_SIZE, KEY_SIZE);
	m_keyBack.SetPosition(keyPosition);
	m_keyBack.SetColor(KEY_BACK_COLOR);
	m_keyText.SetPosition(keyPosition);
	m_keyText.SetScale(KEY_TEXT_SCALE);
	m_keyText.SetColor(KEY_TEXT_COLOR);
	m_keyText.SetText(L"F");
}

void PickupHud::Update(float deltaTime)
{
	m_rowCount = 0;
	if (m_player != nullptr) {
		const std::vector<Item*>& candidates = m_player->GetItemPicker().GetCandidates();
		m_rowCount = min(static_cast<int>(candidates.size()), MAX_ROWS);
		for (int i = 0; i < m_rowCount; ++i) {
			Row& row = m_rows[i];
			const Item* item = candidates[i];
			if (row.iconPath != item->GetIconPath()) {
				row.iconPath = item->GetIconPath();
				row.icon.Init(row.iconPath.c_str(), ICON_SIZE, ICON_SIZE);
			}
			row.name.SetText(item->GetName());
		}
	}

	for (int i = 0; i < MAX_ROWS; ++i) {
		Row& row = m_rows[i];
		row.back.Update(deltaTime);
		row.name.Update(deltaTime);
		if (!row.iconPath.empty()) {
			row.icon.Update(deltaTime);
		}
	}
	m_keyBack.Update(deltaTime);
	m_keyText.Update(deltaTime);
}

void PickupHud::Draw(RenderContext& rc)
{
	if (m_rowCount <= 0) {
		return;
	}
	for (int i = 0; i < m_rowCount; ++i) {
		Row& row = m_rows[i];
		row.back.Draw(rc);
		row.icon.Draw(rc);
		row.name.Draw(rc);
	}
	m_keyBack.Draw(rc);
	m_keyText.Draw(rc);
}
