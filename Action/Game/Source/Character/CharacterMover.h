#pragma once

struct CharacterInput;

// キャラクターの移動の設定。
struct CharacterMoveSettings
{
	float walkSpeed = 300.0f;
	float runSpeedMultiplier = 1.0f;  // 走る入力中の速度倍率。1なら走れない。
	float jumpSpeed = 0.0f;           // 0ならジャンプできない。
	float gravity = 980.0f;
	float rushDistance = 0.0f;        // ラッシュの総移動距離。0ならラッシュできない。
	float rushSpeed = 1500.0f;
	float capsuleRadius = 20.0f;
	float capsuleHeight = 100.0f;
};

// CharacterController を包み、入力から移動・ジャンプ・ラッシュ・重力・向きを処理する。
// パッドやカメラは触らず、CharacterInput だけを見て動く。
class CharacterMover
{
public:
	// SetPosition() で決めた位置にカプセルを作る。
	void Init(const CharacterMoveSettings& settings);
	void Update(const CharacterInput& input, float deltaTime);

	// 速度とラッシュを打ち消して止める（凍結・リスポーン時など）。
	void Stop();
	// ラッシュだけを取り消す（凍結中など。落下の速度は残す）。
	void CancelRush()
	{
		m_rushRemainingDistance = 0.0f;
	}

	// Init() の前に呼んでもよい。
	void SetPosition(const Vector3& position);
	const Vector3& GetPosition() const
	{
		return m_position;
	}
	void SetRotation(const Quaternion& rotation)
	{
		m_rotation = rotation;
	}
	const Quaternion& GetRotation() const
	{
		return m_rotation;
	}
	Vector3 GetForward() const;

	bool IsOnGround() const;
	bool IsRushing() const
	{
		return m_rushRemainingDistance > 0.0f;
	}
	// 水平方向に動いているか。アニメーションの切り替えに使う。
	bool IsMoving() const
	{
		return m_isMoving;
	}

private:
	void StartRush(const Vector3& direction);

	CharacterMoveSettings m_settings;
	CharacterController m_charaCon;
	Vector3 m_position = Vector3::Zero;
	Vector3 m_moveSpeed = Vector3::Zero;
	Vector3 m_rushDirection = Vector3::AxisZ;   // ラッシュ中の移動方向（正規化済み）。
	Quaternion m_rotation = Quaternion::Identity;
	float m_rushRemainingDistance = 0.0f;       // 0より大きい間ラッシュ中と判定する。
	bool m_isMoving = false;
	bool m_isInitialized = false;
};
