#pragma once

#include <memory>
#include <vector>

// ステージに置く岩・木・草などの部品。
// 部品の種類ごとに1つの ModelRender を作り、インスタンス描画（同じモデルをまとめて1回で描く）する。
// エンジンのインスタンス描画は画面の外の物も描くので、毎フレーム「描く区画にあって、描く距離の内側の物」だけを渡す。
class StageProps
{
public:
	// 部品1種類の情報。配置 JSON の models の1つ。
	struct ModelDesc
	{
		std::string path;
		bool isShadowCaster = false;
		float drawDistance = 0.0f;   // カメラからこの距離より遠い物は描かない（cm）
		enum class Collision { None, Mesh, Trunk } collision = Collision::None;
		float trunkRadius = 0.0f;    // 幹の当たり判定の半径（cm）
		float trunkHeight = 0.0f;    // 幹の当たり判定の高さ（cm）
	};
	// 部品を1つ置く場所。
	struct Item
	{
		Vector3 position;
		Quaternion rotation;
		Vector3 scale;
	};

	// models: 部品の種類、itemsByChunk[区画番号][部品の番号]: 置く場所、chunkCenters: 区画の真ん中、chunkSize: 区画の幅（cm）。
	void Init(
		const std::vector<ModelDesc>& models,
		std::vector<std::vector<std::vector<Item>>>&& itemsByChunk,
		const std::vector<Vector3>& chunkCenters,
		float chunkSize);
	// 描く物を選んで、インスタンスの情報を渡す。Render より前に呼ぶ。
	void Update();
	void Render(RenderContext& rc);

private:
	void CreateCollision(int modelNo, const Item& item);

	struct ModelEntry
	{
		ModelDesc desc;
		ModelRender modelRender;
		int numInstance = 0;   // このフレームに渡した数
	};
	struct TrunkCollision
	{
		CCapsuleCollider collider;
		RigidBody rigidBody;
	};

	std::vector<std::unique_ptr<ModelEntry>> m_models;
	std::vector<std::vector<std::vector<Item>>> m_itemsByChunk;
	std::vector<Vector3> m_chunkCenters;
	float m_chunkRadius = 0.0f;   // 区画の真ん中から角までの水平の距離
	std::vector<std::unique_ptr<PhysicsStaticObject>> m_meshCollisions;
	std::vector<std::unique_ptr<TrunkCollision>> m_trunkCollisions;
};
