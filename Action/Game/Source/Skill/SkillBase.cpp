#include "stdafx.h"
#include "SkillBase.h"
#include "CharacterInput.h"
#include "GameClock.h"

void SkillBase::Init(const Vector3& position, const Vector3& direction, const IGameObject* owner, Faction faction)
{
	SetPose(position, direction);
	m_owner = owner;
	m_faction = faction;
	m_state = EnState::Charging;
	m_chargeStartTime = GameClock::Now();
	// 溜めないスキルは、作った時点で離したのと同じ扱いにする。
	m_isReleased = !IsChargeType();
}

void SkillBase::ReceiveInput(const ButtonState& button, const Vector3& position, const Vector3& direction)
{
	if (!IsOwnerBusy()) {
		// 溜めていなければ、そのあとの入力は関係ない。
		return;
	}
	if (button.isPress) {
		// 押している間: 撃つ位置と向きを手元に合わせる。
		SetPose(position, direction);
	}
	else {
		// 離した: 次の Update で使い始める。
		m_isReleased = true;
	}
}

void SkillBase::Cancel()
{
	if (IsOwnerBusy()) {
		m_isCanceled = true;
	}
}

bool SkillBase::IsOwnerBusy() const
{
	return m_state == EnState::Charging && !m_isReleased && !m_isCanceled;
}

bool SkillBase::Start()
{
	m_isValid = OnStart();
	if (!m_isValid) {
		return true;
	}
	if (m_isReleased && !m_isCanceled) {
		// 溜めないスキル（または Start より前に離された）は、すぐ使い始める。
		Activate();
	}
	else if (IsOwnerBusy()) {
		// 溜めの見た目を最初の位置に合わせておく。
		OnCharge(0.0f);
	}
	return true;
}

void SkillBase::Update()
{
	if (!m_isValid) {
		// 溜めている間は SkillController がこのスキルを指しているので、離されるか中断されるまで消えない。
		if (IsOwnerBusy()) {
			return;
		}
		DeleteGO(this);
		return;
	}

	switch (m_state) {
	case EnState::Charging:
		UpdateCharging();
		break;
	case EnState::Active:
		OnSkillUpdate();
		break;
	case EnState::Ending:
		if (IsVanishFinished()) {
			DeleteGO(this);
		}
		break;
	}
}

void SkillBase::End()
{
	m_state = EnState::Ending;
}

void SkillBase::SetPose(const Vector3& position, const Vector3& direction)
{
	m_position = position;
	m_direction = direction;
	if (m_direction.LengthSq() <= FLT_EPSILON) {
		m_direction = Vector3::AxisZ;
	}
	m_direction.Normalize();
	m_rotation.SetRotationYFromDirectionXZ(m_direction);
}

void SkillBase::UpdateCharging()
{
	if (m_isCanceled) {
		OnCancel();
		End();
		return;
	}
	if (m_isReleased) {
		Activate();
		return;
	}
	OnCharge(GameClock::Now() - m_chargeStartTime);
}

void SkillBase::Activate()
{
	OnRelease();
	m_state = EnState::Active;
	OnActivate();
}
