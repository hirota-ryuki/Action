#pragma once

#include "Faction.h"

struct ButtonState;

// スキルの基底クラス。使うたびに1つ NewGO し、終わったら自分で消える。
// 入力を受け取ったあとの判断（押している間・離した・中断）は、ここに書く。派生クラスは書かない。
//
// 流れ:
//   溜めるスキルは、離すまで撃つ位置で待つ（Charging）。溜めないスキルは、作った直後に使い始める。
//   使い始めたら（Active）、派生クラスの OnSkillUpdate() が毎フレーム呼ばれる。
//   派生クラスが End() を呼ぶか、中断されたら（Ending）、IsVanishFinished() が true になるのを待って消える。
//
// 派生クラスが書くもの:
//   COOLDOWN           手を離してから、次に使えるまでの秒数（static constexpr。既定は 0）。
//   IsChargeType()     溜めるなら true。
//   OnStart()          エフェクトやモデルの読み込み。失敗したら false を返すと削除される。
//   OnCharge()         溜めている間、毎フレーム。押している秒数が渡される。
//   OnRelease()        使い始める瞬間（溜めるスキルは離した瞬間）。
//   OnCancel()         溜めている途中で中断されたとき。
//   OnSkillUpdate()    使い始めてから、毎フレーム。
//   IsVanishFinished() 終わったあと（中断のあと）の演出が終わったか。
class SkillBase : public IGameObject
{
public:
	// NewGO の名前。Game が終了時にまとめて削除するのに使う。
	static constexpr const char* OBJECT_NAME = "skill";
	// 手を離してから、次に使えるまでの秒数。派生クラスで同じ名前の定数を書くと、SkillController がそちらを使う。
	static constexpr float COOLDOWN = 0.0f;

	virtual ~SkillBase() = default;

	// NewGO 直後、Start() より前に呼ぶ。direction は正規化していなくてよい。
	// owner はスキルの当たり判定から外す。owner は削除済みになることがあるので比較にだけ使う。
	void Init(const Vector3& position, const Vector3& direction, const IGameObject* owner, Faction faction);

	// そのスキルを使ったボタンの状態を、毎フレーム渡す。押している間は撃つ位置に合わせ、離したら使う。
	void ReceiveInput(const ButtonState& button, const Vector3& position, const Vector3& direction);
	// 中断する。溜めている途中なら、使わずに消える。
	void Cancel();
	// キャラクターがまだ手を離せないか（溜めている間は true）。true の間、SkillController はほかのスキルを使わない。
	bool IsOwnerBusy() const;

	bool Start() final;
	void Update() final;

protected:
	virtual bool IsChargeType() const { return false; }
	virtual bool OnStart() { return true; }
	virtual void OnCharge(float chargeSeconds) { (void)chargeSeconds; }
	virtual void OnRelease() {}
	virtual void OnCancel() {}
	// 使い始めた直後に1回（OnRelease() のあと）。派生クラスの中間（MagicBase など）が見た目を合わせるのに使う。
	virtual void OnActivate() {}
	virtual void OnSkillUpdate() {}
	virtual bool IsVanishFinished() const { return true; }

	// 使い終わった。IsVanishFinished() が true になったら消える。
	void End();

	void SetPosition(const Vector3& position)
	{
		m_position = position;
	}
	const Vector3& GetPosition() const
	{
		return m_position;
	}
	const Vector3& GetDirection() const
	{
		return m_direction;
	}
	const Quaternion& GetRotation() const
	{
		return m_rotation;
	}
	const IGameObject* GetOwner() const
	{
		return m_owner;
	}
	Faction GetFaction() const
	{
		return m_faction;
	}

private:
	enum class EnState
	{
		Charging,  // 撃つ位置で待っている（溜めるスキル）。溜めないスキルも、使い始めるまではここ。
		Active,    // 使っている。
		Ending,    // 使い終わった（または中断した）。演出が終わるのを待つ。
	};

	void SetPose(const Vector3& position, const Vector3& direction);
	void UpdateCharging();
	void Activate();

	const IGameObject* m_owner = nullptr;
	Vector3 m_position = Vector3::Zero;
	Vector3 m_direction = Vector3::AxisZ;     // 向き（正規化済み）。
	Quaternion m_rotation = Quaternion::Identity;
	float m_chargeStartTime = 0.0f;           // 溜め始めた時刻（GameClock）。
	Faction m_faction = Faction::Neutral;
	EnState m_state = EnState::Charging;
	bool m_isValid = false;                   // OnStart() に成功したか。
	bool m_isReleased = false;                // 離した（溜めないスキルは作った時点で true）。次の Update で使い始める。
	bool m_isCanceled = false;                // 中断した。次の Update で消え始める。
};
