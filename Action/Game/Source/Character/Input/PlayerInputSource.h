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
//   サブの発射  マウス右ボタン / 右トリガー / 8
class PlayerInputSource : public IInputSource
{
public:
	explicit PlayerInputSource(int padNo = 0);
	void Update(const InputSourceContext& context, CharacterInput& input) override;

private:
	// ゲームのウィンドウが前面にあるか。マウスは他のウィンドウを操作中に反応させない。
	static bool IsGameWindowActive();
	// マウスのボタンをゲームの操作に使ってよいか（ウィンドウが前にあり、開発用の画面を出していない）。
	static bool CanUseMouse();
	static bool IsKeyPressed(int virtualKey);

	int m_padNo = 0;
};
