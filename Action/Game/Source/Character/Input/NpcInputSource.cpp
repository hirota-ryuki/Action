#include "stdafx.h"
#include "NpcInputSource.h"
#include "HitTarget.h"

NpcInputSource::NpcInputSource(const Settings& settings)
	: m_settings(settings)
{
}

NpcInputSource::Range NpcInputSource::ClassifyRange(float distance) const
{
	if (distance <= m_settings.nearRange) {
		return Range::Near;
	}
	if (distance <= m_settings.midRange) {
		return Range::Middle;
	}
	return Range::Far;
}

const NpcInputSource::RangeAction& NpcInputSource::GetRangeAction(Range range) const
{
	switch (range) {
	case Range::Near:
		return m_settings.nearAction;
	case Range::Middle:
		return m_settings.midAction;
	default:
		return m_settings.farAction;
	}
}

ActionButton NpcInputSource::DecideButton(Range range, float deltaTime)
{
	if (range == Range::Near) {
		if (m_isRunning) {
			// 走って近くまで来たら、歩きに戻してすぐに押す。
			m_isRunning = false;
			m_actionTimer = 0.0f;
		}
		m_decideTimer = m_settings.decideInterval;
		if (m_actionTimer <= 0.0f) {
			return m_settings.nearAction.button;
		}
		return ActionButton::Num;
	}
	if (m_isRunning) {
		// 走っている間は、近くまで来るまで押さない。
		return ActionButton::Num;
	}
	// 中くらい・遠いときは、歩きながら間隔ごとに決め直す。
	m_decideTimer -= deltaTime;
	if (m_decideTimer > 0.0f || m_actionTimer > 0.0f) {
		return ActionButton::Num;
	}
	m_decideTimer = m_settings.decideInterval;
	std::uniform_real_distribution<float> chance(0.0f, 1.0f);
	if (chance(m_random) < m_settings.runChance) {
		m_isRunning = true;
		return ActionButton::Num;
	}
	return GetRangeAction(range).button;
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
		m_isRunning = false;
		m_holdButton = ActionButton::Num;
		for (ButtonState& action : input.actions) {
			action.Update(false);
		}
		return;
	}
	if (!m_hasTarget) {
		// 見つけた直後は歩き出し、少し待ってから行動を決める。
		m_hasTarget = true;
		m_isRunning = false;
		m_decideTimer = m_settings.decideInterval;
		m_actionTimer = m_settings.firstActionDelay;
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
			input.moveDirection = direction * m_settings.moveRate;
		}
	}

	if (m_actionTimer > 0.0f) {
		m_actionTimer -= context.deltaTime;
	}

	ActionButton pressButton = ActionButton::Num;  // このフレームに押しているボタン。Num なら押さない。
	if (m_holdButton != ActionButton::Num) {
		// 押し続けている途中。時間が来たら離す（離したフレームも、ほかのボタンは押さない。
		// 炎魔法は離した瞬間に撃つので、同じフレームに別のボタンを押すと重なるため）。
		m_holdTimer -= context.deltaTime;
		if (m_holdTimer > 0.0f) {
			pressButton = m_holdButton;
		}
		else {
			m_holdButton = ActionButton::Num;
		}
	}
	else {
		const Range range = ClassifyRange(distance);
		pressButton = DecideButton(range, context.deltaTime);
		if (pressButton != ActionButton::Num) {
			m_actionTimer = m_settings.actionInterval;
			m_decideTimer = m_settings.decideInterval;
			m_holdButton = pressButton;
			m_holdTimer = GetRangeAction(range).holdSeconds;
		}
	}
	input.isRun = m_isRunning;
	for (int button = 0; button < static_cast<int>(ActionButton::Num); button++) {
		input.actions[button].Update(button == static_cast<int>(pressButton));
	}

	// 足元から castHeight の高さから、相手の胸のあたりを狙う。
	input.hasAim = true;
	input.aimOrigin = context.position;
	input.aimOrigin.y += m_settings.castHeight;
	input.aimDirection = target->GetCenter() - input.aimOrigin;
}
