#include "stdafx.h"
#include "Stage.h"

namespace
{
	const char* kStageModelPath = "Assets/modelData/stage/kariAsiba.tkm";
}

Stage::Stage()
{

}

Stage::~Stage()
{

}

bool Stage::Start()
{
	m_modelRender.Init(kStageModelPath);
	m_modelRender.SetShadowCasterFlag(false);
	m_modelRender.SetTRS(m_position, m_rotation, m_scale);
	m_modelRender.Update();

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
