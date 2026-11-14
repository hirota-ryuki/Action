#include "stdafx.h"
#include "MagicBase.h"
#include "HitTarget.h"

MagicBase::MagicBase(const MagicSettings& settings)
	: m_settings(settings)
{
}

void MagicBase::OnActivate()
{
	// 飛び始める位置に見た目を合わせる。
	OnMove();
}

void MagicBase::OnSkillUpdate()
{
	const float moveDistance = m_settings.speed * g_gameTime->GetFrameDeltaTime();
	const Vector3 nextPosition = GetPosition() + GetDirection() * moveDistance;

	// キャラクター → 地形の順に調べる。地形のレイはキャラクターのカプセルにも当たるが、
	// キャラクターの当たり判定の方が大きいので、狙って撃てば先にキャラクターとして命中する。
	Vector3 hitPosition;
	HitTarget* hitTarget = HitTargetRegistry::FindHitOnSegment(
		GetPosition(), nextPosition, m_settings.radius, GetFaction(), GetOwner(), hitPosition);
	if (hitTarget != nullptr) {
		HitInfo hitInfo;
		hitInfo.attacker = GetOwner();
		hitInfo.attackerFaction = GetFaction();
		hitInfo.position = hitPosition;
		hitInfo.direction = GetDirection();
		hitTarget->NotifyHit(hitInfo);
		OnHitTarget(*hitTarget, hitPosition);
		Explode(hitPosition);
		return;
	}

	PhysicsWorld* physicsWorld = PhysicsWorld::GetInstance();
	if (physicsWorld != nullptr && physicsWorld->RayTest(GetPosition(), nextPosition, hitPosition)) {
		Explode(hitPosition);
		return;
	}

	SetPosition(nextPosition);
	m_flightDistance += moveDistance;
	if (m_flightDistance >= m_settings.range) {
		// 射程切れは空中で着弾した扱いにする。
		Explode(GetPosition());
		return;
	}

	OnMove();
}

void MagicBase::Explode(const Vector3& hitPosition)
{
	SetPosition(hitPosition);
	OnMove();
	OnExplode(hitPosition);
	End();
}
