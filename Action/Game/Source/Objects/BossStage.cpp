#include "stdafx.h"
#include "BossStage.h"

namespace
{
	const char* HALL_MODEL_PATH = "Assets/modelData/stage/boss/BossHall.fbx";
	const char* NO_SHADOW_MODEL_PATH = "Assets/modelData/stage/boss/BossHallNoShadow.fbx";   // 天井と炎
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

	// ---- 炎を照らすライト ----
	// 炎は板の法線を真下に向けてあるので、炎の段ごとに、根元のすぐ下に置いた明るいライトで光って見せる（上からの月の光や夜空の映り込みを拾わない）。
	// 高さは make_boss_stage.py が書き出すときに出す「FLAME LIGHT cm」。燭台は炎が2段なので、ライトも2つ。
	const float CANDELABRUM_FLAME_LIGHT_HEIGHTS[] = { 209.0f, 258.0f };
	const float DAIS_FLAME_LIGHT_HEIGHTS[] = { 306.0f, 335.0f };
	constexpr float SCONCE_FLAME_LIGHT_HEIGHT = 360.0f;
	constexpr float SCONCE_FLAME_LIGHT_Z = 2322.0f;   // 入口の壁は -、玉座の後ろの壁は +
	const Vector3 FLAME_LIGHT_COLOR = { 14.0f, 7.0f, 2.0f };
	constexpr float FLAME_LIGHT_RANGE = 160.0f;   // 燭台の外側の炎（ライトから横に約 50cm）までしっかり届く

	// ---- 炎の揺らめき ----
	// ろうそくの光の明るさを、2つの速さの波を重ねて少しだけ揺らす。
	constexpr float FLICKER_SPEED_1 = 9.0f;      // 1秒に何ラジアン進むか
	constexpr float FLICKER_SPEED_2 = 23.0f;
	constexpr float FLICKER_AMOUNT_1 = 0.6f;     // 2つの波の割合
	constexpr float FLICKER_AMOUNT_2 = 0.4f;
	constexpr float FLAME_FLICKER = 0.15f;       // 炎のライトの揺れ幅（明るさの割合）
	constexpr float CANDLE_FLICKER = 0.06f;      // 広間を照らすろうそくのライトの揺れ幅

	// ---- 窓から入る青い月の光 ----
	// 玉座の後ろの大きな窓の前。玉座と石像を後ろから青く照らし、影絵のように見せる。
	const Vector3 BACK_WINDOW_LIGHT_POSITION = { 0.0f, 900.0f, 2250.0f };
	const Vector3 BACK_WINDOW_LIGHT_COLOR = { 0.6f, 0.8f, 2.6f };
	constexpr float BACK_WINDOW_LIGHT_RANGE = 2000.0f;
	// 横の窓の前（make_boss_stage.py の WINDOW_ZS）。
	constexpr float SIDE_WINDOW_LIGHT_X = 1150.0f;
	constexpr float SIDE_WINDOW_LIGHT_HEIGHT = 700.0f;
	const float WINDOW_ZS[] = { -1400.0f, -600.0f, 200.0f, 1000.0f, 1800.0f };
	const Vector3 SIDE_WINDOW_LIGHT_COLOR = { 0.2f, 0.3f, 0.9f };
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
	constexpr float RUNE_LIGHT_HEIGHT = 15.0f;   // 低くして、床に丸い光の点が映りにくくする
	constexpr float RUNE_Z_START = -2200.0f;
	constexpr float RUNE_Z_END = 1300.0f;
	constexpr float RUNE_LIGHT_STEP = 250.0f;
	const Vector3 RUNE_LIGHT_COLOR = { 0.6f, 0.18f, 1.3f };
	constexpr float RUNE_LIGHT_RANGE = 180.0f;
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

	// 天井と炎は影を落とさない（天井が落とすと、月の光が広間に入らなくなる）。
	m_noShadowModel.Init(NO_SHADOW_MODEL_PATH, nullptr, 0, enModelUpAxisZ, false);
	m_noShadowModel.SetShadowCasterFlag(false);
	m_noShadowModel.Update();

	m_collisionModel.Init(HALL_COLLISION_PATH, nullptr, 0, enModelUpAxisZ, false);
	m_collisionModel.Update();
	m_physicsStaticObject.CreateFromModel(m_collisionModel.GetModel(), m_collisionModel.GetModel().GetWorldMatrix());
	m_physicsStaticObject.SetFriction(1.0f);

	for (float z : PILLAR_ZS) {
		AddPointLight({ -CANDELABRUM_X, CANDLE_HEIGHT, z }, PILLAR_CANDLE_COLOR, PILLAR_CANDLE_RANGE, CANDLE_FLICKER);
		AddPointLight({ CANDELABRUM_X, CANDLE_HEIGHT, z }, PILLAR_CANDLE_COLOR, PILLAR_CANDLE_RANGE, CANDLE_FLICKER);
	}
	for (const auto& position : DAIS_CANDLE_POSITIONS) {
		AddPointLight(position, DAIS_CANDLE_COLOR, DAIS_CANDLE_RANGE, CANDLE_FLICKER);
	}
	for (const auto& position : SCONCE_LIGHT_POSITIONS) {
		AddPointLight(position, SCONCE_LIGHT_COLOR, SCONCE_LIGHT_RANGE, CANDLE_FLICKER);
	}
	for (float height : CANDELABRUM_FLAME_LIGHT_HEIGHTS) {
		for (float z : PILLAR_ZS) {
			AddPointLight({ -CANDELABRUM_X, height, z }, FLAME_LIGHT_COLOR, FLAME_LIGHT_RANGE, FLAME_FLICKER);
			AddPointLight({ CANDELABRUM_X, height, z }, FLAME_LIGHT_COLOR, FLAME_LIGHT_RANGE, FLAME_FLICKER);
		}
	}
	for (float height : DAIS_FLAME_LIGHT_HEIGHTS) {
		for (const auto& candle : DAIS_CANDLE_POSITIONS) {
			AddPointLight({ candle.x, height, candle.z }, FLAME_LIGHT_COLOR, FLAME_LIGHT_RANGE, FLAME_FLICKER);
		}
	}
	for (const auto& sconce : SCONCE_LIGHT_POSITIONS) {
		const float z = sconce.z < 0.0f ? -SCONCE_FLAME_LIGHT_Z : SCONCE_FLAME_LIGHT_Z;
		AddPointLight({ sconce.x, SCONCE_FLAME_LIGHT_HEIGHT, z }, FLAME_LIGHT_COLOR, FLAME_LIGHT_RANGE, FLAME_FLICKER);
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

void BossStage::AddPointLight(const Vector3& position, const Vector3& color, float range, float flicker)
{
	auto light = std::make_unique<nsK2Engine::PointLight>();
	light->Init();
	light->SetPosition(position);
	light->SetColor(color);
	light->SetRange(range);
	light->SetAffectPowParam(LIGHT_AFFECT_POW);
	if (flicker > 0.0f) {
		// 揺れの始まりを位置からずらして、どのろうそくも同じ拍子で揺れないようにする。
		const float phase = position.x * 0.013f + position.z * 0.007f;
		m_flickers.push_back({ light.get(), color, phase, flicker });
	}
	m_lights.push_back(std::move(light));
}

void BossStage::UpdateLights()
{
	m_time += g_gameTime->GetFrameDeltaTime();
	for (const auto& flicker : m_flickers) {
		const float wave = FLICKER_AMOUNT_1 * sinf(m_time * FLICKER_SPEED_1 + flicker.phase)
			+ FLICKER_AMOUNT_2 * sinf(m_time * FLICKER_SPEED_2 + flicker.phase * 2.3f);
		flicker.light->SetColor(flicker.baseColor * (1.0f + flicker.amount * wave));
	}
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
	m_noShadowModel.Draw(rc);
}

Vector3 BossStage::GetPlayerStartPosition()
{
	return PLAYER_START_POSITION;
}

Vector3 BossStage::GetEnemyStartPosition()
{
	return ENEMY_START_POSITION;
}
