#include "stdafx.h"
#include "Game.h"

#include "Player.h"
#include "Enemy.h"
#include "GameCamera.h"
#include "Stage.h"
#include "BossStage.h"
#include "FireEffect.h"
#include "BulletBase.h"
#include "BurningGround.h"
#include "UIManager.h"

namespace
{
	constexpr float SKY_LUMINANCE = 1.6f;     // スカイキューブ自体の輝度。IBLより高めにして空の映り込みを強調。
	constexpr float IBL_LUMINANCE = 0.16f;    // 環境マップ由来の環境光の強度。強すぎると影が潰れる。
	const Vector3 MAIN_LIGHT_DIRECTION = { 1.0f, -1.0f, -1.0f };  // 正規化前の向き。Start()でNormalize()する。
	const Vector3 MAIN_LIGHT_COLOR = { 2.0f, 2.0f, 2.0f };        // HDR値（1.0超）で太陽光の強さを表現。
	const Vector3 PLAYER_START_POSITION = { 0.0f, 100.0f, 0.0f };
	const Vector3 ENEMY_START_POSITION = { 320.0f, 100.0f, 320.0f };
	const Vector3 FIRE_POSITION = { -200.0f, 0.0f, 200.0f };
	const Vector3 FIRE_SCALE = { 10.0f, 10.0f, 10.0f };  // Effekseerの1単位をワールド10単位に拡大。見た目に合わせて要調整。

	// true で、草原の代わりにボス戦の広間（月明かりの玉座の間）を使う。
	constexpr bool USE_BOSS_STAGE = true;
	// 広間の光: 青白い月の光を、斜め上（玉座の方の右上）から入れる。
	const Vector3 BOSS_MOON_DIRECTION = { -0.4f, -1.0f, 0.5f };
	const Vector3 BOSS_MOON_COLOR = { 0.9f, 1.05f, 1.5f };
	constexpr float BOSS_SKY_LUMINANCE = 0.6f;
	constexpr float BOSS_IBL_LUMINANCE = 0.08f;
	// 露出は画面の平均の明るさに合わせて自動で変わる。暗い広間を昼のように持ち上げないよう、中間の明るさを下げる（ふつうは 0.18）。
	constexpr float BOSS_SCENE_MIDDLE_GRAY = 0.05f;
	const Vector3 BOSS_FIRE_POSITION = { 0.0f, 0.0f, -20000.0f };   // 広間では炎を見えないところへ置く。
}

Game::Game()
{

}

Game::~Game()
{
	// UI はキャラクターを読んでいるので、キャラクターより先に消す。
	DeleteGO(m_uiManager);
	DeleteGO(m_stage);
	DeleteGO(m_bossStage);
	DeleteGO(m_player);
	DeleteGO(m_enemy);
	DeleteGO(m_gameCamera);
	DeleteGO(m_fireEffect);
	DeleteGO(m_skyCube);
	// キャラクターが撃った弾は Game が持っていないので、名前で探して消す。
	QueryGOs<BulletBase>(BulletBase::OBJECT_NAME, [](BulletBase* bullet) {
		DeleteGO(bullet);
		return true;
	});
	QueryGOs<BurningGround>(BurningGround::OBJECT_NAME, [](BurningGround* ground) {
		DeleteGO(ground);
		return true;
	});
}

bool Game::Start()
{
	// レイトレーシングは現状未使用のため無効化。
	g_renderingEngine->DisableRaytracing();
	// カスケードシャドウの各分割領域の近距離比率を設定。近くを細かく、遠くを粗くする。
	g_renderingEngine->SetCascadeNearAreaRates(0.01f, 0.1f, 0.5f);

	Vector3 mainLightDirection = USE_BOSS_STAGE ? BOSS_MOON_DIRECTION : MAIN_LIGHT_DIRECTION;
	mainLightDirection.Normalize();
	g_renderingEngine->SetDirectionLight(0, mainLightDirection, USE_BOSS_STAGE ? BOSS_MOON_COLOR : MAIN_LIGHT_COLOR);
	if (USE_BOSS_STAGE) {
		g_renderingEngine->SetSceneMiddleGray(BOSS_SCENE_MIDDLE_GRAY);
	}

	// GameCamera::Start() が完了するまでの仮カメラ設定。
	g_camera3D->SetPosition({ 0.0f, 180.0f, -600.0f });
	g_camera3D->SetTarget({ 0.0f, 80.0f, 0.0f });
	g_camera3D->SetFar(40000.0f);

	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ、優先度3=UI（キャラが動いたあとの値を出す）。
	if (USE_BOSS_STAGE) {
		m_bossStage = NewGO<BossStage>(0, "bossStage");
	}
	else {
		m_stage = NewGO<Stage>(0, "stage");
	}
	m_player = NewGO<Player>(1, "player");
	m_player->SetPosition(USE_BOSS_STAGE ? BossStage::GetPlayerStartPosition() : PLAYER_START_POSITION);
	m_enemy = NewGO<Enemy>(1, "enemy");
	m_enemy->SetPosition(USE_BOSS_STAGE ? BossStage::GetEnemyStartPosition() : ENEMY_START_POSITION);
	m_gameCamera = NewGO<GameCamera>(2, "gameCamera");
	m_fireEffect = NewGO<FireEffect>(1, "fireEffect");
	m_fireEffect->SetPosition(USE_BOSS_STAGE ? BOSS_FIRE_POSITION : FIRE_POSITION);
	m_fireEffect->SetScale(FIRE_SCALE);
	m_uiManager = NewGO<UIManager>(3, "uiManager");
	m_uiManager->SetPlayer(m_player);

	// SkyCubeのテクスチャをIBLの環境マップとしても使い回す。
	m_skyCube = NewGO<nsK2Engine::SkyCube>(0, "skyCube");
	m_skyCube->SetType(USE_BOSS_STAGE ? nsK2Engine::enSkyCubeType_NightToon : nsK2Engine::enSkyCubeType_SunriseToon);
	m_skyCube->SetLuminance(USE_BOSS_STAGE ? BOSS_SKY_LUMINANCE : SKY_LUMINANCE);
	g_renderingEngine->SetAmbientByIBLTexture(m_skyCube->GetTextureFilePath(), USE_BOSS_STAGE ? BOSS_IBL_LUMINANCE : IBL_LUMINANCE);

	return true;
}

void Game::Update()
{

}

void Game::Render(RenderContext& rc)
{
	(void)rc;
}
