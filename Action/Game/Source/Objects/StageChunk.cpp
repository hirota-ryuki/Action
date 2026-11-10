#include "stdafx.h"
#include "StageChunk.h"

namespace
{
	// カメラから区画の真ん中までがこの距離より遠いと、遠い用のモデルで描く（cm）。
	constexpr float FAR_MODEL_DISTANCE = 15000.0f;
}

void StageChunk::Init(const char* model, const char* farModel, const Vector3& center)
{
	m_center = center;

	// 地形は Z が上の FBX なので enModelUpAxisZ で読む。地面は影を受けるが、影は落とさない。
	m_nearModel.Init(model, nullptr, 0, enModelUpAxisZ, true);
	m_nearModel.SetShadowCasterFlag(false);
	m_nearModel.Update();

	m_farModel.Init(farModel, nullptr, 0, enModelUpAxisZ, true);
	m_farModel.SetShadowCasterFlag(false);
	m_farModel.Update();

	// 当たり判定は近い用の形で作る。
	m_physicsStaticObject.CreateFromModel(m_nearModel.GetModel(), m_nearModel.GetModel().GetWorldMatrix());
	m_physicsStaticObject.SetFriction(1.0f);
}

void StageChunk::Render(RenderContext& rc)
{
	// 高さは見ずに、水平の距離で決める（崖の上から見下ろしても近い用で描くように）。
	Vector3 toChunk = m_center - g_camera3D->GetPosition();
	toChunk.y = 0.0f;
	if (toChunk.LengthSq() > FAR_MODEL_DISTANCE * FAR_MODEL_DISTANCE) {
		m_farModel.Draw(rc);
	}
	else {
		m_nearModel.Draw(rc);
	}
}
