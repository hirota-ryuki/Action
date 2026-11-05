#pragma once

#include "Faction.h"

class StatusEffects;

// 何かに当たったときに受け手へ渡す情報。
struct HitInfo
{
	const IGameObject* attacker = nullptr;  // 撃ったキャラクター。削除済みの可能性があるので比較にだけ使う。
	Faction attackerFaction = Faction::Neutral;
	Vector3 position = Vector3::Zero;       // 当たった位置。
	Vector3 direction = Vector3::AxisZ;     // 当たったものの進行方向。
};

// 弾などに当ててもらうための当たり判定。キャラクターがメンバーとして持つ。
// Init() で HitTargetRegistry に登録され、デストラクタで登録が解除される。
// 弾やAIは HitTargetRegistry を通して相手を探すので、相手のクラスを知らなくてよい。
class HitTarget : public Noncopyable
{
public:
	using OnHitFunc = std::function<void(const HitInfo&)>;

	HitTarget() = default;
	~HitTarget();

	// centerHeight: 足元から当たり判定の中心までの高さ。radius: 当たり判定の球の半径。
	void Init(IGameObject* owner, Faction faction, float radius, float centerHeight);
	// 持ち主が毎フレーム足元の位置を渡す。
	void SetPosition(const Vector3& footPosition)
	{
		m_footPosition = footPosition;
	}
	void SetFaction(Faction faction)
	{
		m_faction = faction;
	}
	// 当たったときに状態異常をかける先。nullptr なら状態異常を受けない。
	void SetStatusEffects(StatusEffects* statusEffects)
	{
		m_statusEffects = statusEffects;
	}
	// 当たったときに持ち主へ知らせる処理（ひるみ、反撃など）。
	void SetOnHit(OnHitFunc onHit)
	{
		m_onHit = std::move(onHit);
	}

	// 弾などが当たったときに呼ぶ。
	void NotifyHit(const HitInfo& hitInfo) const;

	Vector3 GetCenter() const;
	float GetRadius() const
	{
		return m_radius;
	}
	Faction GetFaction() const
	{
		return m_faction;
	}
	const IGameObject* GetOwner() const
	{
		return m_owner;
	}
	StatusEffects* GetStatusEffects() const
	{
		return m_statusEffects;
	}
	// 持ち主が削除待ちになっていたら当たらないようにする。
	bool IsActive() const;

private:
	IGameObject* m_owner = nullptr;
	StatusEffects* m_statusEffects = nullptr;
	OnHitFunc m_onHit;
	Vector3 m_footPosition = Vector3::Zero;
	Faction m_faction = Faction::Neutral;
	float m_radius = 0.0f;
	float m_centerHeight = 0.0f;
	bool m_isRegistered = false;
};

// 全キャラクターの当たり判定の一覧。弾の命中判定とAIの索敵に使う。
class HitTargetRegistry
{
public:
	static void Register(HitTarget* target);
	static void Unregister(HitTarget* target);

	// from→to を半径 radius の球が通るとき、attackerFaction と敵対する相手のうち最初に当たるものを返す。
	// ignoreOwner の当たり判定は無視する（撃った本人に当たらないように）。見つからなければ nullptr。
	static HitTarget* FindHitOnSegment(
		const Vector3& from, const Vector3& to, float radius,
		Faction attackerFaction, const IGameObject* ignoreOwner,
		Vector3& hitPosition);

	// position から maxDistance 以内で、faction と敵対する相手のうち一番近いものを返す。
	static HitTarget* FindNearestHostile(
		const Vector3& position, float maxDistance,
		Faction faction, const IGameObject* ignoreOwner);

	// 登録されている当たり判定をデバッグ表示用に返す。
	static const std::vector<HitTarget*>& GetTargets();

private:
	static std::vector<HitTarget*>& Targets();
};
