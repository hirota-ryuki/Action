#pragma once

#include "MagicBase.h"

// 氷魔法。発射ボタンを押した瞬間に、当たった相手を凍らせる氷の弾（IceBullet）を1発撃つ。
class IceMagic : public MagicBase
{
public:
	// cooldown: 連射間隔（秒）。
	explicit IceMagic(float cooldown = 0.35f);

	const char* GetName() const override
	{
		return "IceMagic";
	}
	void OnPress(const MagicCastContext& context) override;

private:
	float m_cooldownSeconds = 0.0f;
};
