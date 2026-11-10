#pragma once

#include "BulletBase.h"

// 黒騎士が剣を振って飛ばす斬撃。暗めの赤の三日月形の板が、まっすぐ飛ぶ。
// 斜めの斬撃にするため、進む向きのまわりに SetRoll() の角度だけ回して置く。
// 当たったら（相手・地形・射程切れ）すぐに消える。当たった相手へのダメージはまだない。
class SlashBullet : public BulletBase
{
public:
	SlashBullet();

	// 進む向きのまわりに回す角度（度）。Init() の前に呼ぶ。
	void SetRoll(float rollDeg)
	{
		m_rollDeg = rollDeg;
	}

	void Render(RenderContext& renderContext) override;

protected:
	bool OnStart() override;
	void OnMove() override;
	void OnExplode(const Vector3& hitPosition) override;

private:
	ModelRender m_model;
	float m_rollDeg = 0.0f;
	bool m_isVisible = false;   // 飛んでいる間だけ描く。
};
