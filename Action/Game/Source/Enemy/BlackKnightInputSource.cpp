#include "stdafx.h"
#include "BlackKnightInputSource.h"
#include "HitTarget.h"

BlackKnightInputSource::BlackKnightInputSource(const Settings& settings)
	: m_settings(settings)
{
}

BlackKnightInputSource::Range BlackKnightInputSource::ClassifyRange(float distance) const
{
	if (distance <= m_settings.nearRange) {
		return Range::Near;
	}
	if (distance <= m_settings.midRange) {
		return Range::Middle;
	}
	return Range::Far;
}

void BlackKnightInputSource::Update(const InputSourceContext& context, CharacterInput& input)
{
	input.moveDirection = Vector3::Zero;
	input.lookDirection = Vector3::Zero;
	input.isRun = false;
	input.jump.Update(false);
	input.dash.Update(false);
	input.subFire.Update(false);
	input.magicSwitch = 0;
	input.hasAim = false;

	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);
	if (target == nullptr) {
		m_hasTarget = false;
		m_isRunning = false;
		input.fire.Update(false);
		return;
	}
	if (!m_hasTarget) {
		// 見つけた直後は歩き出し、少し待ってから行動を決める。
		m_hasTarget = true;
		m_isRunning = false;
		m_decideTimer = m_settings.decideInterval;
		m_attackTimer = m_settings.firstAttackDelay;
	}

	// 高低差を無視して、水平方向に相手を向いて近づく。
	Vector3 toTarget = target->GetCenter() - context.position;
	toTarget.y = 0.0f;
	const float distance = toTarget.Length();
	if (distance > FLT_EPSILON) {
		Vector3 direction = toTarget;
		direction.Normalize();
		input.lookDirection = direction;
		if (distance > m_settings.stopDistance) {
			input.moveDirection = direction;
		}
	}

	if (m_attackTimer > 0.0f) {
		m_attackTimer -= context.deltaTime;
	}

	const Range range = ClassifyRange(distance);
	bool isAttack = false;
	if (range == Range::Near) {
		if (m_isRunning) {
			// 走って近くまで来たら、歩きに戻してすぐに振る。
			m_isRunning = false;
			m_attackTimer = 0.0f;
		}
		if (m_attackTimer <= 0.0f) {
			isAttack = true;
			m_attackTimer = m_settings.attackInterval;
		}
		m_decideTimer = m_settings.decideInterval;
	}
	else if (!m_isRunning) {
		// 中くらい・遠いときは、歩きながら間隔ごとに決め直す。
		m_decideTimer -= context.deltaTime;
		if (m_decideTimer <= 0.0f) {
			m_decideTimer = m_settings.decideInterval;
			std::uniform_real_distribution<float> chance(0.0f, 1.0f);
			if (chance(m_random) < m_settings.runChance) {
				m_isRunning = true;
			}
			// 残りは攻撃（中くらいなら攻撃1、遠いなら攻撃3）。まだモーションがないので、歩き続ける。
		}
	}

	input.isRun = m_isRunning;
	// fire は1フレームだけ押す（EnemyGroundState が押した瞬間に Attack へ移る）。
	input.fire.Update(isAttack);
}
