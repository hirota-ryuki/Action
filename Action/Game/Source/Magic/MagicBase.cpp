#include "stdafx.h"
#include "MagicBase.h"
#include "HitTarget.h"
#include "GameClock.h"

MagicBase::MagicBase(const MagicSettings& settings)
	: m_settings(settings)
{
}

void MagicBase::Init(const Vector3& position, const Vector3& direction, const IGameObject* owner, Faction faction)
{
	SetPose(position, direction);
	m_owner = owner;
	m_faction = faction;
	m_state = IsChargeType() ? EnState::Charging : EnState::Flying;
	m_chargeStartTime = GameClock::Now();
}

void MagicBase::Hold(const Vector3& position, const Vector3& direction)
{
	if (m_state == EnState::Charging) {
		SetPose(position, direction);
	}
}

void MagicBase::Release()
{
	m_isReleased = true;
}

void MagicBase::Cancel()
{
	m_isCanceled = true;
}

bool MagicBase::Start()
{
	m_isValid = OnStart();
	if (m_isValid) {
		// 見た目を最初の位置に合わせておく。
		if (m_state == EnState::Charging) {
			OnCharge(0.0f);
		}
		else {
			OnMove();
		}
	}
	return true;
}

void MagicBase::Update()
{
	if (!m_isValid) {
		// 待っている間は MagicCaster がこの魔法を指しているので、離されるか中断されるまで消えない。
		if (m_state == EnState::Charging && !m_isReleased && !m_isCanceled) {
			return;
		}
		DeleteGO(this);
		return;
	}

	switch (m_state) {
	case EnState::Charging:
		UpdateCharging();
		break;
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

void MagicBase::SetPose(const Vector3& position, const Vector3& direction)
{
	m_position = position;
	m_direction = direction;
	if (m_direction.LengthSq() <= FLT_EPSILON) {
		m_direction = Vector3::AxisZ;
	}
	m_direction.Normalize();
	m_rotation.SetRotationYFromDirectionXZ(m_direction);
}

void MagicBase::UpdateCharging()
{
	if (m_isCanceled) {
		OnCancel();
		m_state = EnState::Vanishing;
		return;
	}
	if (m_isReleased) {
		OnLaunch();
		m_state = EnState::Flying;
		OnMove();
		return;
	}
	OnCharge(GameClock::Now() - m_chargeStartTime);
}

void MagicBase::UpdateFlying()
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

void MagicBase::Explode(const Vector3& hitPosition)
{
	m_position = hitPosition;
	OnMove();
	OnExplode(hitPosition);
	m_state = EnState::Vanishing;
}
