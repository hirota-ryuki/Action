#pragma once

#include "Faction.h"

class HitTarget;

// 魔法の性能。
struct MagicSettings
{
	float speed = 1500.0f;   // 毎秒の移動距離。
	float range = 2000.0f;   // これだけ飛ぶと消える。
	float radius = 10.0f;    // 当たり判定の半径。相手の当たり判定の半径と足して判定する。
};

// 魔法の基底クラス。撃つたびに1つ作り、飛んでいく1発そのものになる。
// 溜め撃ちの魔法は、離すまで撃つ位置で待つ。そのあと（溜めない魔法は作った直後から）まっすぐ飛び、
// 敵対する相手か地形に当たるか射程が尽きると着弾する。着弾後は演出が終わるのを待ってから自分を削除する。
//
// 派生クラスは見た目と効果だけを書く:
//   OnStart()          エフェクトやモデルの読み込み。失敗したら false を返すと削除される。
//   OnCharge()         待っている間、毎フレーム（溜め撃ちだけ）。押している秒数が渡される。
//   OnLaunch()         飛び始める瞬間（溜め撃ちだけ）。溜め具合で性能を変えるならここで SetSettings() する。
//   OnCancel()         待っている途中で中断されたとき（溜め撃ちだけ）。
//   OnMove()           位置が変わったときに見た目を合わせる。
//   OnHitTarget()      敵対する相手に当たったときの効果（凍結など）。
//   OnExplode()        着弾したときの演出（相手・地形・射程切れのすべてで呼ばれる）。
//   IsVanishFinished() 着弾後（中断後）の演出が終わったか。
class MagicBase : public IGameObject
{
public:
	// NewGO の名前。Game が終了時にまとめて削除するのに使う。
	static constexpr const char* OBJECT_NAME = "magic";

	virtual ~MagicBase() = default;

	// NewGO 直後、Start() より前に呼ぶ。direction は正規化していなくてよい。
	// owner には当たらない。owner は削除済みになることがあるので比較にだけ使う。
	void Init(const Vector3& position, const Vector3& direction, const IGameObject* owner, Faction faction);

	// true なら、Release() されるまで撃つ位置で待つ（溜め撃ち）。
	virtual bool IsChargeType() const
	{
		return false;
	}
	// 待っている間、毎フレーム呼ぶ。撃つ位置と向きを手元に合わせる。
	void Hold(const Vector3& position, const Vector3& direction);
	// 離した。待っていれば飛び始める。
	void Release();
	// 中断した。待っていれば飛ばずに消える。
	void Cancel();

	bool Start() final;
	void Update() final;

protected:
	explicit MagicBase(const MagicSettings& settings);

	// 溜め具合などで性能を変えたいときに、OnLaunch() の中で呼ぶ。
	void SetSettings(const MagicSettings& settings)
	{
		m_settings = settings;
	}

	virtual bool OnStart() { return true; }
	virtual void OnCharge(float chargeSeconds) { (void)chargeSeconds; }
	virtual void OnLaunch() {}
	virtual void OnCancel() {}
	virtual void OnMove() {}
	virtual void OnHitTarget(HitTarget& target, const Vector3& hitPosition) { (void)target; (void)hitPosition; }
	virtual void OnExplode(const Vector3& hitPosition) { (void)hitPosition; }
	virtual bool IsVanishFinished() const { return true; }

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
		Charging,    // 撃つ位置で待っている（溜め撃ちだけ）。
		Flying,      // 飛翔中。
		Vanishing,   // 着弾済み（または中断済み）。演出が終わるのを待つ。
	};

	void SetPose(const Vector3& position, const Vector3& direction);
	void UpdateCharging();
	void UpdateFlying();
	void Explode(const Vector3& hitPosition);

	MagicSettings m_settings;
	const IGameObject* m_owner = nullptr;
	Vector3 m_position = Vector3::Zero;
	Vector3 m_direction = Vector3::AxisZ;     // 進行方向（正規化済み）。
	Quaternion m_rotation = Quaternion::Identity;
	float m_flightDistance = 0.0f;            // ここまでに飛んだ距離。射程判定に使う。
	float m_chargeStartTime = 0.0f;           // 待ち始めた時刻（GameClock）。
	Faction m_faction = Faction::Neutral;
	EnState m_state = EnState::Flying;
	bool m_isValid = false;                   // OnStart() に成功したか。
	bool m_isReleased = false;                // Release() された。次の Update で飛び始める。
	bool m_isCanceled = false;                // Cancel() された。次の Update で消え始める。
};
