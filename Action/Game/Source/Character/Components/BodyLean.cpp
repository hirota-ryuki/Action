#include "stdafx.h"
#include "BodyLean.h"

namespace
{
	// 重力加速度（cm/秒²）。傾きの角度を、加速度と重力の釣り合いから求める。
	constexpr float GRAVITY = 980.0f;
	// これより速く動いたら、ワープ（リスポーンなど）とみなして傾きをなくす（cm/秒）。
	constexpr float TELEPORT_SPEED = 5000.0f;

	float WrapAngle(float angle)
	{
		while (angle > Math::PI) {
			angle -= Math::PI2;
		}
		while (angle < -Math::PI) {
			angle += Math::PI2;
		}
		return angle;
	}

	// 目標に、time 秒くらいで近づく割合。
	float SmoothRate(float deltaTime, float time)
	{
		if (time <= 0.0f) {
			return 1.0f;
		}
		return 1.0f - expf(-deltaTime / time);
	}
}

void BodyLean::Init(const BodyLeanSettings& settings)
{
	m_settings = settings;
	m_isFirstFrame = true;
}

Quaternion BodyLean::Update(const Vector3& position, const Quaternion& rotation, float deltaTime)
{
	if (!m_settings.isEnabled || deltaTime <= 0.0f) {
		return rotation;
	}
	Vector3 forward = Vector3::AxisZ;
	rotation.Apply(forward);
	forward.y = 0.0f;
	const float yaw = atan2f(forward.x, forward.z);

	Vector3 velocity = (position - m_prevPosition) * (1.0f / deltaTime);
	velocity.y = 0.0f;
	if (m_isFirstFrame || velocity.Length() > TELEPORT_SPEED) {
		m_isFirstFrame = false;
		m_prevPosition = position;
		m_prevYaw = yaw;
		m_smoothedVelocity = Vector3::Zero;
		m_roll = 0.0f;
		m_pitch = 0.0f;
		return rotation;
	}
	const float yawRate = WrapAngle(yaw - m_prevYaw) / deltaTime;
	m_prevPosition = position;
	m_prevYaw = yaw;

	// 速さをならしてから、前向きの加速を求める。
	Vector3 smoothed = m_smoothedVelocity;
	smoothed.Lerp(SmoothRate(deltaTime, m_settings.velocitySmoothSeconds), m_smoothedVelocity, velocity);
	const Vector3 acceleration = (smoothed - m_smoothedVelocity) * (1.0f / deltaTime);
	m_smoothedVelocity = smoothed;
	if (forward.LengthSq() > FLT_EPSILON) {
		forward.Normalize();
	}
	const float forwardAcceleration = acceleration.Dot(forward);
	const float speed = m_smoothedVelocity.Length();

	// 曲がるとき: 遠心力（速さ × 回る速さ）と重力が釣り合う角度で内側へ。右に曲がる（yaw が増える）と右へ傾く。
	const float maxRoll = Math::DegToRad(m_settings.maxTurnLeanDeg);
	float targetRoll = -atanf(speed * yawRate / GRAVITY) * m_settings.turnLeanScale;
	targetRoll = max(-maxRoll, min(maxRoll, targetRoll));
	// 速くなるとき前へ、遅くなるとき後ろへ。
	const float maxPitch = Math::DegToRad(m_settings.maxAccelLeanDeg);
	float targetPitch = atanf(forwardAcceleration / GRAVITY) * m_settings.accelLeanScale;
	targetPitch = max(-maxPitch, min(maxPitch, targetPitch));

	const float leanRate = SmoothRate(deltaTime, m_settings.leanSmoothSeconds);
	m_roll += (targetRoll - m_roll) * leanRate;
	m_pitch += (targetPitch - m_pitch) * leanRate;

	// 体の向きで見た前後（X 軸回り）・左右（Z 軸回り）に傾けてから、体の向きに回す。
	Quaternion pitchRotation, rollRotation;
	pitchRotation.SetRotation(Vector3::AxisX, m_pitch);
	rollRotation.SetRotation(Vector3::AxisZ, m_roll);
	return rotation * (rollRotation * pitchRotation);
}
