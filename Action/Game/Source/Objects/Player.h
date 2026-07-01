#pragma once

// プレイヤーキャラクターを管理するクラス。
// 移動・ジャンプ・ラッシュスキル・剣のボーン追従・リスポーンを担う。
class Player : public IGameObject
{
public:
	// 再生するアニメーションの種類。
	enum EnAnimClip {
		enAnimClip_Idle,
		/*enAnimClip_Walk,
		enAnimClip_Run,
		enAnimClip_Jump,*/
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
	Vector3 m_respawnPosition = Vector3::Zero;  // Start()時の位置を記録。落下後の復帰先。
	Vector3 m_moveSpeed = Vector3::Zero;
	Vector3 m_rushDirection = Vector3::AxisZ;   // ラッシュ中の移動方向（正規化済み）。
	Quaternion m_rotation = Quaternion::Identity;
	float m_rushRemainingDistance = 0.0f;       // 0より大きい間ラッシュ中と判定する。
	int m_rightHandBoneNo = -1;                 // -1は右手ボーンが見つからなかった状態。
	int m_currentAnimNo = enAnimClip_Idle;
	bool m_isRushKeyPressed = false;            // 前フレームのLキー状態。トリガー判定に使用。
	bool m_isRushPoseActive = false;            // ラッシュ中にアニメを停止しているか。
	bool m_isCharaConReady = false;             // CharacterControllerの初期化が完了しているか。
	bool m_isRenderBody = true;                 // 一人称カメラ時はfalseにして体を非表示にする。
};
