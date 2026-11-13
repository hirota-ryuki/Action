#include "stdafx.h"
#include "IceMagic.h"
#include "IceBullet.h"

IceMagic::IceMagic(const IceMagicSettings& settings)
	: m_settings(settings)
{
}

void IceMagic::OnPress(const MagicCastContext& context)
{
	if (!IsReady()) {
		return;
	}
	IceBullet* bullet = NewGO<IceBullet>(1, BulletBase::OBJECT_NAME);
	bullet->Setup(m_settings);
	bullet->Init(context.origin, context.direction, context.caster, context.faction);
	StartCooldown(m_settings.cooldown);
}
