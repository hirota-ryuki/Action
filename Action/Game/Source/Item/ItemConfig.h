#pragma once

#include "Item.h"
#include "PlayerState.h"

// アイテムの調整値。Assets/data/item.json を読んで、構造体に詰めて返す（処理は書かない）。
// JSON のキー名を知っているのは、このファイルだけにする。
namespace ItemConfig
{
	// ポーションの見た目。
	ItemLook MakePotionLook();
	// プレイヤーが拾う・飲むときの数値。
	PlayerItemSettings MakePlayerItemSettings();
	// ゲームを始めたときに、ステージに置くポーションの位置（y は地面に合わせるので何でもよい）。
	std::vector<Vector3> MakePotionSpawnPositions();
}
