#pragma once

class Player : public IGameObject
{
public:
	enum EnAnimClip {
		enAnimClip_Idle,
		enAnimClip_Walk,
		enAnimClip_Run,
		enAnimClip_Jump,
		enAnimClip_Num,
	};

	Player();
	~Player();
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

	Vector3 GetForward() const;
	void SetRenderBody(bool isRenderBody)
	{
		m_isRenderBody = isRenderBody;
	}

private:
	void Move();
	void UpdateAnimation();
	void InitSword();
	void UpdateSword();
	bool IsRushSkillKeyTrigger();
	void StartRushSkill(const Vector3& direction);
	void RespawnIfNeeded();
	void Respawn();

	AnimationClip m_animClips[enAnimClip_Num];
	ModelRender m_modelRender;
	ModelRender m_swordModelRender;
	CharacterController m_charaCon;
	Vector3 m_position = Vector3::Zero;
	Vector3 m_respawnPosition = Vector3::Zero;
	Vector3 m_moveSpeed = Vector3::Zero;
	Vector3 m_rushDirection = Vector3::AxisZ;
	Quaternion m_rotation = Quaternion::Identity;
	float m_rushRemainingDistance = 0.0f;
	int m_rightHandBoneNo = -1;
	int m_currentAnimNo = enAnimClip_Idle;
	bool m_isRushKeyPressed = false;
	bool m_isRushPoseActive = false;
	bool m_isCharaConReady = false;
	bool m_isRenderBody = true;
};
