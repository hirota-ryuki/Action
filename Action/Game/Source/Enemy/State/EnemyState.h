#pragma once

#include "State.h"

class CharacterMover;
class CharacterModel;
class StatusEffects;

// 敵のアニメーションクリップ。CharacterModelDesc::clips の添字になる。
enum EnEnemyAnimClip
{
	enEnemyAnimClip_Idle,
	enEnemyAnimClip_Walk,
	enEnemyAnimClip_Attack,
	enEnemyAnimClip_Num,
};

// 敵のステートの種類。
enum class EnemyStateId
{
	Idle,    // 立ち止まっている
	Move,    // 入力どおりに進んでいる
	Attack,  // 剣を振っている（AI が fire を押したときに振る）
	Frozen,  // 凍結などで行動できない
	Num,
};

// ステートの調整用の数値。歩く速さなどは CharacterMoveSettings にある。
struct EnemyStateSettings
{
	float moveThreshold = 0.1f;     // 移動入力の長さがこれより小さければ立ち止まっているとみなす。
	float animBlendSeconds = 0.2f;  // アニメーションを切り替えるときの補間時間。
};

// ステートが触ってよいもの。ステートは Enemy クラスを直接は知らない。
struct EnemyStateContext
{
	CharacterMover* mover = nullptr;
	CharacterModel* model = nullptr;
	const StatusEffects* statusEffects = nullptr;
	EnemyStateSettings settings;
	// ひとつ前のステート。StateMachine が入れる。
	EnemyStateId previousId = EnemyStateId::Idle;
};

// 敵のステートの基底クラス。中身は共通の State（Character/StateMachine/State.h）にある。
using EnemyState = State<EnemyStateId, EnemyStateContext>;
