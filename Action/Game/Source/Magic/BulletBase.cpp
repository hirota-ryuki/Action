#include "stdafx.h"
#include "BulletBase.h"
#include "HitTarget.h"

BulletBase::BulletBase(const BulletSettings& settings)
	: m_settings(settings)
{
}

void BulletBase::Init(const Vector3& position, const Vector3& direction, const IGameObject* owner, Faction faction)
{
	m_position = position;
	m_direction = direction;
	if (m_direction.LengthSq() <= FLT_EPSILON) {
		m_direction = Vector3::AxisZ;
	}
	m_direction.Normalize();
	m_rotation.SetRotationYFromDirectionXZ(m_direction);
	m_owner = owner;
	m_faction = faction;
}

bool BulletBase::Start()
{
	m_isValid = OnStart();
	if (m_isValid) {
		// 見た目を最初の位置に合わせておく。
		OnMove();
	}
	return true;
}

void BulletBase::Update()
{
	if (!m_isValid) {
		DeleteGO(this);
		return;
	}

	switch (m_state) {
	case EnState::Flying:
		UpdateFlying();
		break;
	case EnState::Vanishing:
		if (IsVanishFinished()) {
			DeleteGO(this);
		}
		break;
	}
}

void BulletBase::UpdateFlying()
{
	const float moveDistance = m_settings.speed * g_gameTime->GetFrameDeltaTime();
	const Vector3 nextPosition = m_position + m_direction * moveDistance;

	// キャラクター → 地形の順に調べる。地形のレイはキャラクターのカプセルにも当たるが、
	// キャラクターの当たり判定の方が大きいので、狙って撃てば先にキャラクターとして命中する。
	Vector3 hitPosition;
	HitTarget* hitTarget = HitTargetRegistry::FindHitOnSegment(
		m_position, nextPosition, m_settings.radius, m_faction, m_owner, hitPosition);
	if (hitTarget != nullptr) {
		HitInfo hitInfo;
		hitInfo.attacker = m_owner;
		hitInfo.attackerFaction = m_faction;
		hitInfo.position = hitPosition;
		hitInfo.direction = m_direction;
		hitTarget->NotifyHit(hitInfo);
		OnHitTarget(*hitTarget, hitPosition);
		Explode(hitPosition);
		return;
	}

	PhysicsWorld* physicsWorld = PhysicsWorld::GetInstance();
	if (physicsWorld != nullptr && physicsWorld->RayTest(m_position, nextPosition, hitPosition)) {
		Explode(hitPosition);
		return;
	}

	m_position = nextPosition;
	m_flightDistance += moveDistance;
	if (m_flightDistance >= m_settings.range) {
		// 射程切れは空中で着弾した扱いにする。
		Explode(m_position);
		return;
	}

	OnMove();
}

void BulletBase::Explode(const Vector3& hitPosition)
{
	m_position = hitPosition;
	OnMove();
	OnExplode(hitPosition);
	m_state = EnState::Vanishing;
}
