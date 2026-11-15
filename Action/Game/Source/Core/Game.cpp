#include "stdafx.h"
#include "Game.h"

#include "Player.h"
#include "Enemy.h"
#include "GameCamera.h"
#include "BossStage.h"
#include "SkillBase.h"
#include "GameClock.h"
#include "BurningGround.h"
#include "UIManager.h"

namespace
{
	// 広間の光: 青白い月の光を、玉座の後ろの窓の方の上から入れる（見本の画像のように、玉座を逆光にする）。
	const Vector3 BOSS_MOON_DIRECTION = { 0.35f, -1.0f, -0.45f };
	const Vector3 BOSS_MOON_COLOR = { 0.55f, 0.7f, 1.35f };
	constexpr float BOSS_SKY_LUMINANCE = 0.6f;
	constexpr float BOSS_IBL_LUMINANCE = 0.08f;
	// 露出は画面の平均の明るさに合わせて自動で変わる。暗い広間を昼のように持ち上げないよう、中間の明るさを下げる（ふつうは 0.18）。
	constexpr float BOSS_SCENE_MIDDLE_GRAY = 0.05f;
}

Game::Game()
{

}

Game::~Game()
{
	// UI はキャラクターを読んでいるので、キャラクターより先に消す。
	DeleteGO(m_uiManager);
	DeleteGO(m_bossStage);
	DeleteGO(m_player);
	DeleteGO(m_enemy);
	DeleteGO(m_gameCamera);
	DeleteGO(m_skyCube);
	// キャラクターが使ったスキル（魔法など）は Game が持っていないので、名前で探して消す。
	QueryGOs<SkillBase>(SkillBase::OBJECT_NAME, [](SkillBase* skill) {
		DeleteGO(skill);
		return true;
	});
	QueryGOs<BurningGround>(BurningGround::OBJECT_NAME, [](BurningGround* ground) {
		DeleteGO(ground);
		return true;
	});
}

bool Game::Start()
{
	// カスケードシャドウの各分割領域の近距離比率を設定。近くを細かく、遠くを粗くする。
	g_renderingEngine->SetCascadeNearAreaRates(0.01f, 0.1f, 0.5f);

	Vector3 mainLightDirection = BOSS_MOON_DIRECTION;
	mainLightDirection.Normalize();
	g_renderingEngine->SetDirectionLight(0, mainLightDirection, BOSS_MOON_COLOR);
	g_renderingEngine->SetSceneMiddleGray(BOSS_SCENE_MIDDLE_GRAY);

	// GameCamera::Start() が完了するまでの仮カメラ設定。
	g_camera3D->SetPosition({ 0.0f, 180.0f, -600.0f });
	g_camera3D->SetTarget({ 0.0f, 80.0f, 0.0f });
	g_camera3D->SetFar(40000.0f);

	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ、優先度3=UI（キャラが動いたあとの値を出す）。
	m_bossStage = NewGO<BossStage>(0, "bossStage");
	m_player = NewGO<Player>(1, "player");
	m_player->SetPosition(BossStage::GetPlayerStartPosition());
	m_enemy = NewGO<Enemy>(1, "enemy");
	m_enemy->SetPosition(BossStage::GetEnemyStartPosition());
	m_gameCamera = NewGO<GameCamera>(2, "gameCamera");
	m_uiManager = NewGO<UIManager>(3, "uiManager");
	m_uiManager->SetPlayer(m_player);

	// SkyCubeのテクスチャをIBLの環境マップとしても使い回す。
	m_skyCube = NewGO<nsK2Engine::SkyCube>(0, "skyCube");
	m_skyCube->SetType(nsK2Engine::enSkyCubeType_NightToon);
	m_skyCube->SetLuminance(BOSS_SKY_LUMINANCE);
	g_renderingEngine->SetAmbientByIBLTexture(m_skyCube->GetTextureFilePath(), BOSS_IBL_LUMINANCE);

	return true;
}

void Game::Update()
{
	// Game はキャラクター（優先度1）より先に更新されるので、このフレームの時刻をここで進める。
	GameClock::Advance(g_gameTime->GetFrameDeltaTime());
}

void Game::Render(RenderContext& rc)
{
	(void)rc;
}
