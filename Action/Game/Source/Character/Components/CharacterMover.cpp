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
	m_isMoving = false;
}

void CharacterMover::Update(const CharacterInput& input, float deltaTime)
{
	const bool canRun = input.isRun && m_settings.runSpeedMultiplier != 1.0f;
	const float speed = canRun ? m_settings.walkSpeed * m_settings.runSpeedMultiplier : m_settings.walkSpeed;
	if (input.jump.isTrigger) {
		Jump(m_settings.jumpSpeed);
	}
	MoveByInput(input, speed, deltaTime);
}

void CharacterMover::MoveByInput(const CharacterInput& input, float speed, float deltaTime)
{
	Vector3 horizontalVelocity = input.moveDirection;
	horizontalVelocity.y = 0.0f;
	horizontalVelocity *= speed;

	// 向きたい方向の指定があればそちらを、なければ移動方向を向く。
	Vector3 lookDirection = input.lookDirection;
	lookDirection.y = 0.0f;
	if (lookDirection.LengthSq() > FLT_EPSILON) {
		FaceTo(lookDirection);
	}
	else if (horizontalVelocity.LengthSq() > kMoveDeadZoneSq) {
		FaceTo(horizontalVelocity);
	}

	Move(horizontalVelocity, deltaTime);
}

void CharacterMover::Move(const Vector3& horizontalVelocity, float deltaTime)
{
	if (!m_isInitialized) {
		return;
	}

	m_moveSpeed.x = horizontalVelocity.x;
	m_moveSpeed.z = horizontalVelocity.z;
	m_moveSpeed.y -= m_settings.gravity * deltaTime;
	m_isMoving = m_moveSpeed.x * m_moveSpeed.x + m_moveSpeed.z * m_moveSpeed.z > kMoveDeadZoneSq;

	m_position = m_charaCon.Execute(m_moveSpeed, deltaTime);

	if (m_charaCon.IsOnGround()) {
		m_moveSpeed.y = 0.0f;
	}
}

bool CharacterMover::Jump(float speed)
{
	if (speed <= 0.0f || !IsOnGround()) {
		return false;
	}
	m_moveSpeed.y = speed;
	return true;
}

void CharacterMover::FaceTo(const Vector3& direction)
{
	Vector3 horizontal = direction;
	horizontal.y = 0.0f;
	if (horizontal.LengthSq() <= FLT_EPSILON) {
		return;
	}
	m_rotation.SetRotationYFromDirectionXZ(horizontal);
}
