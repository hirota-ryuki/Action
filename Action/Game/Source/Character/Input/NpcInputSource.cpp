#include "stdafx.h"
#include "NpcInputSource.h"
#include "HitTarget.h"

NpcInputSource::NpcInputSource(const Settings& settings)
	: m_settings(settings)
{
}

void NpcInputSource::Update(const InputSourceContext& context, CharacterInput& input)
{
	input.moveDirection = Vector3::Zero;
	input.lookDirection = Vector3::Zero;
	input.isRun = false;
	input.jump.Update(false);
	input.dash.Update(false);
	input.hasAim = false;

	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);

	if (target == nullptr) {
		m_hasTarget = false;
		m_isFiring = false;
		input.fire.Update(false);
		return;
	}
	if (!m_hasTarget) {
		// 見つけた直後にいきなり撃たないよう、少し待つ。
		m_fireTimer = m_settings.firstFireDelay;
		m_hasTarget = true;
	}

	// 高低差を無視して水平方向に追う。
	Vector3 toTarget = target->GetCenter() - context.position;
	toTarget.y = 0.0f;
	const float distanceSq = toTarget.LengthSq();
	if (distanceSq > FLT_EPSILON) {
		Vector3 direction = toTarget;
		direction.Normalize();
		input.lookDirection = direction;
		if (distanceSq > m_settings.stopDistance * m_settings.stopDistance) {
			input.moveDirection = direction * m_settings.moveRate;
		}
	}

	// 撃つかどうかを決める。
	const bool isInFireRange = distanceSq <= m_settings.fireRange * m_settings.fireRange;
	if (m_fireTimer > 0.0f) {
		m_fireTimer -= context.deltaTime;
	}
	if (m_isFiring) {
		m_fireHoldTimer -= context.deltaTime;
		if (m_fireHoldTimer <= 0.0f) {
			m_isFiring = false;
		}
	}
	else if (m_settings.canFire && isInFireRange && m_fireTimer <= 0.0f) {
		m_isFiring = true;
		m_fireHoldTimer = m_settings.fireHoldSeconds;
		m_fireTimer = m_settings.fireInterval;
	}
	input.fire.Update(m_isFiring);

	// 相手の胸のあたりを狙う。
	input.hasAim = true;
	input.aimOrigin = context.position;
	input.aimOrigin.y += m_settings.castHeight;
	input.aimDirection = target->GetCenter() - input.aimOrigin;
}
