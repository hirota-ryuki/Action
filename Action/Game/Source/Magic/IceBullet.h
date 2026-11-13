#pragma once

#include "BulletBase.h"
#include "MagicSettings.h"

// 氷魔法の弾。当たった相手を一定時間凍らせる。
// 着弾後も軌跡と着弾エフェクトが消え切るまで残る。
class IceBullet : public BulletBase
{
public:
	IceBullet();
	~IceBullet() override;

	// 強さ（速さ・射程・凍らせる秒数など）。NewGO 直後、Start() より前に呼ぶ。
	void Setup(const IceMagicSettings& settings);

protected:
	bool OnStart() override;
	void OnMove() override;
	void OnHitTarget(HitTarget& target, const Vector3& hitPosition) override;
	void OnExplode(const Vector3& hitPosition) override;
	bool IsVanishFinished() const override;

private:
	Effect m_bulletEffect;
	Effect m_hitEffect;
	float m_freezeSeconds = 0.0f;  // 当たった相手を凍らせる秒数。
	bool m_isEffectReady = false;  // エフェクトを読み込めたか。
};
