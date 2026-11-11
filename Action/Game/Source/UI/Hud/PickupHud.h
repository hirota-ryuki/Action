#pragma once

#include "Icon.h"
#include "Text.h"

class Player;

// 拾えるアイテムの一覧。プレイヤーの近くにアイテムがあるときだけ、画面の真ん中の右に出す。
// 1行は「アイテムの絵＋名前」。近い順に並べ、1行目（F で拾うもの）の左に F の印を付けて明るくする。
// Player の ItemPicker を読むだけで、拾うのはプレイヤーのステート。UIManager が持つ。
class PickupHud
{
public:
	// 一度に出す行の数。これより多いときは、近い方から MAX_ROWS 個だけ出す。
	static constexpr int MAX_ROWS = 5;

	void Init();
	void SetTarget(const Player* player)
	{
		m_player = player;
	}
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	struct Row
	{
		Icon back;              // 行の後ろの帯。
		Icon icon;              // アイテムの絵。
		Text name;              // アイテムの名前。
		std::string iconPath;   // 今 icon に読んである絵。違う絵になったときだけ読み直す。
	};

	const Player* m_player = nullptr;
	Row m_rows[MAX_ROWS];
	int m_rowCount = 0;  // 今出している行の数。
	Icon m_keyBack;      // F の印の四角。
	Text m_keyText;      // F の文字。
};
