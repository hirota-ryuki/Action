#pragma once

#include "PlayerHud.h"
#include "PickupHud.h"
#include "BossHud.h"
#include "DebugHud.h"

class Player;
class Character;

// 画面の UI をまとめて持ち、毎フレーム更新して描く。ゲームに1つだけ。
// UI はゲームを読むだけで、ゲーム側は UI のことを知らない。UIManager に話しかけるのは Game だけにする
// （シングルトンにしない。キャラクターが自分で UI を呼べると、ゲームが UI を知ることになるため）。
// キャラクターが動いたあとの値を出すため、NewGO の優先度はキャラクター（1）・カメラ（2）より後の 3 にする。
class UIManager : public IGameObject
{
public:
	bool Start() override;
	void Update() override;
	void Render(RenderContext& rc) override;

	void SetPlayer(const Player* player)
	{
		m_playerHud.SetTarget(player);
		m_pickupHud.SetTarget(player);
	}
	// ボスの HPバーを出す。ボスを DeleteGO する前に、必ず HideBoss() を呼ぶ。
	void ShowBoss(const Character* boss, const wchar_t* name)
	{
		m_bossHud.Show(boss, name);
	}
	void HideBoss()
	{
		m_bossHud.Hide();
	}

private:
	PlayerHud m_playerHud;
	PickupHud m_pickupHud;  // 拾えるアイテムの一覧。
	BossHud m_bossHud;
	DebugHud m_debugHud;  // 開発用の表示（FPS）。いちばん上に描く。
};
