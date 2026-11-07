#include "stdafx.h"
#include "DebugHud.h"

namespace
{
	// FPS の文字。右上に小さく出す。
	constexpr float FPS_MARGIN_RIGHT = 20.0f;  // 画面の右端から、いちばん長い表示（FPS_WIDEST_TEXT）の右端まで。
	constexpr float FPS_MARGIN_TOP = 20.0f;    // 画面の上端から、文字の中央まで。
	constexpr float FPS_SCALE = 0.4f;
	const Vector4 FPS_COLOR = { 1.0f, 1.0f, 1.0f, 0.9f };
	const Vector4 FPS_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.8f };
	constexpr float FPS_SHADOW_OFFSET = 1.5f;
	// 数字の幅で文字が左右に動かないよう、左端を固定する。左端は、この文字が右端に収まる位置。
	constexpr const wchar_t* FPS_WIDEST_TEXT = L"FPS 0000.0";
	// この秒数ごとに、その間の平均の FPS に書き換える。
	constexpr float FPS_AVERAGE_SECONDS = 0.5f;

	// FPS を出すか。Release で速さを確かめるため、Debug・Preview・Release のどのビルドでも出す。
	// 出したくないビルドがあれば、ここを #ifdef K2_DEBUG などで切り替える。
	constexpr bool SHOW_FPS = true;
}

void DebugHud::Init()
{
	// 右寄せにすると、数字の幅が変わるたびに文字全体が左右に動く（ガクガクして見える）。
	// そのため左寄せにし、左端を「いちばん長い表示が右端に収まる位置」に固定する。
	const float widestWidth = g_graphicsEngine->GetFontEngine().MeasureString(FPS_WIDEST_TEXT).x * FPS_SCALE;
	m_fps.SetPosition({
		static_cast<float>(UI_SPACE_WIDTH) * 0.5f - FPS_MARGIN_RIGHT - widestWidth,
		static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - FPS_MARGIN_TOP
	});
	m_fps.SetAlign(Text::Align::Left);
	m_fps.SetScale(FPS_SCALE);
	m_fps.SetColor(FPS_COLOR);
	m_fps.SetShadow(FPS_SHADOW_OFFSET, FPS_SHADOW_COLOR);
	m_fps.SetVisible(SHOW_FPS);
	// 最初の平均が出るまでの表示。
	m_fps.SetText(L"FPS --");
}

void DebugHud::Update(float deltaTime)
{
	if (!SHOW_FPS) {
		return;
	}
	const Clock::time_point now = Clock::now();
	if (!m_isStarted) {
		m_averageStart = now;
		m_isStarted = true;
	}
	m_frameCount++;
	const float elapsed = std::chrono::duration<float>(now - m_averageStart).count();
	if (elapsed >= FPS_AVERAGE_SECONDS) {
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
