#include "stdafx.h"
#include "PlayerAnimator.h"

PlayerAnimator::PlayerAnimator()
{
	ClearNext();
}

void PlayerAnimator::Init(ModelRender* modelRender, float defaultBlendSeconds)
{
	m_modelRender = modelRender;
	m_defaultBlendSeconds = defaultBlendSeconds;
	ClearNext();
	m_runTime = 0.0f;
}

void PlayerAnimator::Play(int clip, float speed)
{
	const float blend = m_nextBlendSeconds >= 0.0f ? m_nextBlendSeconds : m_defaultBlendSeconds;
	const float startTime = GetNextStartTime(clip);
	ClearNext();
	if (m_modelRender == nullptr) {
		return;
	}
	if (startTime >= 0.0f) {
		m_modelRender->PlayAnimationFromTime(clip, blend, startTime);
	}
	else {
		m_modelRender->PlayAnimation(clip, blend);
	}
	m_modelRender->SetAnimationSpeed(speed);
}

void PlayerAnimator::PlayFrom(int clip, float blendSeconds, float startTime, float speed)
{
	if (m_modelRender == nullptr) {
		return;
	}
	m_modelRender->PlayAnimationFromTime(clip, blendSeconds, startTime);
	m_modelRender->SetAnimationSpeed(speed);
}

void PlayerAnimator::SetNextStartTime(int clip, float time)
{
	if (clip >= 0 && clip < enPlayerAnimClip_Num) {
		m_nextStartTimes[clip] = time;
	}
}

float PlayerAnimator::GetNextStartTime(int clip) const
{
	if (clip >= 0 && clip < enPlayerAnimClip_Num) {
		return m_nextStartTimes[clip];
	}
	return -1.0f;
}

void PlayerAnimator::ClearNext()
{
	m_nextBlendSeconds = -1.0f;
	for (float& time : m_nextStartTimes) {
		time = -1.0f;
	}
}
