#pragma once

#include "Faction.h"

class HitTarget;

// 弾の設定。
struct BulletSettings
{
	float speed = 1500.0f;   // 毎秒の移動距離。
	float range = 2000.0f;   // これだけ飛ぶと消える。
	float radius = 10.0f;    // 弾の当たり判定の半径。相手の当たり判定の半径と足して判定する。
};

// 弾の基底クラス。まっすぐ飛び、敵対する相手か地形に当たるか射程が尽きると着弾する。
// 着弾後は演出が終わるのを待ってから自分を削除する。
//
// 派生クラスは見た目と命中時の効果だけを書く:
//   OnStart()          エフェクトやモデルの読み込み。失敗したら false を返すとすぐ削除される。
//   OnMove()           位置が変わったときに見た目を合わせる。
//   OnHitTarget()      敵対する相手に当たったときの効果（凍結など）。
//   OnExplode()        着弾したときの演出（相手・地形・射程切れのすべてで呼ばれる）。
//   IsVanishFinished() 着弾後の演出が終わったか。
class BulletBase : public IGameObject
{
public:
	// NewGO の名前。Game が終了時にまとめて削除するのに使う。
	static constexpr const char* OBJECT_NAME = "bullet";

	virtual ~BulletBase() = default;

	// NewGO 直後、Start() より前に呼ぶ。direction は正規化していなくてよい。
	// owner には当たらない。owner は削除済みになることがあるので比較にだけ使う。
	void Init(const Vector3& position, const Vector3& direction, const IGameObject* owner, Faction faction);

	bool Start() final;
	void Update() final;

protected:
	explicit BulletBase(const BulletSettings& settings);

	// 弾ごとに性能を変えたいとき（溜め具合など）に、Start() より前に呼ぶ。
	void SetSettings(const BulletSettings& settings)
	{
		m_settings = settings;
	}

	virtual bool OnStart() { return true; }
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
		Flying,      // 飛翔中。
		Vanishing,   // 着弾済み。演出が終わるのを待つ。
	};

	void UpdateFlying();
	void Explode(const Vector3& hitPosition);

	BulletSettings m_settings;
	const IGameObject* m_owner = nullptr;
	Vector3 m_position = Vector3::Zero;
	Vector3 m_direction = Vector3::AxisZ;     // 進行方向（正規化済み）。
	Quaternion m_rotation = Quaternion::Identity;
	float m_flightDistance = 0.0f;            // ここまでに飛んだ距離。射程判定に使う。
	Faction m_faction = Faction::Neutral;
	EnState m_state = EnState::Flying;
	bool m_isValid = false;                   // OnStart() に成功したか。
};
