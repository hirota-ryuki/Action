#pragma once

#include "Item.h"

class PlayerStatus;

// 落ちているアイテムをまとめて持つ。ゲームに1つだけ（Game が作り、Player に渡す）。
// アイテムを置く（Spawn）のも、拾われて消す（PickUp）のもここだけにする。一覧を持つのもここだけ。
// 拾うかどうかは決めない（プレイヤーのステートが決めて、PickUp を呼ぶ）。
class ItemManager : public IGameObject
{
public:
	// NewGO の優先度。キャラクターと同じ 1。
	static constexpr int ITEM_PRIORITY = 1;
	static constexpr const char* ITEM_OBJECT_NAME = "item";

	~ItemManager() override;

	// position の真下の地面に、T（Item の派生クラス）を置く。
	template <class T>
	T* Spawn(const Vector3& position)
	{
		T* item = NewGO<T>(ITEM_PRIORITY, ITEM_OBJECT_NAME);
		item->SetPosition(position);
		m_items.push_back(item);
		return item;
	}

	// position から radius 以内のアイテムを、近い順に out に入れる（out は先に空にする）。
	void FindInRange(const Vector3& position, float radius, std::vector<Item*>& out) const;
	// item の効き目を status に与え、一覧から外して消す。一覧にないものは何もしない。
	void PickUp(Item* item, PlayerStatus& status);

private:
	std::vector<Item*> m_items;
};
