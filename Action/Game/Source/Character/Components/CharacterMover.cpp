#include "stdafx.h"
#include "CharacterMover.h"
#include "CharacterInput.h"

namespace
{
	// 移動入力のデッドゾーン閾値（二乗値）。sqrt を避けるため平方根前の値で比較。
	constexpr float MOVE_DEAD_ZONE_SQ = 0.01f;
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

	// 向きたい方向の指定があればそちらへ、なければ移動方向へ回る。
	Vector3 lookDirection = input.lookDirection;
	lookDirection.y = 0.0f;
	if (lookDirection.LengthSq() > FLT_EPSILON) {
		TurnTo(lookDirection, deltaTime);
	}
	else if (horizontalVelocity.LengthSq() > MOVE_DEAD_ZONE_SQ) {
		TurnTo(horizontalVelocity, deltaTime);
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
	m_isMoving = m_moveSpeed.x * m_moveSpeed.x + m_moveSpeed.z * m_moveSpeed.z > MOVE_DEAD_ZONE_SQ;

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

void CharacterMover::TurnTo(const Vector3& direction, float deltaTime)
{
	if (m_settings.turnSpeedDeg <= 0.0f) {
		FaceTo(direction);
		return;
	}
	Vector3 horizontal = direction;
	horizontal.y = 0.0f;
	if (horizontal.LengthSq() <= FLT_EPSILON) {
		return;
	}
	// 今の向きと目標の向きを、Y軸まわりの角度にする（SetRotationYFromDirectionXZ と同じ測り方）。
	const Vector3 forward = GetForward();
	const float currentYaw = atan2f(forward.x, forward.z);
	const float targetYaw = atan2f(horizontal.x, horizontal.z);
	// 差を -180〜180° にして、近い方へ回る。
	float diff = targetYaw - currentYaw;
	while (diff > Math::PI) {
		diff -= Math::PI2;
	}
	while (diff < -Math::PI) {
		diff += Math::PI2;
	}
	// 1フレームで回れる角度を超えたら、そこで止める（次のフレームで続きを回る）。
	const float maxStep = Math::DegToRad(m_settings.turnSpeedDeg) * deltaTime;
	if (fabsf(diff) <= maxStep) {
		m_rotation.SetRotationY(targetYaw);
		return;
	}
	m_rotation.SetRotationY(currentYaw + (diff > 0.0f ? maxStep : -maxStep));
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
