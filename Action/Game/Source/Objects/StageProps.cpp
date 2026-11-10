#include "stdafx.h"
#include "StageProps.h"

void StageProps::Init(
	const std::vector<ModelDesc>& models,
	std::vector<std::vector<std::vector<Item>>>&& itemsByChunk,
	const std::vector<Vector3>& chunkCenters,
	float chunkSize)
{
	m_itemsByChunk = std::move(itemsByChunk);
	m_chunkCenters = chunkCenters;
	m_chunkRadius = chunkSize * 0.5f * 1.4143f;

	for (int modelNo = 0; modelNo < static_cast<int>(models.size()); modelNo++) {
		// インスタンスの最大数は、全区画に置いたこの部品の数。
		int maxInstance = 0;
		for (const auto& chunk : m_itemsByChunk) {
			maxInstance += static_cast<int>(chunk[modelNo].size());
		}

		auto entry = std::make_unique<ModelEntry>();
		entry->desc = models[modelNo];
		if (maxInstance > 0) {
			// エンジンは最大数が 2 以上のときだけインスタンス描画にするので、1個しかない部品でも 2 にする。
			maxInstance = (std::max)(maxInstance, 2);
			entry->modelRender.Init(entry->desc.path.c_str(), nullptr, 0, enModelUpAxisZ, true, maxInstance);
			entry->modelRender.SetShadowCasterFlag(entry->desc.isShadowCaster);
		}
		m_models.push_back(std::move(entry));

		// 当たり判定は動かないので、最初に全部作る。
		for (const auto& chunk : m_itemsByChunk) {
			for (const Item& item : chunk[modelNo]) {
				CreateCollision(modelNo, item);
			}
		}
	}
}

void StageProps::CreateCollision(int modelNo, const Item& item)
{
	ModelEntry& entry = *m_models[modelNo];
	switch (entry.desc.collision) {
	case ModelDesc::Collision::Mesh: {
		// 岩はモデルの形のまま。インスタンス描画のモデルには置き場所の行列がないので、ここで計算して渡す。
		Model& model = entry.modelRender.GetModel();
		Matrix world = model.CalcWorldMatrix(item.position, item.rotation, item.scale);
		auto obj = std::make_unique<PhysicsStaticObject>();
		obj->CreateFromModel(model, world);
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
	const Vector3 cameraPos = g_camera3D->GetPosition();
	Vector3 forward = g_camera3D->GetForward();
	forward.y = 0.0f;
	forward.Normalize();

	for (auto& entry : m_models) {
		entry->numInstance = 0;
	}

	for (int chunkNo = 0; chunkNo < static_cast<int>(m_chunkCenters.size()); chunkNo++) {
		Vector3 toChunk = m_chunkCenters[chunkNo] - cameraPos;
		toChunk.y = 0.0f;
		// 区画まるごとカメラの後ろにあれば飛ばす。
		if (toChunk.Dot(forward) < -m_chunkRadius) {
			continue;
		}
		const float chunkDistance = toChunk.Length() - m_chunkRadius;

		for (int modelNo = 0; modelNo < static_cast<int>(m_models.size()); modelNo++) {
			ModelEntry& entry = *m_models[modelNo];
			const float drawDistance = entry.desc.drawDistance;
			// 区画のいちばん近いところでも描く距離より遠ければ飛ばす。
			if (chunkDistance > drawDistance) {
				continue;
			}
			const float drawDistanceSq = drawDistance * drawDistance;
			for (const Item& item : m_itemsByChunk[chunkNo][modelNo]) {
				Vector3 diff = item.position - cameraPos;
				if (diff.LengthSq() > drawDistanceSq) {
					continue;
				}
				// 渡した数だけ描かれるので、番号は 0 から詰めて渡す。
				entry.modelRender.UpdateInstancingData(entry.numInstance, item.position, item.rotation, item.scale);
				entry.numInstance++;
			}
		}
	}
}

void StageProps::Render(RenderContext& rc)
{
	for (auto& entry : m_models) {
		if (entry->numInstance > 0) {
			entry->modelRender.Draw(rc);
		}
	}
}
