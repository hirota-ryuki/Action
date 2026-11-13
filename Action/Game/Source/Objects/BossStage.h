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
	// 点の光を1つ足す。flicker が 0 より大きいと、その割合で明るさを揺らす（ろうそく）。
	void AddPointLight(const Vector3& position, const Vector3& color, float range, float flicker = 0.0f);
	// 炎の揺らめきを進め、ライトのカメラから見た位置を計算し直す。毎フレーム呼ぶ。
	void UpdateLights();

	// 明るさを揺らすライト。
	struct Flicker {
		nsK2Engine::PointLight* light;
		Vector3 baseColor;
		float phase;    // 揺れの始まりのずれ（ラジアン）
		float amount;   // 揺れ幅（明るさの割合）
	};

	ModelRender m_model;
	ModelRender m_noShadowModel;    // 天井と炎。影を落とさない。
	ModelRender m_collisionModel;   // 当たり判定を作るためだけに読む（描かない）。
	PhysicsStaticObject m_physicsStaticObject;
	std::vector<std::unique_ptr<nsK2Engine::PointLight>> m_lights;
	std::vector<Flicker> m_flickers;
	float m_time = 0.0f;            // 揺らめきの時間（秒）
	std::vector<std::unique_ptr<nsK2Engine::SpotLight>> m_spotLights;               // 光の筋の元になるスポットライト。
	std::vector<std::unique_ptr<nsK2Engine::VolumeSpotLight>> m_volumeSpotLights;   // スポットライトを参照するので、後に宣言して先に消す。
};
