#pragma once

#include "MagicBase.h"

// 炎魔法。押している間は撃つ位置で溜め、離すと飛んでいく。
// 溜めている間は、撃つ位置に溜め具合に応じて大きくなる火の玉を出し、周りから炎を吸い込ませる。
// 最大まで溜まると閃光と衝撃波を出し、その後は火の玉が脈打つ。
// 着弾すると「ぼわっ」と燃え広がり、足元の床をしばらく燃やす（BurningGround）。
// 長く溜めるほど、飛ぶ炎が大きく、燃える床が広くなる。
class FireMagic : public MagicBase
{
public:
	static constexpr float COOLDOWN = 0.6f;  // 撃ってから次に溜め始められるまでの秒数。

	FireMagic();
	~FireMagic() override;

	bool IsChargeType() const override
	{
		return true;
	}

	// 炎魔法のエフェクト（飛ぶ炎・溜め・着弾・燃える床）を登録する。読み込めなければ false。
	// 登録済みなら何もしないので、何度呼んでもよい。
	static bool RegisterEffects();

protected:
	bool OnStart() override;
	void OnCharge(float chargeSeconds) override;
	void OnLaunch() override;
	void OnCancel() override;
	void OnMove() override;
	void OnHitTarget(HitTarget& target, const Vector3& hitPosition) override;
	void OnExplode(const Vector3& hitPosition) override;
	bool IsVanishFinished() const override;

private:
	void SpawnBurningGround(const Vector3& hitPosition);

	Effect m_chargeEffect;   // 溜め中の火の玉。
	Effect m_chargedEffect;  // 最大まで溜まった瞬間の閃光。
	Effect m_flyEffect;      // 飛んでいる炎。
	Effect m_hitEffect;      // 着弾。
	Vector3 m_hitFootPosition = Vector3::Zero;  // 当たった相手の足元。燃える床を置く場所に使う。
	float m_chargeRate = 0.0f;       // 溜め具合（0〜1）。
	float m_fullChargeSeconds = 0.0f;  // 最大まで溜まったときの、押していた秒数。脈打ちに使う。
	bool m_isFullCharged = false;
	bool m_hasHitTarget = false;     // 相手に当たったか（地形・射程切れなら false）。
	bool m_isEffectReady = false;    // エフェクトを読み込めたか。
};
