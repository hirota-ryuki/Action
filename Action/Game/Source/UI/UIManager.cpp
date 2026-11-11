#include "stdafx.h"
#include "UIManager.h"

bool UIManager::Start()
{
	// Init は部品の画像を読むだけで、SetPlayer / ShowBoss で入れた値は消さない。
	m_playerHud.Init();
	m_pickupHud.Init();
	m_bossHud.Init();
	m_debugHud.Init();
	return true;
}

void UIManager::Update()
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
	m_playerHud.Update(deltaTime);
	m_pickupHud.Update(deltaTime);
	m_bossHud.Update(deltaTime);
	m_debugHud.Update(deltaTime);
}

void UIManager::Render(RenderContext& rc)
{
	// 後に描いたものが上に重なる。開発用の表示はいちばん上。
	m_playerHud.Draw(rc);
	m_pickupHud.Draw(rc);
	m_bossHud.Draw(rc);
	m_debugHud.Draw(rc);
}
