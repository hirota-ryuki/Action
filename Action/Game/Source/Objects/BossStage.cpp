#include "stdafx.h"
#include "BossStage.h"

namespace
{
	const char* HALL_MODEL_PATH = "Assets/modelData/stage/boss/BossHall.fbx";
	const char* HALL_COLLISION_PATH = "Assets/modelData/stage/boss/collision/BossHallCollision.fbx";

	// ゲームの +Z が玉座の方、-Z が入口の扉の方（cm）。広間は X が ±1400、Z が ±2400。
	const Vector3 PLAYER_START_POSITION = { 0.0f, 100.0f, -1800.0f };
	const Vector3 ENEMY_START_POSITION = { 0.0f, 100.0f, 1000.0f };

	// 燭台のろうそくの暖かい光。燭台の置き場所と、ろうそくの高さに合わせる（make_boss_stage.py の CANDELABRUM_X・PILLAR_ZS・DAIS_CANDELABRA）。
	// 柱の前の燭台: 柱ごとに1本（左右 × 5列）。燭台の高さ 3m、ろうそくは約 2.6m。
	constexpr float CANDELABRUM_X = 680.0f;
	constexpr float CANDLE_HEIGHT = 260.0f;
	const float PILLAR_ZS[] = { -1800.0f, -1000.0f, -200.0f, 600.0f, 1400.0f };
	const Vector3 PILLAR_CANDLE_COLOR = { 2.0f, 1.1f, 0.5f };
	constexpr float PILLAR_CANDLE_RANGE = 700.0f;
	// 台座の上、玉座の手前の小さな燭台（台座の上面 180cm + ろうそく約 156cm）。
	const Vector3 DAIS_CANDLE_POSITIONS[] = { { -260.0f, 336.0f, 1870.0f }, { 260.0f, 336.0f, 1870.0f } };
	const Vector3 DAIS_CANDLE_COLOR = { 3.0f, 1.6f, 0.7f };
	constexpr float DAIS_CANDLE_RANGE = 900.0f;
	// 壁の灯りのランタン（make_boss_stage.py の SCONCES。書き出すときに出る位置）。入口の壁に4つ、玉座の後ろの壁の両端に2つ。
	const Vector3 SCONCE_LIGHT_POSITIONS[] = {
		{ -500.0f, 350.0f, -2318.0f }, { 500.0f, 350.0f, -2318.0f },
		{ -1000.0f, 350.0f, -2318.0f }, { 1000.0f, 350.0f, -2318.0f },
		{ -1050.0f, 350.0f, 2318.0f }, { 1050.0f, 350.0f, 2318.0f },
	};
	const Vector3 SCONCE_LIGHT_COLOR = { 2.0f, 1.1f, 0.5f };
	constexpr float SCONCE_LIGHT_RANGE = 600.0f;
	constexpr float LIGHT_AFFECT_POW = 2.0f;
}

BossStage::BossStage()
{
}

BossStage::~BossStage()
{
}

bool BossStage::Start()
{
	// Blender で作った Z が上の FBX。広間の床や柱は影を受け、柱・壁は影を落とす。
	m_model.Init(HALL_MODEL_PATH, nullptr, 0, enModelUpAxisZ, true);
	m_model.SetShadowCasterFlag(true);
	m_model.Update();

	m_collisionModel.Init(HALL_COLLISION_PATH, nullptr, 0, enModelUpAxisZ, false);
	m_collisionModel.Update();
	m_physicsStaticObject.CreateFromModel(m_collisionModel.GetModel(), m_collisionModel.GetModel().GetWorldMatrix());
	m_physicsStaticObject.SetFriction(1.0f);

	for (float z : PILLAR_ZS) {
		AddCandleLight({ -CANDELABRUM_X, CANDLE_HEIGHT, z }, PILLAR_CANDLE_COLOR, PILLAR_CANDLE_RANGE);
		AddCandleLight({ CANDELABRUM_X, CANDLE_HEIGHT, z }, PILLAR_CANDLE_COLOR, PILLAR_CANDLE_RANGE);
	}
	for (const auto& position : DAIS_CANDLE_POSITIONS) {
		AddCandleLight(position, DAIS_CANDLE_COLOR, DAIS_CANDLE_RANGE);
	}
	for (const auto& position : SCONCE_LIGHT_POSITIONS) {
		AddCandleLight(position, SCONCE_LIGHT_COLOR, SCONCE_LIGHT_RANGE);
	}
	return true;
}

void BossStage::AddCandleLight(const Vector3& position, const Vector3& color, float range)
{
	auto light = std::make_unique<nsK2Engine::PointLight>();
	light->Init();
	light->SetPosition(position);
	light->SetColor(color);
	light->SetRange(range);
	light->SetAffectPowParam(LIGHT_AFFECT_POW);
	m_lights.push_back(std::move(light));
}

void BossStage::Update()
{
}

void BossStage::Render(RenderContext& rc)
{
	m_model.Draw(rc);
}

Vector3 BossStage::GetPlayerStartPosition()
{
	return PLAYER_START_POSITION;
}

Vector3 BossStage::GetEnemyStartPosition()
{
	return ENEMY_START_POSITION;
}
