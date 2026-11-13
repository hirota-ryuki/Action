#pragma once

#include "BulletBase.h"
#include "MagicSettings.h"

// 炎魔法の弾。着弾すると「ぼわっ」と燃え広がり、足元の床をしばらく燃やす（BurningGround）。
// 燃える床に入った敵は燃焼状態になる。溜め具合が大きいほど、弾が大きく、燃える床が広くなる。
class FireBullet : public BulletBase
{
public:
	FireBullet();
	~FireBullet() override;

	// 強さと溜め具合（0〜1）。NewGO 直後、Start() より前に呼ぶ。
	void Setup(const FireMagicSettings& settings, float chargeRate);

	// 炎魔法のエフェクト（弾・溜め・着弾・燃える床）を登録する。読み込めなければ false。
	// 登録済みなら何もしないので、何度呼んでもよい。
	static bool RegisterEffects();
	// 溜め具合に応じた弾のエフェクトの拡大率。溜め中の見た目（FireMagic）と揃えるのに使う。
	static Vector3 GetBulletEffectScale(float chargeRate);

protected:
	bool OnStart() override;
	void OnMove() override;
	void OnHitTarget(HitTarget& target, const Vector3& hitPosition) override;
	void OnExplode(const Vector3& hitPosition) override;
	bool IsVanishFinished() const override;

private:
	void SpawnBurningGround(const Vector3& hitPosition);

	Effect m_bulletEffect;
	Effect m_hitEffect;
	FireMagicSettings m_settings;
	Vector3 m_hitFootPosition = Vector3::Zero;  // 当たった相手の足元。燃える床を置く場所に使う。
	float m_chargeRate = 0.0f;
	bool m_hasHitTarget = false;   // 相手に当たったか（地形・射程切れなら false）。
	bool m_isEffectReady = false;  // エフェクトを読み込めたか。
};
