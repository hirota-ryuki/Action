#pragma once

#include "AttackBase.h"

// 黒騎士の攻撃1。左腰に剣を構えてかがみ、そこで溜めてから、一気に踏み込んで横に斬り抜ける（KinemIQ の dash slash）。
// アニメーションの中で腰が進む分（約3.7m）だけ、黒騎士ごと前へ動かす。
class DashSlashAttack : public AttackBase
{
public:
	// 構え（左腰に剣を構えてかがむ）のコマで止めて溜める秒数。
	static constexpr float CHARGE_SECONDS = 0.6f;

protected:
	const AttackData& GetData() const override;
};
