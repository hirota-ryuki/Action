#pragma once

#include "UIElement.h"

// 文字を出す。位置は、横は Align で決めた端（左端・中央・右端）、縦は文字の中央。
class Text : public UIElement
{
public:
	enum class Align
	{
		Left,
		Center,
		Right,
	};

	// 値が変わったときだけ描画用の文字を書き換える。毎フレーム呼んでよい。
	// FontRender は文字を書式（swprintf の書式）として扱うので、% は %% と書く。
	void SetText(const wchar_t* text);
	void SetColor(const Vector4& color)
	{
		m_color = color;
	}
	void SetScale(float scale)
	{
		m_scale = scale;
	}
	void SetAlign(Align align)
	{
		m_align = align;
	}
	// 文字の周りに影（縁取り）を付ける。offset はずらすピクセル数。
	void SetShadow(float offset, const Vector4& color)
	{
		m_shadowOffset = offset;
		m_shadowColor = color;
	}

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

private:
	FontRender m_font;
	std::wstring m_text;
	Vector2 m_textSize = Vector2::Zero;  // 大きさ 1 のときの文字の幅と高さ。SetText のときに測る。
	Vector4 m_color = g_vec4White;
	float m_scale = 1.0f;
	Align m_align = Align::Center;
	float m_shadowOffset = 0.0f;  // 0 なら影なし。
	Vector4 m_shadowColor = { 0.0f, 0.0f, 0.0f, 0.8f };
};
