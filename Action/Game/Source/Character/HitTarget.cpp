#include "stdafx.h"
#include "HitTarget.h"

namespace
{
	// 線分 from→to 上で point に最も近い点を返す。
	Vector3 ClosestPointOnSegment(const Vector3& from, const Vector3& to, const Vector3& point)
	{
		Vector3 segment = to - from;
		const float lengthSq = segment.LengthSq();
		if (lengthSq <= FLT_EPSILON) {
			return from;
		}
		float t = (point - from).Dot(segment) / lengthSq;
		t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
		return from + segment * t;
	}
}

HitTarget::~HitTarget()
{
	if (m_isRegistered) {
		HitTargetRegistry::Unregister(this);
	}
}

void HitTarget::Init(IGameObject* owner, Faction faction, float radius, float centerHeight)
{
	m_owner = owner;
	m_faction = faction;
	m_radius = radius;
	m_centerHeight = centerHeight;
	if (!m_isRegistered) {
		HitTargetRegistry::Register(this);
		m_isRegistered = true;
	}
}

void HitTarget::NotifyHit(const HitInfo& hitInfo) const
{
	if (m_onHit) {
		m_onHit(hitInfo);
	}
}

Vector3 HitTarget::GetCenter() const
{
	Vector3 center = m_footPosition;
	center.y += m_centerHeight;
	return center;
}

bool HitTarget::IsActive() const
{
	return m_owner != nullptr && !m_owner->IsDead();
}

void HitTargetRegistry::Register(HitTarget* target)
{
	Targets().push_back(target);
}

void HitTargetRegistry::Unregister(HitTarget* target)
{
	std::vector<HitTarget*>& targets = Targets();
	targets.erase(std::remove(targets.begin(), targets.end(), target), targets.end());
}

HitTarget* HitTargetRegistry::FindHitOnSegment(
	const Vector3& from, const Vector3& to, float radius,
	Faction attackerFaction, const IGameObject* ignoreOwner,
	Vector3& hitPosition)
{
	HitTarget* hitTarget = nullptr;
	// 複数に当たる場合は、線分の始点に一番近い相手を選ぶ。
	float nearestFromDistanceSq = FLT_MAX;
	for (HitTarget* target : Targets()) {
		if (!target->IsActive() || target->GetOwner() == ignoreOwner) {
			continue;
		}
		if (!IsHostile(attackerFaction, target->GetFaction())) {
			continue;
		}
		const Vector3 center = target->GetCenter();
		const Vector3 closest = ClosestPointOnSegment(from, to, center);
		const float hitRadius = radius + target->GetRadius();
		if ((center - closest).LengthSq() > hitRadius * hitRadius) {
			continue;
		}
		const float fromDistanceSq = (closest - from).LengthSq();
		if (fromDistanceSq < nearestFromDistanceSq) {
			nearestFromDistanceSq = fromDistanceSq;
			hitTarget = target;
			hitPosition = closest;
		}
	}
	return hitTarget;
}

HitTarget* HitTargetRegistry::FindNearestHostile(
	const Vector3& position, float maxDistance,
	Faction faction, const IGameObject* ignoreOwner)
{
	HitTarget* nearest = nullptr;
	float nearestDistanceSq = maxDistance * maxDistance;
	for (HitTarget* target : Targets()) {
		if (!target->IsActive() || target->GetOwner() == ignoreOwner) {
			continue;
		}
		if (!IsHostile(faction, target->GetFaction())) {
			continue;
		}
		const float distanceSq = (target->GetCenter() - position).LengthSq();
		if (distanceSq <= nearestDistanceSq) {
			nearestDistanceSq = distanceSq;
			nearest = target;
		}
	}
	return nearest;
}

const std::vector<HitTarget*>& HitTargetRegistry::GetTargets()
{
	return Targets();
}

std::vector<HitTarget*>& HitTargetRegistry::Targets()
{
	// 関数内の static にして、他の static 変数との初期化順の問題を避ける。
	static std::vector<HitTarget*> s_targets;
	return s_targets;
}
