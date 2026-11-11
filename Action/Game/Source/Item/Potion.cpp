#include "stdafx.h"
#include "Potion.h"
#include "ItemConfig.h"
#include "PlayerStatus.h"

void Potion::OnPickUp(PlayerStatus& status)
{
	status.AddPotion(1);
}

ItemLook Potion::MakeLook() const
{
	return ItemConfig::MakePotionLook();
}
