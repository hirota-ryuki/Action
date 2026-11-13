#pragma once

// 氷魔法の強さ。Player も敵も同じ値を使う。
struct IceMagicSettings
{
	float cooldown = 0.35f;       // 連射間隔（秒）。
	float speed = 1500.0f;        // 弾が毎秒進む距離。
	float range = 2000.0f;        // 弾がこれだけ飛ぶと消える。
	float radius = 10.0f;         // 弾の当たり判定の半径。
	float freezeSeconds = 2.5f;   // 当たった相手を凍らせる秒数。
};

// 炎魔法の強さ。「min〜」は溜めなし、「max〜」は最大まで溜めたときの値で、間は溜め具合で補間する。
struct FireMagicSettings
{
	float cooldown = 0.6f;            // 撃ってから次に溜め始められるまでの秒数。
	float maxChargeSeconds = 1.2f;    // 最大まで溜まる秒数。
	float speed = 1300.0f;            // 弾が毎秒進む距離。
	float range = 2000.0f;            // 弾がこれだけ飛ぶと消える。
	float minRadius = 14.0f;          // 弾の当たり判定の半径。
	float maxRadius = 45.0f;
	float minGroundRadius = 130.0f;   // 燃える床の半径。
	float maxGroundRadius = 300.0f;
	float groundBurnSeconds = 4.0f;   // 床が燃えている秒数。
};

// 魔法の強さを Assets/data/magic.json から読む。最初に呼ばれたときに1回だけ読み、あとは取っておいた値を返す。
// キーがないときは上の構造体に書いてある値を使う。
// 使い方: m_magicCaster.AddMagic(std::make_unique<IceMagic>(MagicSettings::GetIce()));
namespace MagicSettings
{
	const IceMagicSettings& GetIce();
	const FireMagicSettings& GetFire();
}
