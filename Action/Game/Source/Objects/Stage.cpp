#include "stdafx.h"
#include "Stage.h"

#include "System/JsonFile.h"
#include "Objects/StageRender/StageRenderSettings.h"

namespace
{
	const char* STAGE_JSON_PATH = "stage/field_props.json";
	const char* RENDER_SETTINGS_JSON_PATH = "stage/render_settings.json";

	Vector3 ToVector3(const JsonFile::Json& a)
	{
		return Vector3(a[0].get<float>(), a[1].get<float>(), a[2].get<float>());
	}

	StageProps::ModelDesc::Collision ToCollision(const std::string& s)
	{
		if (s == "mesh") {
			return StageProps::ModelDesc::Collision::Mesh;
		}
		if (s == "trunk") {
			return StageProps::ModelDesc::Collision::Trunk;
		}
		return StageProps::ModelDesc::Collision::None;
	}
}

Stage::Stage()
{

}

Stage::~Stage()
{

}

bool Stage::Start()
{
	JsonFile::Json json;
	if (!JsonFile::TryLoad(STAGE_JSON_PATH, json)) {
		return true;
	}

	// 地形の区画。
	for (const auto& c : json["chunks"]) {
		auto chunk = std::make_unique<StageChunk>();
		chunk->Init(c.value("model", std::string()).c_str(), c.value("far", std::string()).c_str(), ToVector3(c["center"]));
		m_chunks.push_back(std::move(chunk));
	}

	// 部品の種類。
	std::vector<StageProps::ModelDesc> models;
	for (const auto& m : json["models"]) {
		StageProps::ModelDesc desc;
		desc.path = m.value("path", std::string());
		desc.kind = m.value("kind", std::string());
		desc.isShadowCaster = m.value("shadow", false);
		desc.drawDistance = m.value("drawDistance", 0.0f);
		desc.collision = ToCollision(m.value("collision", std::string("none")));
		desc.trunkRadius = m.value("trunkRadius", 0.0f);
		desc.trunkHeight = m.value("trunkHeight", 0.0f);
		models.push_back(desc);
	}

	// 置き場所。items の1つは [x, y, z, 向き（Y 軸まわりの度）, 大きさ, 傾き X, 傾き Z]。
	// 描くときは区画の分け方（ISpatialIndex）が場所で分け直すので、ここでは部品の種類ごとにまとめるだけ。
	std::vector<std::vector<StageProps::Item>> itemsByModel(models.size());
	for (const auto& p : json["props"]) {
		const int modelNo = p.value("model", 0);
		auto& items = itemsByModel[modelNo];
		for (const auto& it : p["items"]) {
			StageProps::Item item;
			item.position = Vector3(it[0].get<float>(), it[1].get<float>(), it[2].get<float>());
			Quaternion tiltX, tiltZ, yaw;
			tiltX.SetRotationDegX(it[5].get<float>());
			tiltZ.SetRotationDegZ(it[6].get<float>());
			yaw.SetRotationDegY(it[3].get<float>());
			// 少し傾けてから、Y 軸まわりに回す。
			item.rotation = tiltX;
			item.rotation.Multiply(tiltZ);
			item.rotation.Multiply(yaw);
			const float scale = it[4].get<float>();
			item.scale = Vector3(scale, scale, scale);
			items.push_back(item);
		}
	}

	const StageRenderSettings renderSettings = StageRenderSettings::Load(RENDER_SETTINGS_JSON_PATH);
	m_props.Init(models, itemsByModel, renderSettings);
	return true;
}

void Stage::Update()
{
	m_props.Update();
}

void Stage::Render(RenderContext& rc)
{
	for (auto& chunk : m_chunks) {
		chunk->Render(rc);
	}
	m_props.Render(rc);
}
