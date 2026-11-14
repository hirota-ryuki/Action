#pragma once

#include "MagicBase.h"

// 氷魔法。押した瞬間に飛び出し、当たった相手を一定時間凍らせる。
// 着弾後も軌跡と着弾エフェクトが消え切るまで残る。
class IceMagic : public MagicBase
{
public:
	static constexpr float COOLDOWN = 0.35f;  // 連射間隔（秒）。

	IceMagic();
	~IceMagic() override;

protected:
	bool OnStart() override;
	void OnMove() override;
	void OnHitTarget(HitTarget& target, const Vector3& hitPosition) override;
	void OnExplode(const Vector3& hitPosition) override;
	bool IsVanishFinished() const override;

private:
	Effect m_flyEffect;
	Effect m_hitEffect;
	bool m_isEffectReady = false;  // エフェクトを読み込めたか。
};
