#include "stdafx.h"
#include "BossStage.h"

namespace
{
	const char* HALL_MODEL_PATH = "Assets/modelData/stage/boss/BossHall.fbx";
	const char* CEILING_MODEL_PATH = "Assets/modelData/stage/boss/BossHallCeiling.fbx";
	const char* HALL_COLLISION_PATH = "Assets/modelData/stage/boss/collision/BossHallCollision.fbx";

	// ゲームの +Z が玉座の方、-Z が入口の扉の方（cm）。広間は X が ±1400、Z が ±2400。
	const Vector3 PLAYER_START_POSITION = { 0.0f, 100.0f, -1800.0f };
	const Vector3 ENEMY_START_POSITION = { 0.0f, 100.0f, 1000.0f };

	constexpr float LIGHT_AFFECT_POW = 2.0f;

	// ---- 燭台・壁の灯り（暖かい光） ----
	// 置き場所は make_boss_stage.py の CANDELABRUM_X・PILLAR_ZS・DAIS_CANDELABRA・SCONCES に合わせる。
	// 柱の前の燭台: 柱ごとに1本（左右 × 5列）。燭台の高さ 3m、ろうそくは約 2.6m。
	constexpr float CANDELABRUM_X = 680.0f;
	constexpr float CANDLE_HEIGHT = 260.0f;
	const float PILLAR_ZS[] = { -1800.0f, -1000.0f, -200.0f, 600.0f, 1400.0f };
	const Vector3 PILLAR_CANDLE_COLOR = { 2.6f, 1.3f, 0.5f };
	constexpr float PILLAR_CANDLE_RANGE = 700.0f;
	// 台座の上、玉座の手前の小さな燭台（台座の上面 180cm + ろうそく約 156cm）。
	const Vector3 DAIS_CANDLE_POSITIONS[] = { { -260.0f, 336.0f, 1870.0f }, { 260.0f, 336.0f, 1870.0f } };
	const Vector3 DAIS_CANDLE_COLOR = { 3.0f, 1.5f, 0.6f };
	constexpr float DAIS_CANDLE_RANGE = 900.0f;
	// 壁の灯りのランタン（書き出すときに出る位置）。入口の壁に4つ、玉座の後ろの壁の両端に2つ。
	const Vector3 SCONCE_LIGHT_POSITIONS[] = {
		{ -500.0f, 350.0f, -2318.0f }, { 500.0f, 350.0f, -2318.0f },
		{ -1000.0f, 350.0f, -2318.0f }, { 1000.0f, 350.0f, -2318.0f },
		{ -1050.0f, 350.0f, 2318.0f }, { 1050.0f, 350.0f, 2318.0f },
	};
	const Vector3 SCONCE_LIGHT_COLOR = { 2.0f, 1.0f, 0.4f };
	constexpr float SCONCE_LIGHT_RANGE = 600.0f;

	// ---- 窓から入る青い月の光 ----
	// 玉座の後ろの大きな窓の前。玉座と石像を後ろから青く照らし、影絵のように見せる。
	const Vector3 BACK_WINDOW_LIGHT_POSITION = { 0.0f, 900.0f, 2250.0f };
	const Vector3 BACK_WINDOW_LIGHT_COLOR = { 0.6f, 0.8f, 2.6f };
	constexpr float BACK_WINDOW_LIGHT_RANGE = 2000.0f;
	// 横の窓の前（make_boss_stage.py の WINDOW_ZS）。
	constexpr float SIDE_WINDOW_LIGHT_X = 1150.0f;
	constexpr float SIDE_WINDOW_LIGHT_HEIGHT = 700.0f;
	const float WINDOW_ZS[] = { -1400.0f, -600.0f, 200.0f, 1000.0f, 1800.0f };
	const Vector3 SIDE_WINDOW_LIGHT_COLOR = { 0.35f, 0.5f, 1.6f };
	constexpr float SIDE_WINDOW_LIGHT_RANGE = 900.0f;

	// ---- 光の筋（ボリュームスポットライト） ----
	// 窓から斜めに差し込む月の光。筋の明るさは、筋の向こうに見える物の色に比例する（DrawVolumeLight.fx）ので、暗い広間では強めの色にする。
	struct ShaftDesc {
		Vector3 position;
		Vector3 target;   // 筋が当たる床の点
		float angle;      // 広がりの半分の角度（ラジアン）
		float range;
		Vector3 color;
	};
	const ShaftDesc SHAFTS[] = {
		// 玉座の後ろの大きな窓から、広間の真ん中の紋章へ
		{ { 0.0f, 1500.0f, 2350.0f }, { 0.0f, 0.0f, 300.0f }, 0.18f, 3200.0f, { 6.0f, 8.0f, 20.0f } },
		// 横の窓から、通り道へ
		{ { -1330.0f, 1100.0f, -1400.0f }, { -300.0f, 0.0f, -1700.0f }, 0.15f, 2000.0f, { 4.0f, 5.0f, 14.0f } },
		{ {  1330.0f, 1100.0f, -600.0f },  {  300.0f, 0.0f, -900.0f },  0.15f, 2000.0f, { 4.0f, 5.0f, 14.0f } },
		{ { -1330.0f, 1100.0f, 200.0f },   { -300.0f, 0.0f, -100.0f },  0.15f, 2000.0f, { 4.0f, 5.0f, 14.0f } },
		{ {  1330.0f, 1100.0f, 1000.0f },  {  300.0f, 0.0f, 700.0f },   0.15f, 2000.0f, { 4.0f, 5.0f, 14.0f } },
	};

	// ---- 通り道の両側の紫の光る帯 ----
	// 帯（make_boss_stage.py の RUNE_X・RUNE_Z_START・RUNE_Z_END）のすぐ上に、紫の小さなライトを並べて、帯が光って見えるようにする。
	constexpr float RUNE_X = 560.0f;
	constexpr float RUNE_LIGHT_HEIGHT = 40.0f;
	constexpr float RUNE_Z_START = -2200.0f;
	constexpr float RUNE_Z_END = 1300.0f;
	constexpr float RUNE_LIGHT_STEP = 350.0f;
	const Vector3 RUNE_LIGHT_COLOR = { 1.4f, 0.4f, 3.0f };
	constexpr float RUNE_LIGHT_RANGE = 260.0f;
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

	// 天井は影を落とさない（落とすと、月の光が広間に入らなくなる）。
	m_ceilingModel.Init(CEILING_MODEL_PATH, nullptr, 0, enModelUpAxisZ, false);
	m_ceilingModel.SetShadowCasterFlag(false);
	m_ceilingModel.Update();

	m_collisionModel.Init(HALL_COLLISION_PATH, nullptr, 0, enModelUpAxisZ, false);
	m_collisionModel.Update();
	m_physicsStaticObject.CreateFromModel(m_collisionModel.GetModel(), m_collisionModel.GetModel().GetWorldMatrix());
	m_physicsStaticObject.SetFriction(1.0f);

	for (float z : PILLAR_ZS) {
		AddPointLight({ -CANDELABRUM_X, CANDLE_HEIGHT, z }, PILLAR_CANDLE_COLOR, PILLAR_CANDLE_RANGE);
		AddPointLight({ CANDELABRUM_X, CANDLE_HEIGHT, z }, PILLAR_CANDLE_COLOR, PILLAR_CANDLE_RANGE);
	}
	for (const auto& position : DAIS_CANDLE_POSITIONS) {
		AddPointLight(position, DAIS_CANDLE_COLOR, DAIS_CANDLE_RANGE);
	}
	for (const auto& position : SCONCE_LIGHT_POSITIONS) {
		AddPointLight(position, SCONCE_LIGHT_COLOR, SCONCE_LIGHT_RANGE);
	}
	AddPointLight(BACK_WINDOW_LIGHT_POSITION, BACK_WINDOW_LIGHT_COLOR, BACK_WINDOW_LIGHT_RANGE);
	for (float z : WINDOW_ZS) {
		AddPointLight({ -SIDE_WINDOW_LIGHT_X, SIDE_WINDOW_LIGHT_HEIGHT, z }, SIDE_WINDOW_LIGHT_COLOR, SIDE_WINDOW_LIGHT_RANGE);
		AddPointLight({ SIDE_WINDOW_LIGHT_X, SIDE_WINDOW_LIGHT_HEIGHT, z }, SIDE_WINDOW_LIGHT_COLOR, SIDE_WINDOW_LIGHT_RANGE);
	}
	for (float z = RUNE_Z_START; z <= RUNE_Z_END; z += RUNE_LIGHT_STEP) {
		AddPointLight({ -RUNE_X, RUNE_LIGHT_HEIGHT, z }, RUNE_LIGHT_COLOR, RUNE_LIGHT_RANGE);
		AddPointLight({ RUNE_X, RUNE_LIGHT_HEIGHT, z }, RUNE_LIGHT_COLOR, RUNE_LIGHT_RANGE);
	}

	for (const auto& desc : SHAFTS) {
		auto spot = std::make_unique<nsK2Engine::SpotLight>();
		spot->Init();
		spot->SetPosition(desc.position);
		Vector3 direction = desc.target - desc.position;
		direction.Normalize();
		spot->SetDirection(direction);
		spot->SetAngle(desc.angle);
		spot->SetRange(desc.range);
		spot->SetColor(desc.color);
		spot->SetRangeAffectPowParam(LIGHT_AFFECT_POW);
		auto volume = std::make_unique<nsK2Engine::VolumeSpotLight>();
		volume->Init(*spot);
		m_spotLights.push_back(std::move(spot));
		m_volumeSpotLights.push_back(std::move(volume));
	}
	UpdateLights();
	return true;
}

void BossStage::AddPointLight(const Vector3& position, const Vector3& color, float range)
{
	auto light = std::make_unique<nsK2Engine::PointLight>();
	light->Init();
	light->SetPosition(position);
	light->SetColor(color);
	light->SetRange(range);
	light->SetAffectPowParam(LIGHT_AFFECT_POW);
	m_lights.push_back(std::move(light));
}

void BossStage::UpdateLights()
{
	// ライトのカメラから見た位置は、カメラが動くたびに計算し直す必要がある（エンジンが画面のどこを照らすかをこれで選ぶ）。
	for (auto& light : m_lights) {
		light->Update();
	}
	for (auto& spot : m_spotLights) {
		spot->Update();
	}
	for (auto& volume : m_volumeSpotLights) {
		volume->Update();
	}
}

void BossStage::Update()
{
	UpdateLights();
}

void BossStage::Render(RenderContext& rc)
{
	m_model.Draw(rc);
	m_ceilingModel.Draw(rc);
}

Vector3 BossStage::GetPlayerStartPosition()
{
	return PLAYER_START_POSITION;
}

Vector3 BossStage::GetEnemyStartPosition()
{
	return ENEMY_START_POSITION;
}
