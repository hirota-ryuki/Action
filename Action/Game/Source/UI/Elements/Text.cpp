#include "stdafx.h"
#include "Text.h"

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
}

void Text::Update(float deltaTime)
{
	m_font.SetPosition(m_position.x, m_position.y, 0.0f);
	m_font.SetColor(ApplyAlpha(m_color));
}

void Text::OnDraw(RenderContext& rc)
{
	if (m_text.empty()) {
		return;
	}
	m_font.Draw(rc);
}
