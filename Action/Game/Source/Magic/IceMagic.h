#pragma once

#include "MagicBase.h"
#include "MagicSettings.h"

// 氷魔法。発射ボタンを押した瞬間に、当たった相手を凍らせる氷の弾（IceBullet）を1発撃つ。
class IceMagic : public MagicBase
{
public:
	// settings: 強さ。ふつうは MagicSettings::GetIce()（magic.json の値）を渡す。
	explicit IceMagic(const IceMagicSettings& settings);

	const char* GetName() const override
	{
		return "IceMagic";
	}
	void OnPress(const MagicCastContext& context) override;

private:
	IceMagicSettings m_settings;
};
