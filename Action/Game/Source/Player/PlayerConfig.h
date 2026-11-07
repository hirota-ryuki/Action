#pragma once

#include "Character.h"
#include "PlayerState.h"
#include "WeaponAttachment.h"

// プレイヤーの調整値。数値とモデルのパスだけを置き、処理は書かない。
namespace PlayerConfig
{
	// 移動・モデル・魔法を撃つ位置・当たり判定・HP。
	CharacterConfig MakeCharacterConfig();
	// ステート（ダッシュ・止まるなど）の数値。
	PlayerStateSettings MakeStateSettings();
	// 右手に持つ剣。
	WeaponAttachmentDesc MakeSwordDesc();
	// この高さを下回るとステージの外と判断して、最初の位置へ戻す。
	constexpr float RESPAWN_HEIGHT = -1000.0f;
}
