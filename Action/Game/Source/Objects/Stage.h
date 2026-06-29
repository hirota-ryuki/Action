#pragma once

class Stage : public IGameObject
{
public:
	Stage();
	~Stage();
	bool Start();
	void Update();
	void Render(RenderContext& rc);

	void SetPosition(const Vector3& position)
	{
		m_position = position;
	}

	void SetScale(const Vector3& scale)
	{
		m_scale = scale;
	}

	void SetRotation(const Quaternion& rotation)
	{
		m_rotation = rotation;
	}

private:
	Vector3 m_position = Vector3::Zero;
	Vector3 m_scale = Vector3::One;
	Quaternion m_rotation = Quaternion::Identity;
	ModelRender m_modelRender;
	PhysicsStaticObject m_physicsStaticObject;
};
