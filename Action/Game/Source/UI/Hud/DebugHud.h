#pragma once

#include <chrono>
#include "Text.h"

// 開発用の表示（今は FPS だけ）。ゲームの HUD とは分けて持つ。UIElement は継承しない。UIManager が持つ。
// FPS は Debug・Preview・Release のどのビルドでも出す（出すかどうかは DebugHud.cpp の kShowFps）。
// FPS は画面の右上に小さく出す。毎フレームの値は揺れて読みにくいので、一定時間ごとの平均を出す。
// FPS は実際の時間（steady_clock）で測る。ゲームの経過時間（GetFrameDeltaTime）は 1/30 秒で頭打ちになるため、
// それで測ると 30 FPS より下が出せない。
class DebugHud
{
public:
	void Init();
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	using Clock = std::chrono::steady_clock;

	Text m_fps;
	Clock::time_point m_averageStart;  // 平均を取り始めた時刻。
	int m_frameCount = 0;              // 平均を取り始めてからのフレーム数。
	bool m_isStarted = false;          // 最初の Update で m_averageStart を決めたか。
};
