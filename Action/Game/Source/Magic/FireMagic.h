#pragma once

#include "MagicBase.h"
#include "MagicSettings.h"

// 炎魔法。発射ボタンを押している間に溜め、離すと炎の弾（FireBullet）を撃つ。
// 長く溜めるほど弾が大きくなり、着弾後に燃える床が広くなる。
// 溜めている間は、撃つ位置に溜め具合に応じて大きくなる火の玉を出し、周りから炎を吸い込ませる。
// 最大まで溜まると閃光と衝撃波を出し、その後は火の玉が脈打つ。
class FireMagic : public MagicBase
{
public:
	// settings: 強さ。ふつうは MagicSettings::GetFire()（magic.json の値）を渡す。
	explicit FireMagic(const FireMagicSettings& settings);
	~FireMagic() override;

	const char* GetName() const override
	{
		return "FireMagic";
	}
	void OnPress(const MagicCastContext& context) override;
	void OnHold(const MagicCastContext& context) override;
	void OnRelease(const MagicCastContext& context) override;
	void OnCancel() override;

private:
	float GetChargeRate() const;
	void UpdateChargeEffect(const MagicCastContext& context);
	void StopCharge();

	Effect m_chargeEffect;   // 溜め中の火の玉。
	Effect m_chargedEffect;  // 最大まで溜まった瞬間の閃光。
	FireMagicSettings m_settings;
	float m_chargeSeconds = 0.0f;  // ここまでに溜めた秒数。
	float m_fullElapsed = 0.0f;    // 最大まで溜まってからの秒数。脈打ちに使う。
	bool m_isCharging = false;
	bool m_isFullCharged = false;
	bool m_isEffectReady = false;  // 溜め中のエフェクトを読み込めたか。
};
