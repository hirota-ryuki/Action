#include "stdafx.h"
#include "CharacterMover.h"
#include "CharacterInput.h"

namespace
{
	// 移動入力のデッドゾーン閾値（二乗値）。sqrt を避けるため平方根前の値で比較。
	constexpr float kMoveDeadZoneSq = 0.01f;
}

void CharacterMover::Init(const CharacterMoveSettings& settings)
{
	m_settings = settings;
	m_charaCon.Init(m_settings.capsuleRadius, m_settings.capsuleHeight, m_position);
	m_isInitialized = true;
}

void CharacterMover::SetPosition(const Vector3& position)
{
	m_position = position;
	if (m_isInitialized) {
		m_charaCon.SetPosition(position);
	}
}

Vector3 CharacterMover::GetForward() const
{
	Vector3 forward = Vector3::AxisZ;
	m_rotation.Apply(forward);
	return forward;
}

bool CharacterMover::IsOnGround() const
{
	return m_isInitialized && m_charaCon.IsOnGround();
}

void CharacterMover::Stop()
{
	m_moveSpeed = Vector3::Zero;
	m_rushRemainingDistance = 0.0f;
	m_isMoving = false;
}

void CharacterMover::Update(const CharacterInput& input, float deltaTime)
{
	if (!m_isInitialized) {
		return;
	}

	Vector3 moveDirection = input.moveDirection;
	moveDirection.y = 0.0f;
	const bool canRun = input.isRun && m_settings.runSpeedMultiplier != 1.0f;
	const float speed = canRun ? m_settings.walkSpeed * m_settings.runSpeedMultiplier : m_settings.walkSpeed;
	Vector3 horizontalMoveSpeed = moveDirection * speed;

	if (input.rush.isTrigger && !IsRushing() && m_settings.rushDistance > 0.0f) {
		StartRush(horizontalMoveSpeed);
	}

	if (IsRushing() && deltaTime > FLT_EPSILON) {
		// 今フレームの移動距離が残り距離を超えないよう、小さい方の値を使う。
		const float maxRushMoveDistance = m_settings.rushSpeed * deltaTime;
		const float rushMoveDistance = maxRushMoveDistance < m_rushRemainingDistance ? maxRushMoveDistance : m_rushRemainingDistance;
		horizontalMoveSpeed = m_rushDirection * (rushMoveDistance / deltaTime);
		m_rushRemainingDistance -= rushMoveDistance;
		if (m_rushRemainingDistance < 0.0f) {
			m_rushRemainingDistance = 0.0f;
		}
	}

	m_moveSpeed.x = horizontalMoveSpeed.x;
	m_moveSpeed.z = horizontalMoveSpeed.z;

	if (input.jump.isTrigger && m_settings.jumpSpeed > 0.0f && m_charaCon.IsOnGround()) {
		m_moveSpeed.y = m_settings.jumpSpeed;
	}
	m_moveSpeed.y -= m_settings.gravity * deltaTime;

	// 向きたい方向の指定があればそちらを、なければ移動方向を向く。
	m_isMoving = horizontalMoveSpeed.LengthSq() > kMoveDeadZoneSq;
	Vector3 lookDirection = input.lookDirection;
	lookDirection.y = 0.0f;
	if (lookDirection.LengthSq() > FLT_EPSILON && !IsRushing()) {
		m_rotation.SetRotationYFromDirectionXZ(lookDirection);
	}
	else if (m_isMoving) {
		m_rotation.SetRotationYFromDirectionXZ(horizontalMoveSpeed);
	}

	m_position = m_charaCon.Execute(m_moveSpeed, deltaTime);

	if (m_charaCon.IsOnGround()) {
		m_moveSpeed.y = 0.0f;
	}
}

void CharacterMover::StartRush(const Vector3& direction)
{
	m_rushDirection = direction;
	m_rushDirection.y = 0.0f;
	// 移動入力がデッドゾーン以下なら、向いている方向へラッシュする。
	if (m_rushDirection.LengthSq() <= kMoveDeadZoneSq) {
		m_rushDirection = GetForward();
		m_rushDirection.y = 0.0f;
	}
	if (m_rushDirection.LengthSq() > FLT_EPSILON) {
		m_rushDirection.Normalize();
		m_rushRemainingDistance = m_settings.rushDistance;
	}
}
