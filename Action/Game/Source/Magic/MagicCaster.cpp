#include "stdafx.h"
#include "MagicCaster.h"
#include "CharacterInput.h"
#include "GameClock.h"

void MagicCaster::Init(IGameObject* owner, Faction faction)
{
	m_owner = owner;
	m_faction = faction;
}

MagicCaster::Slot& MagicCaster::GetOrAddSlot(int slot)
{
	if (slot >= static_cast<int>(m_slots.size())) {
		m_slots.resize(slot + 1);
	}
	return m_slots[slot];
}

bool MagicCaster::IsReady(const Slot& slot) const
{
	return !slot.hasFired || GameClock::Now() - slot.lastFireTime >= slot.cooldown;
}

void MagicCaster::Fire(int slot, const ButtonState& button, const Vector3& origin, const Vector3& direction)
{
	if (slot < 0 || slot >= static_cast<int>(m_slots.size())) {
		return;
	}
	Slot& target = m_slots[slot];
	if (!target.create) {
		return;
	}

	// 押した瞬間: クールダウンが明けていれば、魔法を1つ作る。
	if (button.isTrigger && target.holding == nullptr && IsReady(target)) {
		MagicBase* magic = target.create();
		magic->Init(origin, direction, m_owner, m_faction);
		if (magic->IsChargeType()) {
			// 溜め撃ちは、離して飛ばしたときからクールダウンを数える。
			target.holding = magic;
		}
		else {
			target.lastFireTime = GameClock::Now();
			target.hasFired = true;
		}
	}

	if (target.holding == nullptr) {
		return;
	}
	if (button.isPress) {
		// 押している間: 待たせている魔法を手元に合わせる。
		target.holding->Hold(origin, direction);
	}
	else {
		// 離した: 飛ばして、手を放す。
		target.holding->Release();
		target.holding = nullptr;
		target.lastFireTime = GameClock::Now();
		target.hasFired = true;
	}
}

void MagicCaster::CancelAll()
{
	for (Slot& slot : m_slots) {
		if (slot.holding != nullptr) {
			slot.holding->Cancel();
			slot.holding = nullptr;
		}
	}
}
