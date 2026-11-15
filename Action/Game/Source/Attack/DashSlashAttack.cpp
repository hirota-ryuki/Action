#include "stdafx.h"
#include "DashSlashAttack.h"
#include "EnemyState.h"

namespace
{
	constexpr float ANIM_FPS = 30.0f;

	// 腰が前へ進んだ距離（黒騎士のモデルの m）。30コマで1秒、0コマ目が 0。
	// retarget_black_knight.py が "in_place" で腰の水平の動きを消すときに出す「ROOT [Attack1] … コマごと」の値。
	// 7〜12コマ目で一気に 3m 踏み込み、そのあと少しずつ止まる。ゲームはこの分だけ黒騎士ごと前へ動かす。
	constexpr float ROOT[] = {
		0.00f, 0.01f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.85f, 1.66f, 2.33f,
		2.80f, 3.05f, 3.19f, 3.30f, 3.40f, 3.47f, 3.53f, 3.58f, 3.61f, 3.63f,
		3.65f, 3.65f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f,
		3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.68f, 3.70f, 3.71f, 3.72f,
		3.72f, 3.72f, 3.72f, 3.72f, 3.72f, 3.72f, 3.72f, 3.72f,
	};
	constexpr int ROOT_NUM = sizeof(ROOT) / sizeof(ROOT[0]);

	// Attack1 の 5.5コマ目（左腰に剣を構えてかがむ）で止めて溜める。
	const AttackData DATA = { enEnemyAnimClip_Attack1, 5.5f / ANIM_FPS, DashSlashAttack::CHARGE_SECONDS, ROOT, ROOT_NUM };
}

const AttackData& DashSlashAttack::GetData() const
{
	return DATA;
}
