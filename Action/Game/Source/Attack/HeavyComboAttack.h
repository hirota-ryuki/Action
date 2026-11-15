#pragma once

#include "AttackBase.h"

// 黒騎士の攻撃2。その場で、片手剣の重い連続斬り（KinemIQ の Sword - heavy combo）を振る。
class HeavyComboAttack : public AttackBase
{
protected:
	const AttackData& GetData() const override;
};
