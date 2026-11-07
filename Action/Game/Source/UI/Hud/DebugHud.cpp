#include "stdafx.h"
#include "DebugHud.h"

namespace
{
	// FPS の文字。右上に小さく出す。
	constexpr float kFpsMarginRight = 20.0f;  // 画面の右端から、いちばん長い表示（kFpsWidestText）の右端まで。
	constexpr float kFpsMarginTop = 20.0f;    // 画面の上端から、文字の中央まで。
	constexpr float kFpsScale = 0.4f;
	const Vector4 kFpsColor = { 1.0f, 1.0f, 1.0f, 0.9f };
	const Vector4 kFpsShadowColor = { 0.0f, 0.0f, 0.0f, 0.8f };
	constexpr float kFpsShadowOffset = 1.5f;
	// 数字の幅で文字が左右に動かないよう、左端を固定する。左端は、この文字が右端に収まる位置。
	constexpr const wchar_t* kFpsWidestText = L"FPS 0000.0";
	// この秒数ごとに、その間の平均の FPS に書き換える。
	constexpr float kFpsAverageSeconds = 0.5f;

	// FPS を出すか。Release で速さを確かめるため、Debug・Preview・Release のどのビルドでも出す。
	// 出したくないビルドがあれば、ここを #ifdef K2_DEBUG などで切り替える。
	constexpr bool kShowFps = true;
}

void DebugHud::Init()
{
	// 右寄せにすると、数字の幅が変わるたびに文字全体が左右に動く（ガクガクして見える）。
	// そのため左寄せにし、左端を「いちばん長い表示が右端に収まる位置」に固定する。
	const float widestWidth = g_graphicsEngine->GetFontEngine().MeasureString(kFpsWidestText).x * kFpsScale;
	m_fps.SetPosition({
		static_cast<float>(UI_SPACE_WIDTH) * 0.5f - kFpsMarginRight - widestWidth,
		static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - kFpsMarginTop
	});
	m_fps.SetAlign(Text::Align::Left);
	m_fps.SetScale(kFpsScale);
	m_fps.SetColor(kFpsColor);
	m_fps.SetShadow(kFpsShadowOffset, kFpsShadowColor);
	m_fps.SetVisible(kShowFps);
	// 最初の平均が出るまでの表示。
	m_fps.SetText(L"FPS --");
}

void DebugHud::Update(float deltaTime)
{
	if (!kShowFps) {
		return;
	}
	const Clock::time_point now = Clock::now();
	if (!m_isStarted) {
		m_averageStart = now;
		m_isStarted = true;
	}
	m_frameCount++;
	const float elapsed = std::chrono::duration<float>(now - m_averageStart).count();
	if (elapsed >= kFpsAverageSeconds) {
		wchar_t text[64];
		swprintf_s(text, L"FPS %.1f", m_frameCount / elapsed);
		m_fps.SetText(text);
		m_averageStart = now;
		m_frameCount = 0;
	}
	m_fps.Update(deltaTime);
}

void DebugHud::Draw(RenderContext& rc)
{
	m_fps.Draw(rc);
}
