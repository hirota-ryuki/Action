#include "stdafx.h"
#include "DebugHud.h"

namespace
{
	// FPS の文字。右上に小さく、右端をそろえる。
	constexpr float kFpsMarginRight = 20.0f;  // 画面の右端から、文字の右端まで。
	constexpr float kFpsMarginTop = 20.0f;    // 画面の上端から、文字の中央まで。
	constexpr float kFpsScale = 0.4f;
	const Vector4 kFpsColor = { 1.0f, 1.0f, 1.0f, 0.9f };
	const Vector4 kFpsShadowColor = { 0.0f, 0.0f, 0.0f, 0.8f };
	constexpr float kFpsShadowOffset = 1.5f;
	// この秒数ごとに、その間の平均の FPS に書き換える。
	constexpr float kFpsAverageSeconds = 0.5f;

	// K2_DEBUG（Debug・Preview ビルド）のときだけ出す。
#ifdef K2_DEBUG
	constexpr bool kIsDebugBuild = true;
#else
	constexpr bool kIsDebugBuild = false;
#endif
}

void DebugHud::Init()
{
	m_fps.SetPosition({
		static_cast<float>(UI_SPACE_WIDTH) * 0.5f - kFpsMarginRight,
		static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - kFpsMarginTop
	});
	m_fps.SetAlign(Text::Align::Right);
	m_fps.SetScale(kFpsScale);
	m_fps.SetColor(kFpsColor);
	m_fps.SetShadow(kFpsShadowOffset, kFpsShadowColor);
	m_fps.SetVisible(kIsDebugBuild);
	// 最初の平均が出るまでの表示。
	m_fps.SetText(L"FPS --");
}

void DebugHud::Update(float deltaTime)
{
	if (!kIsDebugBuild) {
		return;
	}
	m_elapsed += deltaTime;
	m_frameCount++;
	if (m_elapsed >= kFpsAverageSeconds) {
		wchar_t text[64];
		swprintf_s(text, L"FPS %.1f", m_frameCount / m_elapsed);
		m_fps.SetText(text);
		m_elapsed = 0.0f;
		m_frameCount = 0;
	}
	m_fps.Update(deltaTime);
}

void DebugHud::Draw(RenderContext& rc)
{
	m_fps.Draw(rc);
}
