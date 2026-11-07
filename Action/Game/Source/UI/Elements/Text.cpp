#include "stdafx.h"
#include "Text.h"

namespace
{
	// エンジンの Font は、pivot を割合ではなく「文字の左上からのピクセル数」として扱う（y は 1 - pivot.y になる）。
	// { 0, 1 } を渡すと、位置が文字の左上になる。寄せるのは、測った幅で位置をずらして行う。
	const Vector2 kTopLeftPivot = { 0.0f, 1.0f };
}

void Text::SetText(const wchar_t* text)
{
	if (text == nullptr) {
		text = L"";
	}
	if (m_text == text) {
		return;
	}
	m_text = text;
	m_font.SetText(m_text.c_str());
	m_textSize = m_text.empty() ? Vector2::Zero : g_graphicsEngine->GetFontEngine().MeasureString(m_text.c_str());
}

void Text::Update(float deltaTime)
{
	const float width = m_textSize.x * m_scale;
	const float height = m_textSize.y * m_scale;

	float left = m_position.x;
	if (m_align == Align::Center) {
		left -= width * 0.5f;
	}
	else if (m_align == Align::Right) {
		left -= width;
	}
	// 位置の y は文字の中央なので、上端は高さの半分だけ上。
	const float top = m_position.y + height * 0.5f;

	m_font.SetPivot(kTopLeftPivot);
	m_font.SetScale(m_scale);
	m_font.SetPosition(left, top, 0.0f);
	m_font.SetColor(ApplyAlpha(m_color));
	m_font.SetShadowParam(m_shadowOffset > 0.0f, m_shadowOffset, ApplyAlpha(m_shadowColor));
}

void Text::OnDraw(RenderContext& rc)
{
	if (m_text.empty()) {
		return;
	}
	m_font.Draw(rc);
}
