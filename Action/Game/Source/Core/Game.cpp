#include "stdafx.h"
#include "Game.h"

#include "Player.h"
#include "Enemy.h"
#include "GameCamera.h"
#include "Stage.h"

namespace
{
	constexpr float kSkyLuminance = 1.6f;     // スカイキューブ自体の輝度。IBLより高めにして空の映り込みを強調。
	constexpr float kIblLuminance = 0.16f;    // 環境マップ由来の環境光の強度。強すぎると影が潰れる。
	const Vector3 kMainLightDirection = { 1.0f, -1.0f, -1.0f };  // 正規化前の向き。Start()でNormalize()する。
	const Vector3 kMainLightColor = { 2.0f, 2.0f, 2.0f };        // HDR値（1.0超）で太陽光の強さを表現。
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
	// レイトレーシングは現状未使用のため無効化。
	g_renderingEngine->DisableRaytracing();
	// カスケードシャドウの各分割領域の近距離比率を設定。近くを細かく、遠くを粗くする。
	g_renderingEngine->SetCascadeNearAreaRates(0.01f, 0.1f, 0.5f);

	Vector3 mainLightDirection = kMainLightDirection;
	mainLightDirection.Normalize();
	g_renderingEngine->SetDirectionLight(0, mainLightDirection, kMainLightColor);

	// GameCamera::Start() が完了するまでの仮カメラ設定。
	g_camera3D->SetPosition({ 0.0f, 180.0f, -600.0f });
	g_camera3D->SetTarget({ 0.0f, 80.0f, 0.0f });
	g_camera3D->SetFar(40000.0f);

	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ（最後に更新）。
	m_stage = NewGO<Stage>(0, "stage");
	m_player = NewGO<Player>(1, "player");
	m_player->SetPosition(kPlayerStartPosition);
	m_enemy = NewGO<Enemy>(1, "enemy");
	m_enemy->SetPosition(kEnemyStartPosition);
	m_gameCamera = NewGO<GameCamera>(2, "gameCamera");

	// SkyCubeのテクスチャをIBLの環境マップとしても使い回す。
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
