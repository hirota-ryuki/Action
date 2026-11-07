#pragma once

#include "Text.h"

// 開発用の表示（今は FPS だけ）。ゲームの HUD とは分けて持つ。UIElement は継承しない。UIManager が持つ。
// Debug・Preview ビルド（K2_DEBUG）のときだけ出す。Release では何も描かない。
// FPS は画面の右上に小さく、右寄せで出す。毎フレームの値は揺れて読みにくいので、一定時間ごとの平均を出す。
class DebugHud
{
public:
	void Init();
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	Text m_fps;
	float m_elapsed = 0.0f;  // 平均を取り始めてからの秒数。
	int m_frameCount = 0;    // 平均を取り始めてからのフレーム数。
};
