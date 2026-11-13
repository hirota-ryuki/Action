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
	input.magicSwitch = 0;
	input.hasAim = false;

	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);

	if (target == nullptr) {
		m_hasTarget = false;
		ReleaseAll();
		ApplyButtons(input);
		return;
	}
	if (!m_hasTarget) {
		// 見つけた直後にいきなり攻撃しないよう、少し待つ。
		for (int i = 0; i < ACTION_NUM; i++) {
			m_actions[i].waitTimer = GetActionSettings(i).firstDelay;
		}
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

	UpdateActions(distanceSq, context.deltaTime);
	ApplyButtons(input);

	// 相手の胸のあたりを狙う。
	input.hasAim = true;
	input.aimOrigin = context.position;
	input.aimOrigin.y += m_settings.castHeight;
	input.aimDirection = target->GetCenter() - input.aimOrigin;
}

const NpcInputSource::ActionSettings& NpcInputSource::GetActionSettings(int index) const
{
	switch (index) {
	case ACTION_ATTACK:
		return m_settings.attack;
	case ACTION_FIRE:
		return m_settings.fire;
	default:
		return m_settings.subFire;
	}
}

void NpcInputSource::UpdateActions(float distanceSq, float deltaTime)
{
	// 待ち時間を進め、押し続ける時間が過ぎたものは離す。
	// このフレームで離したものも「使っている」と数え、同じフレームで別のボタンを押さない（離した瞬間に撃つ魔法があるため）。
	bool isBusy = false;
	for (ActionState& state : m_actions) {
		if (state.waitTimer > 0.0f) {
			state.waitTimer -= deltaTime;
		}
		if (state.isPressing) {
			isBusy = true;
			state.holdTimer -= deltaTime;
			if (state.holdTimer <= 0.0f) {
				state.isPressing = false;
			}
		}
	}
	if (isBusy) {
		return;
	}

	// 押せるもののうち、届く距離がいちばん短いものを押す。
	int chosen = -1;
	float chosenRange = FLT_MAX;
	for (int i = 0; i < ACTION_NUM; i++) {
		const ActionSettings& settings = GetActionSettings(i);
		const bool canPress = settings.isEnabled
			&& m_actions[i].waitTimer <= 0.0f
			&& distanceSq <= settings.range * settings.range;
		if (canPress && settings.range < chosenRange) {
			chosen = i;
			chosenRange = settings.range;
		}
	}
	if (chosen < 0) {
		return;
	}
	ActionState& state = m_actions[chosen];
	state.isPressing = true;
	state.holdTimer = GetActionSettings(chosen).holdSeconds;
	state.waitTimer = GetActionSettings(chosen).interval;
}

void NpcInputSource::ReleaseAll()
{
	for (ActionState& state : m_actions) {
		state.isPressing = false;
	}
}

void NpcInputSource::ApplyButtons(CharacterInput& input) const
{
	input.attack.Update(m_actions[ACTION_ATTACK].isPressing);
	input.fire.Update(m_actions[ACTION_FIRE].isPressing);
	input.subFire.Update(m_actions[ACTION_SUB_FIRE].isPressing);
}
