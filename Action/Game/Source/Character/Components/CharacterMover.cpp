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
		// 入力の方向が体の向きから大きく離れているあいだは、その場で回る（向き終わってから進む）。
		// 回りながら進むと、体だけ遅れて横や後ろへ滑ったり、大回りしたりして見える。
		if (m_settings.turnInPlaceAngleDeg > 0.0f) {
			Vector3 forward = GetForward();
			forward.y = 0.0f;
			forward.Normalize();
			Vector3 inputDirection = horizontalVelocity;
			inputDirection.Normalize();
			if (forward.Dot(inputDirection) < cosf(Math::DegToRad(m_settings.turnInPlaceAngleDeg))) {
				horizontalVelocity = Vector3::Zero;
			}
		}
	}

	// 速さは一瞬で変えず、加速・減速の速さで近づける。
	Move(ApproachVelocity(horizontalVelocity, deltaTime), deltaTime);
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

void CharacterMover::FaceTo(const Vector3& direction)
{
	Vector3 horizontal = direction;
	horizontal.y = 0.0f;
	if (horizontal.LengthSq() <= FLT_EPSILON) {
		return;
	}
	m_rotation.SetRotationYFromDirectionXZ(horizontal);
}

Vector3 CharacterMover::ApproachVelocity(const Vector3& targetVelocity, float deltaTime) const
{
	if (m_settings.acceleration <= 0.0f) {
		return targetVelocity;
	}
	const Vector3 current(m_moveSpeed.x, 0.0f, m_moveSpeed.z);
	Vector3 diff = targetVelocity - current;
	const float diffLength = diff.Length();
	// 速くなるときは acceleration、遅くなるときは deceleration（0 なら acceleration）。
	const bool isSpeedingUp = targetVelocity.LengthSq() >= current.LengthSq();
	const float rate = (isSpeedingUp || m_settings.deceleration <= 0.0f) ? m_settings.acceleration : m_settings.deceleration;
	const float maxStep = rate * deltaTime;
	if (diffLength <= maxStep) {
		return targetVelocity;
	}
	diff *= maxStep / diffLength;
	return current + diff;
}

void CharacterMover::TurnTo(const Vector3& direction, float deltaTime)
{
	if (m_settings.turnSpeedDeg <= 0.0f) {
		FaceTo(direction);
		return;
	}
	if (direction.x * direction.x + direction.z * direction.z <= FLT_EPSILON) {
		return;
	}
	// 今の向きと向きたい方向を Y 軸回りの角度にし、差を -180°〜180° にそろえて近い方へ回る。
	const Vector3 forward = GetForward();
	const float currentAngle = atan2f(forward.x, forward.z);
	const float targetAngle = atan2f(direction.x, direction.z);
	float diff = targetAngle - currentAngle;
	while (diff > Math::PI) {
		diff -= Math::PI2;
	}
	while (diff < -Math::PI) {
		diff += Math::PI2;
	}
	const float maxStep = Math::DegToRad(m_settings.turnSpeedDeg) * deltaTime;
	if (fabsf(diff) <= maxStep) {
		m_rotation.SetRotationY(targetAngle);
		return;
	}
	m_rotation.SetRotationY(currentAngle + (diff > 0.0f ? maxStep : -maxStep));
}
