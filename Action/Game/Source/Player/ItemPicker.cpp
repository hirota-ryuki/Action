#include "stdafx.h"
#include "ItemPicker.h"
#include "ItemManager.h"

void ItemPicker::Update(const Vector3& footPosition)
{
	if (m_manager == nullptr) {
		m_candidates.clear();
		return;
	}
	m_manager->FindInRange(footPosition, m_radius, m_candidates);
}

void ItemPicker::PickUpFirst(PlayerStatus& status)
{
	if (m_manager == nullptr || m_candidates.empty()) {
		return;
	}
	Item* item = m_candidates.front();
	m_candidates.erase(m_candidates.begin());
	m_manager->PickUp(item, status);
}
