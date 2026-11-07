#pragma once

#include "CharacterInput.h"

// パッド・キーボード・マウスの入力を CharacterInput に変換する入力元。
// スティックの向きはカメラ基準からワールドの向きに変換して渡す。
//
// 割り当て:
//   移動        左スティック / WASD
//   走る        左Shift
//   ジャンプ    Aボタン / J
//   ダッシュ    Xボタン / L
//   発射        マウス左ボタン / Yボタン / I
//   魔法切替    LB・RB / Q・E
class PlayerInputSource : public IInputSource
{
public:
	explicit PlayerInputSource(int padNo = 0);
	void Update(const InputSourceContext& context, CharacterInput& input) override;

private:
	// ゲームのウィンドウが前面にあるか。マウスは他のウィンドウを操作中に反応させない。
	static bool IsGameWindowActive();
	static bool IsKeyPressed(int virtualKey);

	int m_padNo = 0;
	ButtonState m_prevMagicKey;  // Qキー。
	ButtonState m_nextMagicKey;  // Eキー。
};
