#pragma once

#include <memory>
#include <vector>

namespace nsK2Engine {
	class PointLight;
	class SpotLight;
	class VolumeSpotLight;
}

// ボス戦の広間（月明かりの玉座の間）。
// 見た目のモデル（BossHall.fbx・天井の BossHallCeiling.fbx）と、当たり判定用の単純な形（BossHallCollision.fbx）を読む。
// どれも tools/BossStage/make_boss_stage.py が書き出す。
class BossStage : public IGameObject
{
public:
	BossStage();
	~BossStage();
	bool Start();
	void Update();
	void Render(RenderContext& rc);

	// プレイヤー・黒騎士を置く場所（cm）。
	static Vector3 GetPlayerStartPosition();
	static Vector3 GetEnemyStartPosition();

private:
	// 点の光を1つ足す。
	void AddPointLight(const Vector3& position, const Vector3& color, float range);
	// ライトのカメラから見た位置を計算し直す。毎フレーム呼ぶ。
	void UpdateLights();

	ModelRender m_model;
	ModelRender m_ceilingModel;     // 天井。影を落とさない。
	ModelRender m_collisionModel;   // 当たり判定を作るためだけに読む（描かない）。
	PhysicsStaticObject m_physicsStaticObject;
	std::vector<std::unique_ptr<nsK2Engine::PointLight>> m_lights;
	std::vector<std::unique_ptr<nsK2Engine::SpotLight>> m_spotLights;               // 光の筋の元になるスポットライト。
	std::vector<std::unique_ptr<nsK2Engine::VolumeSpotLight>> m_volumeSpotLights;   // スポットライトを参照するので、後に宣言して先に消す。
};
