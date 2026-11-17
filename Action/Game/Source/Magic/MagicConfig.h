#pragma once

#include "MagicBase.h"

// 氷魔法の調整値。
struct IceMagicConfig
{
	float cooldown = 0.0f;       // 連射間隔（秒）。
	MagicSettings settings;      // 速さ・射程・当たり判定の半径。
	float freezeSeconds = 0.0f;  // 当たった相手を凍らせる秒数。
};

// 炎魔法の調整値。min〜max は、溜めなし〜最大まで溜めたときの値。
struct FireMagicConfig
{
	float cooldown = 0.0f;           // 撃ってから次に溜め始められるまでの秒数。
	float maxChargeSeconds = 0.0f;   // 最大まで溜まる秒数。
	float speed = 0.0f;
	float range = 0.0f;
	float minRadius = 0.0f;          // 当たり判定の半径。
	float maxRadius = 0.0f;
	float minGroundRadius = 0.0f;    // 炎魔法発射後の燃える床の半径。
	float maxGroundRadius = 0.0f;
	float groundBurnSeconds = 0.0f;  // 床が燃えている秒数。
};

// 黒騎士の斬撃の調整値。
struct SlashMagicConfig
{
	MagicSettings settings;  // 速さ・射程・当たり判定の半径。
};

// 魔法の調整値。Assets/data/magic.json を最初に呼ばれたときに1回だけ読む。Player も敵も同じ値を使う。
namespace MagicConfig
{
	const IceMagicConfig& GetIce();
	const FireMagicConfig& GetFire();
	const SlashMagicConfig& GetSlash();
}
