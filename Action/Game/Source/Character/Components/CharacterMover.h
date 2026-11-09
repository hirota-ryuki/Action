#pragma once

struct CharacterInput;

// キャラクターの移動の設定。
struct CharacterMoveSettings
{
	float walkSpeed = 300.0f;
	float runSpeedMultiplier = 1.0f;  // 走る入力中の速度倍率。1なら走れない。
	float jumpSpeed = 0.0f;           // 0ならジャンプできない。
	float gravity = 980.0f;
	float capsuleRadius = 20.0f;
	float capsuleHeight = 100.0f;
	float turnSpeedDeg = 0.0f;        // 見た目の向きを変える速さの上限（度/秒）。0ならすぐその方向を向く。
	float turnStiffness = 0.0f;       // 見た目の向きを寄せるばねの強さ（1/秒）。大きいほど早く向く。向き終わるのは 約 4.7 ÷ この値 秒。0ならすぐ向く。
	float turnInPlaceAngleDeg = 0.0f; // 入力の方向が体の向きからこれより離れていたら、進まずにその場で回る。0ならいつも進む。
	float acceleration = 0.0f;        // 速くなるときの加速度（cm/秒²）。0なら一瞬でその速さになる。
	float deceleration = 0.0f;        // 遅くなるときの減速度（cm/秒²）。0なら acceleration と同じ。
};

// CharacterController を包み、移動・ジャンプ・重力・向きを処理する。
// パッドやカメラは触らない。
// 使い方は2通りある。
// ・Update(input): 入力どおりに歩く・走る・跳ぶ。状態を持たないキャラクター（Enemy）向け。
// ・Move() / MoveByInput() / Jump() / FaceTo(): 1つずつの操作。ステートで動きを変えるキャラクター（Player）向け。
// どちらも、1フレームに1回だけ Move() 系を呼ぶこと（重力はそこでかかる）。
class CharacterMover
{
public:
	// SetPosition() で決めた位置にカプセルを作る。
	void Init(const CharacterMoveSettings& settings);
	// 入力どおりに動く。走る入力なら runSpeedMultiplier 倍、ジャンプボタンで跳ぶ。
	void Update(const CharacterInput& input, float deltaTime);

	// 水平方向の速度で1フレーム動かす。重力もかける。向きは変えない（見た目の向きは本当の向きへ回す）。
	void Move(const Vector3& horizontalVelocity, float deltaTime);
	// 入力の移動方向に speed の速さで1フレーム動かし、入力の向きたい方向（なければ移動方向）をすぐに向く。
	// 見た目の向きは turnSpeedDeg で少しずつ回り、大きく離れているあいだは進まずにその場で回る。
	void MoveByInput(const CharacterInput& input, float speed, float deltaTime);
	// 地面にいれば、speed の初速で跳ぶ。跳べたら true。
	bool Jump(float speed);
	// 水平方向の向きを、見た目も含めてすぐに変える（ダッシュなど）。ゼロに近い方向は無視する。
	void FaceTo(const Vector3& direction);
	// 速度を打ち消して止める（リスポーン時など）。
	void Stop();

	// Init() の前に呼んでもよい。
	void SetPosition(const Vector3& position);
	const Vector3& GetPosition() const
	{
		return m_position;
	}
	void SetRotation(const Quaternion& rotation)
	{
		m_rotation = rotation;
		m_visualRotation = rotation;
		m_visualYawVelocity = 0.0f;
	}
	// 本当の向き（移動・魔法を撃つ向きなど）。入力した方向をすぐに向く。
	const Quaternion& GetRotation() const
	{
		return m_rotation;
	}
	// 見た目の向き（モデルの向き）。本当の向きへ turnSpeedDeg で少しずつ回る。
	const Quaternion& GetVisualRotation() const
	{
		return m_visualRotation;
	}
	Vector3 GetForward() const;
	const CharacterMoveSettings& GetSettings() const
	{
		return m_settings;
	}

	bool IsOnGround() const;
	// 水平方向に動いているか。アニメーションの切り替えに使う。
	bool IsMoving() const
	{
		return m_isMoving;
	}

private:
	// 本当の向きだけを変える。向けたら true。
	bool SetFacing(const Vector3& direction);
	// 見た目の向きを、本当の向きへ turnSpeedDeg の速さで近い方へ回す。
	void UpdateVisualRotation(float deltaTime);
	// 今の水平の速度を、acceleration・deceleration の速さで targetVelocity へ近づけた値を返す。
	Vector3 ApproachVelocity(const Vector3& targetVelocity, float deltaTime) const;

	CharacterMoveSettings m_settings;
	CharacterController m_charaCon;
	Vector3 m_position = Vector3::Zero;
	Vector3 m_moveSpeed = Vector3::Zero;
	Quaternion m_rotation = Quaternion::Identity;
	Quaternion m_visualRotation = Quaternion::Identity;
	float m_visualYawVelocity = 0.0f;  // 見た目の向きが回る速さ（ラジアン/秒）。
	bool m_isMoving = false;
	bool m_isInitialized = false;
};
