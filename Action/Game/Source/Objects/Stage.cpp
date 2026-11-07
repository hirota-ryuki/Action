#include "stdafx.h"
#include "Stage.h"

namespace
{
	const char* STAGE_MODEL_PATH = "Assets/modelData/stage/kariAsiba.tkm";
}

Stage::Stage()
{

}

Stage::~Stage()
{

}

bool Stage::Start()
{
	m_modelRender.Init(STAGE_MODEL_PATH);
	m_modelRender.SetShadowCasterFlag(false);  // ステージ自体は影を落とさない。
	m_modelRender.SetTRS(m_position, m_rotation, m_scale);
	m_modelRender.Update();

	// モデルのメッシュ形状をそのまま物理コリジョンとして使用する。
	m_physicsStaticObject.CreateFromModel(m_modelRender.GetModel(), m_modelRender.GetModel().GetWorldMatrix());
	m_physicsStaticObject.SetFriction(1.0f);

	return true;
}

void Stage::Update()
{
	m_modelRender.Update();
}

void Stage::Render(RenderContext& rc)
{
	m_modelRender.Draw(rc);
}
