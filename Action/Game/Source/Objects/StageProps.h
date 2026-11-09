#pragma once

#include <memory>
#include <vector>

#include "Objects/StageRender/PropTypes.h"
#include "Objects/StageRender/IPropFilter.h"
#include "Objects/StageRender/ISpatialIndex.h"
#include "Objects/StageRender/LodSelector.h"

class StageRenderSettings;

// ステージに置く岩・木・草などの部品。
// 部品の種類ごとに1つの ModelRender を作り、インスタンス描画（同じモデルをまとめて1回で描く）する。
// 毎フレーム、描く物を次の順に選んで、選んだ物の行列だけを渡す。どの段も別のクラスで、ここは順に呼ぶだけ。
//   1. 区画の分け方（ISpatialIndex）で、描く距離の内側で画面にかかるマスの部品を集める
//   2. フィルター（IPropFilter）を設定の順に通して、要らない物を消す
//   3. LOD の振り分け係（LodSelector）で、距離ごとの細かさの段に分ける
//   4. 段ごとの ModelRender に、前もって計算した行列を渡す
class StageProps
{
public:
	// 部品1種類の情報。配置 JSON の models の1つ。
	struct ModelDesc
	{
		std::string path;
		std::string kind;            // tree / rock / detail / grass
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

	// models: 部品の種類、itemsByModel[部品の番号]: 置く場所、settings: 描き方の設定。
	void Init(
		const std::vector<ModelDesc>& models,
		const std::vector<std::vector<Item>>& itemsByModel,
		const StageRenderSettings& settings);
	// 描く物を選んで、インスタンスの情報を渡す。Render より前に呼ぶ。
	void Update();
	void Render(RenderContext& rc);

private:
	struct ModelEntry
	{
		ModelDesc desc;
		PropModelInfo info;
		std::vector<std::unique_ptr<ModelRender>> lodRenders;  // LOD の段ごとのモデル。0 番目がいちばん細かい
		std::vector<int> numInstances;            // 段ごとの、このフレームに渡した数
		std::vector<PropInstance> instances;      // 置いた物の全部。Init のあとは増やさない（ポインターで覚えるため）
		std::unique_ptr<ISpatialIndex> spatialIndex;
		std::unique_ptr<LodSelector> lodSelector;
	};
	struct TrunkCollision
	{
		CCapsuleCollider collider;
		RigidBody rigidBody;
	};

	void CreateCollision(ModelEntry& entry, const Item& item, const Matrix& world);

	std::vector<std::unique_ptr<ModelEntry>> m_models;
	std::vector<std::unique_ptr<IPropFilter>> m_filters;
	PropList m_candidates;   // 毎フレーム使い回す候補の一覧
	std::vector<PropList> m_lodLists;  // 毎フレーム使い回す、LOD の段ごとの一覧
	std::vector<std::unique_ptr<PhysicsStaticObject>> m_meshCollisions;
	std::vector<std::unique_ptr<TrunkCollision>> m_trunkCollisions;
};
