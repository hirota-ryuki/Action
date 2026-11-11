#pragma once

#include "Gauge.h"
#include "Icon.h"
#include "Text.h"

class Player;

// Player 用の HUD。Player の値を読んで、自分の部品に入れる。
// 画面下の中央に、細い緑の HPゲージと、その下に「今のHP / 最大HP」の数字を出す。状態異常のアイコンはゲージの右。
// 右下に、持っているポーションの絵と数（x2）を出す。
// UIElement は継承しない（自分では描かず、部品を持つだけ）。UIManager が持つ。
class PlayerHud
{
public:
	void Init();
	// 表示する Player。読むだけ（const）。nullptr なら何も描かない。
	void SetTarget(const Player* player)
	{
		m_player = player;
	}
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	// 右下のポーションの絵と数。
	void UpdatePotion(int count, float deltaTime);

	const Player* m_player = nullptr;
	Gauge m_hp;
	Text m_hpNumber;  // 「今のHP / 最大HP」。
	Icon m_frozen;    // 凍結中だけ出す。
	Icon m_burning;   // 燃焼中だけ出す。
	Icon m_potionIcon;   // 右下のポーションの絵。0 個のときは暗くする。
	Text m_potionCount;  // ポーションの数（x2）。
	int m_lastPotionCount = -1;  // 前のフレームの数。増えたら絵を一瞬大きくする。
	float m_potionPopSeconds = 0.0f;  // 大きくしてからの残り時間。
};
