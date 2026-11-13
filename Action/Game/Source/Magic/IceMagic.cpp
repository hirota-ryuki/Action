#include "stdafx.h"
#include "IceMagic.h"
#include "IceBullet.h"

IceMagic::IceMagic(float cooldown)
	: m_cooldownSeconds(cooldown)
{
}

void IceMagic::OnPress(const MagicCastContext& context)
{
	if (!IsReady()) {
		return;
	}
	IceBullet* bullet = NewGO<IceBullet>(1, BulletBase::OBJECT_NAME);
	bullet->Init(context.origin, context.direction, context.caster, context.faction);
	StartCooldown(m_cooldownSeconds);
	NotifyCast();
}
