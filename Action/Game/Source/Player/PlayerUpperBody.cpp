#include "stdafx.h"
#include "PlayerUpperBody.h"
#include "PlayerAnimator.h"

namespace
{
	// 上半身に流すクリップ。3段攻撃の1段目（右手で撃つ）を、途中で止めて溜めに使う。
	constexpr int CHARGE_CLIP = enPlayerAnimClip_Attack1;
}

void PlayerUpperBody::Init(ModelRender* modelRender, const PlayerChargeSettings& settings)
{
	m_modelRender = modelRender;
	m_settings = settings;
	Apply();
}

void PlayerUpperBody::Update(bool isCharging, int useCount, bool isEnabled, float deltaTime)
{
	// 層は、前のフレームに決めた速さで、モデルの更新の中で進んでいる。
	m_time += deltaTime * m_speed;

	if (!isEnabled) {
		m_phase = Phase::None;
		m_weight = 0.0f;
	}
	else {
		switch (m_phase) {
		case Phase::None:
			if (isCharging) {
				StartCharge();
			}
			break;
		case Phase::Charge:
			if (!isCharging) {
				// 撃った回数が増えていれば、離して撃った。増えていなければ、溜めをやめた（凍ったなど）。
				m_phase = useCount != m_lastUseCount ? Phase::Cast : Phase::None;
			}
			break;
		case Phase::Cast:
			if (isCharging) {
				StartCharge();
			}
			else if (m_time >= m_settings.castEndTime) {
				m_phase = Phase::None;
			}
			break;
		}
		// 重みを少しずつ変える。
		const bool isActive = m_phase != Phase::None;
		const float seconds = isActive ? m_settings.fadeInSeconds : m_settings.fadeOutSeconds;
		const float step = seconds > 0.0f ? deltaTime / seconds : 1.0f;
		m_weight = isActive ? min(1.0f, m_weight + step) : max(0.0f, m_weight - step);
	}
	m_lastUseCount = useCount;

	// 溜めている間は、止める位置まで来たら止める。それ以外は流す。
	m_speed = (m_phase == Phase::Charge && m_time >= m_settings.holdTime) ? 0.0f : 1.0f;
	Apply();
}

void PlayerUpperBody::Stop()
{
	m_phase = Phase::None;
}

void PlayerUpperBody::StartCharge()
{
	m_phase = Phase::Charge;
	m_time = 0.0f;
	m_modelRender->PlayLayerAnimationFromTime(CHARGE_CLIP, 0.0f, 0.0f);
}

void PlayerUpperBody::Apply()
{
	// 入る・戻るの始めと終わりをゆっくりにする（smoothstep）。
	const float smoothWeight = m_weight * m_weight * (3.0f - 2.0f * m_weight);
	m_modelRender->SetLayerWeight(smoothWeight);
	m_modelRender->SetLayerAnimationSpeed(m_speed);
}
