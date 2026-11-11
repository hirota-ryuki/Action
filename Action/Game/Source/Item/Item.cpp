#include "stdafx.h"
#include "Item.h"

namespace
{
	// 地面を探すレイの長さ（置いた位置の上下）。
	constexpr float GROUND_RAY_LENGTH = 2000.0f;
	constexpr float PI = 3.14159265f;
}

bool Item::Start()
{
	m_look = MakeLook();
	m_model.Init(m_look.modelPath.c_str(), nullptr, 0, m_look.upAxis);
	m_model.SetScale(Vector3(m_look.modelScale, m_look.modelScale, m_look.modelScale));
	SnapToGround();
	return true;
}

void Item::SnapToGround()
{
	PhysicsWorld* physicsWorld = PhysicsWorld::GetInstance();
	if (physicsWorld == nullptr) {
		return;
	}
	const Vector3 up = { 0.0f, GROUND_RAY_LENGTH, 0.0f };
	Vector3 hitPosition;
	if (physicsWorld->RayTest(m_transform.position + up, m_transform.position - up, hitPosition)) {
		m_transform.position = hitPosition;
	}
}

void Item::Update()
{
	m_time += g_gameTime->GetFrameDeltaTime();

	float wave = 0.0f;
	if (m_look.floatSeconds > 0.0f) {
		wave = sinf(m_time / m_look.floatSeconds * 2.0f * PI);
	}
	Vector3 modelPosition = m_transform.position;
	modelPosition.y += m_look.floatHeight + m_look.floatAmplitude * wave;

	Quaternion rotation;
	rotation.SetRotationDegY(fmodf(m_time * m_look.spinDegPerSecond, 360.0f));

	m_model.SetPosition(modelPosition);
	m_model.SetRotation(rotation);
	m_model.Update();
}

void Item::Render(RenderContext& rc)
{
	m_model.Draw(rc);
}
