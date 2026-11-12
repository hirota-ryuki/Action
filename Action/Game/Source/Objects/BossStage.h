#pragma once

#include <memory>
#include <vector>

namespace nsK2Engine {
	class PointLight;
}

// ボス戦の広間（月明かりの玉座の間）。
// 見た目のモデル（BossHall.fbx）と、当たり判定用の単純な形（BossHallCollision.fbx）を読む。
// どちらも tools/BossStage/make_boss_stage.py が書き出す。
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
	// 燭台のろうそくの光を1つ足す。
	void AddCandleLight(const Vector3& position, const Vector3& color, float range);

	ModelRender m_model;
	ModelRender m_collisionModel;   // 当たり判定を作るためだけに読む（描かない）。
	PhysicsStaticObject m_physicsStaticObject;
	std::vector<std::unique_ptr<nsK2Engine::PointLight>> m_lights;
};
