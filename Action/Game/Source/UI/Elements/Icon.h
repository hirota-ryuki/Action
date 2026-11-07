#pragma once

#include "UIElement.h"

// 画像を1枚出す（状態異常のマーク、魔法の絵など）。位置は画像の中央。
// 画像がまだないものは、白い画像（UISprite::WHITE_PATH）に色を付けて四角で出す。
class Icon : public UIElement
{
public:
	void Init(const char* filePath, float width, float height);
	void SetColor(const Vector4& color)
	{
		m_color = color;
	}

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

private:
	SpriteRender m_sprite;
	Vector4 m_color = g_vec4White;
};
