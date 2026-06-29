#include "stdafx.h"
#include "Game.h"

#include "Player.h"
#include "Enemy.h"
#include "GameCamera.h"
#include "Stage.h"

namespace
{
	constexpr float kSkyLuminance = 1.6f;
	constexpr float kIblLuminance = 0.16f;
	const Vector3 kMainLightDirection = { 1.0f, -1.0f, -1.0f };
	const Vector3 kMainLightColor = { 2.0f, 2.0f, 2.0f };
	const Vector3 kPlayerStartPosition = { 0.0f, 100.0f, 0.0f };
	const Vector3 kEnemyStartPosition = { 320.0f, 100.0f, 320.0f };
}

Game::Game()
{

}

Game::~Game()
{
	DeleteGO(m_stage);
	DeleteGO(m_player);
	DeleteGO(m_enemy);
	DeleteGO(m_gameCamera);
	DeleteGO(m_skyCube);
}

bool Game::Start()
{
	g_renderingEngine->DisableRaytracing();
	g_renderingEngine->SetCascadeNearAreaRates(0.01f, 0.1f, 0.5f);

	Vector3 mainLightDirection = kMainLightDirection;
	mainLightDirection.Normalize();
	g_renderingEngine->SetDirectionLight(0, mainLightDirection, kMainLightColor);

	g_camera3D->SetPosition({ 0.0f, 180.0f, -600.0f });
	g_camera3D->SetTarget({ 0.0f, 80.0f, 0.0f });
	g_camera3D->SetFar(40000.0f);

	m_stage = NewGO<Stage>(0, "stage");
	m_player = NewGO<Player>(1, "player");
	m_player->SetPosition(kPlayerStartPosition);
	m_enemy = NewGO<Enemy>(1, "enemy");
	m_enemy->SetPosition(kEnemyStartPosition);
	m_gameCamera = NewGO<GameCamera>(2, "gameCamera");

	m_skyCube = NewGO<nsK2Engine::SkyCube>(0, "skyCube");
	m_skyCube->SetType(nsK2Engine::enSkyCubeType_SunriseToon);
	m_skyCube->SetLuminance(kSkyLuminance);
	g_renderingEngine->SetAmbientByIBLTexture(m_skyCube->GetTextureFilePath(), kIblLuminance);

	return true;
}

void Game::Update()
{

}

void Game::Render(RenderContext& rc)
{
	(void)rc;
}
