#include "stdafx.h"
#include "StageProps.h"

#include <cmath>

#include "Objects/StageRender/StageRenderSettings.h"
#include "Objects/StageRender/CullContextBuilder.h"
#include "tkFile/ModelReducer/ModelReducer.h"

namespace
{
	// モデルの原点（足もと）から、いちばん遠い頂点までの距離。部品をまるごと包む球の半径に使う。
	float CalcModelRadius(Model& model)
	{
		float maxLengthSq = 0.0f;
		model.GetTkmFile().QueryMeshParts([&](const TkmFile::SMesh& mesh) {
			for (const auto& v : mesh.vertexBuffer) {
				maxLengthSq = (std::max)(maxLengthSq, v.pos.LengthSq());
			}
		});
		return std::sqrt(maxLengthSq);
	}

	// 番号から、0〜1 の決まった乱数を作る（間引きで毎フレーム同じ物を残すため）。
	float MakeKeepValue(uint32_t index)
	{
		uint32_t h = index * 2654435761u;
		h ^= h >> 16;
		return static_cast<float>(h & 0xFFFF) / 65536.0f;
	}
}

void StageProps::Init(
	const std::vector<ModelDesc>& models,
	const std::vector<std::vector<Item>>& itemsByModel,
	const StageRenderSettings& settings)
{
	m_filters = settings.CreateFilters();

	for (int modelNo = 0; modelNo < static_cast<int>(models.size()); modelNo++) {
		const auto& items = itemsByModel[modelNo];
		auto entry = std::make_unique<ModelEntry>();
		entry->desc = models[modelNo];
		entry->info.kind = entry->desc.kind;
		entry->info.drawDistance = entry->desc.drawDistance;
		if (items.empty()) {
			m_models.push_back(std::move(entry));
			continue;
		}
		// LOD のモデルを用意する。設定がない種類や、作れなかったときは、もとのモデル1つで描く。
		std::vector<std::string> modelPaths;
		const auto levels = settings.GetLodLevels(entry->desc.kind);
		if (!levels.empty()) {
			modelPaths = nsK2EngineLow::ModelReducer::ResolveLodTkmFilePaths(entry->desc.path.c_str(), levels);
		}
		if (modelPaths.empty()) {
			modelPaths = { entry->desc.path };
			entry->lodSelector = std::make_unique<LodSelector>(std::vector<float>{ 0.0f });
		}
		else {
			entry->lodSelector = std::make_unique<LodSelector>(settings.CreateLodSelector(entry->desc.kind));
		}

		// エンジンは最大数が 2 以上のときだけインスタンス描画にするので、1個しかない部品でも 2 にする。
		// どの物がどの段になるかはフレームごとに変わるので、どの段も全部入る数にしておく。
		const int maxInstance = (std::max)(static_cast<int>(items.size()), 2);
		for (const auto& path : modelPaths) {
			auto modelRender = std::make_unique<ModelRender>();
			modelRender->Init(path.c_str(), nullptr, 0, enModelUpAxisZ, true, maxInstance);
			modelRender->SetShadowCasterFlag(entry->desc.isShadowCaster);
			entry->lodRenders.push_back(std::move(modelRender));
		}
		entry->numInstances.assign(entry->lodRenders.size(), 0);
		ModelRender& baseRender = *entry->lodRenders[0];

		// 動かないので、行列・包む球・間引き用の乱数を最初に計算しておく。
		const float modelRadius = CalcModelRadius(baseRender.GetModel());
		entry->instances.reserve(items.size());
		for (const Item& item : items) {
			PropInstance instance;
			instance.position = item.position;
			instance.radius = modelRadius * item.scale.x;
			instance.world = baseRender.CalcWorldMatrix(item.position, item.rotation, item.scale);
			instance.keepValue = MakeKeepValue(static_cast<uint32_t>(entry->instances.size()));
			entry->instances.push_back(instance);
			CreateCollision(*entry, item, instance.world);
		}
		entry->spatialIndex = settings.CreateSpatialIndex();
		entry->spatialIndex->Build(entry->instances);
		m_models.push_back(std::move(entry));
	}
}

void StageProps::CreateCollision(ModelEntry& entry, const Item& item, const Matrix& world)
{
	switch (entry.desc.collision) {
	case ModelDesc::Collision::Mesh: {
		// 岩はモデルの形のまま。
		auto obj = std::make_unique<PhysicsStaticObject>();
		obj->CreateFromModel(entry.lodRenders[0]->GetModel(), world);
		obj->SetFriction(1.0f);
		m_meshCollisions.push_back(std::move(obj));
		break;
	}
	case ModelDesc::Collision::Trunk: {
		// 木は幹だけを円柱（カプセル）で止める。葉の中は通り抜けてよい。
		const float radius = entry.desc.trunkRadius * item.scale.x;
		const float height = entry.desc.trunkHeight * item.scale.y;
		auto trunk = std::make_unique<TrunkCollision>();
		trunk->collider.Init(radius, height);
		RigidBodyInitData init;
		init.collider = &trunk->collider;
		init.pos = item.position;
		init.pos.y += height * 0.5f;
		init.mass = 0.0f;
		trunk->rigidBody.Init(init);
		m_trunkCollisions.push_back(std::move(trunk));
		break;
	}
	case ModelDesc::Collision::None:
		break;
	}
}

void StageProps::Update()
{
	PropCullContext context = CullContextBuilder::Build(*g_camera3D);
	for (auto& entry : m_models) {
		entry->numInstances.assign(entry->lodRenders.size(), 0);
		if (!entry->spatialIndex) {
			continue;
		}
		context.model = &entry->info;

		// 1. 区画の分け方で候補を集める。
		m_candidates.clear();
		entry->spatialIndex->Query(context, m_candidates);
		// 2. フィルターを順に通す。
		for (const auto& filter : m_filters) {
			filter->Apply(m_candidates, context);
		}
		// 3. LOD の段ごとに分ける。
		entry->lodSelector->Split(m_candidates, context, m_lodLists);
		// 4. 段ごとに、行列を番号 0 から詰めて渡す。渡した数だけ描かれる。
		for (size_t lod = 0; lod < entry->lodRenders.size() && lod < m_lodLists.size(); lod++) {
			for (const PropInstance* instance : m_lodLists[lod]) {
				entry->lodRenders[lod]->UpdateInstancingData(entry->numInstances[lod], instance->world);
				entry->numInstances[lod]++;
			}
		}
	}
}

void StageProps::Render(RenderContext& rc)
{
	for (auto& entry : m_models) {
		for (size_t lod = 0; lod < entry->lodRenders.size(); lod++) {
			if (entry->numInstances[lod] > 0) {
				entry->lodRenders[lod]->Draw(rc);
			}
		}
	}
}
