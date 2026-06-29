#pragma once

class Player;

class Enemy : public IGameObject
{
public:
	enum EnAnimClip {
		enAnimClip_Idle,
		enAnimClip_Run,
		enAnimClip_Num,
	};

	Enemy();
	~Enemy();
	bool Start();
	void Update();
	void Render(RenderContext& rc);

	void SetPosition(const Vector3& position)
	{
		m_position = position;
		if (m_isCharaConReady) {
			m_charaCon.SetPosition(position);
		}
	}

	const Vector3& GetPosition() const
	{
		return m_position;
	}

private:
	void Move();
	void UpdateAnimation();

	Player* m_player = nullptr;
	AnimationClip m_animClips[enAnimClip_Num];
	ModelRender m_modelRender;
	CharacterController m_charaCon;
	Vector3 m_position = Vector3::Zero;
	Vector3 m_moveSpeed = Vector3::Zero;
	Vector3 m_scale = Vector3(1.08f, 1.08f, 1.08f);
	Quaternion m_rotation = Quaternion::Identity;
	int m_currentAnimNo = enAnimClip_Idle;
	bool m_isCharaConReady = false;
	bool m_isChasing = false;
};
