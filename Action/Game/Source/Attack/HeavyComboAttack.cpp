#include "stdafx.h"
#include "HeavyComboAttack.h"
#include "EnemyState.h"

namespace
{
	// その場で振る（溜めない、進まない）。
	const AttackData DATA = { enEnemyAnimClip_Attack2 };
}

const AttackData& HeavyComboAttack::GetData() const
{
	return DATA;
}
