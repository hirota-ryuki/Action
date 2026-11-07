#include "stdafx.h"
#include "Icon.h"

void Icon::Init(const char* filePath, float width, float height)
{
	m_sprite.Init(filePath, width, height);
}

void Icon::Update(float deltaTime)
{
	m_sprite.SetPosition(Vector3(m_position.x, m_position.y, 0.0f));
	m_sprite.SetMulColor(ApplyAlpha(m_color));
	m_sprite.Update();
}

void Icon::OnDraw(RenderContext& rc)
{
	m_sprite.Draw(rc);
}
