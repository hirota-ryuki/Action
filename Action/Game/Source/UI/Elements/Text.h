#pragma once

#include "UIElement.h"

// 文字を出す。位置は文字の中央。
class Text : public UIElement
{
public:
	// 値が変わったときだけ描画用の文字を書き換える。毎フレーム呼んでよい。
	// FontRender は文字を書式（swprintf の書式）として扱うので、% は %% と書く。
	void SetText(const wchar_t* text);
	void SetColor(const Vector4& color)
	{
		m_color = color;
	}
	void SetScale(float scale)
	{
		m_font.SetScale(scale);
	}

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

private:
	FontRender m_font;
	std::wstring m_text;
	Vector4 m_color = g_vec4White;
};
