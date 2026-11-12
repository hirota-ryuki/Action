#include "stdafx.h"
#include "BossStage.h"

namespace
{
	const char* HALL_MODEL_PATH = "Assets/modelData/stage/boss/BossHall.fbx";
	const char* HALL_COLLISION_PATH = "Assets/modelData/stage/boss/collision/BossHallCollision.fbx";

	// ゲームの +Z が玉座の方、-Z が入口の扉の方（cm）。広間は X が ±1400、Z が ±2400。
	const Vector3 PLAYER_START_POSITION = { 0.0f, 100.0f, -1800.0f };
	const Vector3 ENEMY_START_POSITION = { 0.0f, 100.0f, 1000.0f };

	// 燭台のろうそくの暖かい光。燭台の置き場所（make_boss_stage.py の CANDELABRA）と、ろうそくの高さ（約 2.3m）に合わせる。
	struct LightDesc {
		Vector3 position;
		Vector3 color;
		float range;
	};
	const LightDesc LIGHTS[] = {
		{ { -450.0f, 230.0f, 1250.0f }, { 3.0f, 1.6f, 0.7f }, 900.0f },
		{ {  450.0f, 230.0f, 1250.0f }, { 3.0f, 1.6f, 0.7f }, 900.0f },
		{ { -400.0f, 230.0f, -2100.0f }, { 2.0f, 1.1f, 0.5f }, 700.0f },
		{ {  400.0f, 230.0f, -2100.0f }, { 2.0f, 1.1f, 0.5f }, 700.0f },
	};
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

	for (const auto& desc : LIGHTS) {
		auto light = std::make_unique<nsK2Engine::PointLight>();
		light->Init();
		light->SetPosition(desc.position);
		light->SetColor(desc.color);
		light->SetRange(desc.range);
		light->SetAffectPowParam(LIGHT_AFFECT_POW);
		m_lights.push_back(std::move(light));
	}
	return true;
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
