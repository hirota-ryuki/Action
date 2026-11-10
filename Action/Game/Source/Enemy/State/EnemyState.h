#pragma once

#include "State.h"
#include "Faction.h"

class CharacterMover;
class CharacterModel;
class StatusEffects;

// 敵のアニメーションクリップ。CharacterModelDesc::clips の添字になる。
enum EnEnemyAnimClip
{
	enEnemyAnimClip_Idle,
	enEnemyAnimClip_Walk,
	enEnemyAnimClip_Run,
	enEnemyAnimClip_Attack1,  // 突進して突き → 回転斬り
	enEnemyAnimClip_Attack2,  // 左上→右下、右上→左下と斬り下ろし → 横一閃
	enEnemyAnimClip_Attack3,  // 斜めの斬撃を5つ飛ばす
	enEnemyAnimClip_Num,
};

// 敵のステートの種類。
enum class EnemyStateId
{
	Idle,    // 立ち止まっている
	Move,    // 入力どおりに歩いている
	Run,     // 入力どおりに走っている（AI が isRun を立てたとき。剣を肩に乗せて走る）
	Attack,  // 剣で攻撃している（AI が fire を押したときに、attackIndex の攻撃をする）
	Frozen,  // 凍結などで行動できない
	Num,
};

// ステートの調整用の数値。歩く速さなどは CharacterMoveSettings にある。
struct EnemyStateSettings
{
	float moveThreshold = 0.1f;     // 移動入力の長さがこれより小さければ立ち止まっているとみなす。
	float animBlendSeconds = 0.2f;  // アニメーションを切り替えるときの補間時間。
	float walkAnimSpeed = 1.0f;     // Walk の再生の速さ。歩く速さと、Walk の足が地面を送る速さの比にすると、足が滑らない。
};

// ステートが触ってよいもの。ステートは Enemy クラスを直接は知らない。
struct EnemyStateContext
{
	CharacterMover* mover = nullptr;
	CharacterModel* model = nullptr;
	const StatusEffects* statusEffects = nullptr;
	const IGameObject* owner = nullptr;  // 自分。飛ばした斬撃が自分に当たらないように使う。
	Faction faction = Faction::Enemy;    // 飛ばした斬撃が当たる相手を決める。
	EnemyStateSettings settings;
	// ひとつ前のステート。StateMachine が入れる。
	EnemyStateId previousId = EnemyStateId::Idle;
};

// 敵のステートの基底クラス。中身は共通の State（Character/StateMachine/State.h）にある。
using EnemyState = State<EnemyStateId, EnemyStateContext>;
