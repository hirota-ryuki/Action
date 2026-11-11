#pragma once

#include "Item.h"

// 回復のポーション。拾うと、プレイヤーのポーションの数が1つ増える（飲むのは PlayerDrinkState）。
class Potion : public Item
{
public:
	const wchar_t* GetName() const override
	{
		return L"ポーション";
	}
	const char* GetIconPath() const override
	{
		return "Assets/sprite/item/potion.dds";
	}
	void OnPickUp(PlayerStatus& status) override;

protected:
	ItemLook MakeLook() const override;
};
