#include "stdafx.h"
#include "ItemManager.h"

ItemManager::~ItemManager()
{
	for (Item* item : m_items) {
		DeleteGO(item);
	}
	m_items.clear();
}

void ItemManager::FindInRange(const Vector3& position, float radius, std::vector<Item*>& out) const
{
	out.clear();
	const float radiusSq = radius * radius;
	for (Item* item : m_items) {
		const Vector3 diff = item->GetPosition() - position;
		if (diff.LengthSq() <= radiusSq) {
			out.push_back(item);
		}
	}
	std::sort(out.begin(), out.end(), [&position](const Item* a, const Item* b) {
		return (a->GetPosition() - position).LengthSq() < (b->GetPosition() - position).LengthSq();
	});
}

void ItemManager::PickUp(Item* item, PlayerStatus& status)
{
	auto it = std::find(m_items.begin(), m_items.end(), item);
	if (it == m_items.end()) {
		return;
	}
	m_items.erase(it);
	item->OnPickUp(status);
	DeleteGO(item);
}
