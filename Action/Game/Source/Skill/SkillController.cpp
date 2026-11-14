#include "stdafx.h"
#include "SkillController.h"
#include "GameClock.h"

void SkillController::Init(IGameObject* owner, Faction faction)
{
	m_owner = owner;
	m_faction = faction;
}

bool SkillController::IsReady(const Entry& entry) const
{
	return !entry.hasUsed || GameClock::Now() - entry.lastUseTime >= entry.cooldown;
}

void SkillController::Update(const CharacterInput& input, const Vector3& origin, const Vector3& direction)
{
	// 使っている最中なら、そのスキルのボタンだけを渡す（ほかのボタンは受け付けない）。
	if (m_active != nullptr) {
		m_active->ReceiveInput(input.actions[m_activeButton], origin, direction);
		if (!m_active->IsOwnerBusy()) {
			FinishActive();
		}
		return;
	}

	// 登録したボタンが押された瞬間に、クールダウンが明けていれば、そのスキルを作る。
	for (int button = 0; button < static_cast<int>(ActionButton::Num); button++) {
		Entry& entry = m_entries[button];
		const ButtonState& state = input.actions[button];
		if (!entry.create || !state.isTrigger || !IsReady(entry)) {
			continue;
		}
		m_active = entry.create();
		m_activeButton = button;
		m_active->Init(origin, direction, m_owner, m_faction);
		// 溜めないスキルは、ここで手を離す。
		if (!m_active->IsOwnerBusy()) {
			FinishActive();
		}
		return;
	}
}

void SkillController::CancelAll()
{
	if (m_active == nullptr) {
		return;
	}
	// 中断したスキルは、クールダウンを始めない。
	m_active->Cancel();
	m_active = nullptr;
	m_activeButton = -1;
}

void SkillController::FinishActive()
{
	Entry& entry = m_entries[m_activeButton];
	entry.lastUseTime = GameClock::Now();
	entry.hasUsed = true;
	m_active = nullptr;
	m_activeButton = -1;
}
