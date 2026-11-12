#pragma once

#include "State.h"
#include "Faction.h"
#include "Status.h"

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
	enEnemyAnimClip_Attack2,  // 片手剣の重い連続斬り
	enEnemyAnimClip_Attack3,  // 斜めの斬撃を5つ飛ばす
	enEnemyAnimClip_Hit,      // やられ（攻撃中以外で当たったとき）
	enEnemyAnimClip_Dead,     // 死ぬ
	enEnemyAnimClip_Num,
};

// 敵のステートの種類。
enum class EnemyStateId
{
	Idle,    // 立ち止まっている
	Move,    // 入力どおりに歩いている
	Run,     // 入力どおりに走っている（AI が isRun を立てたとき。剣を肩に乗せて走る）
	Attack,  // 剣で攻撃している（AI が fire を押したときに、attackIndex の攻撃をする）
	Hit,     // 攻撃を受けてひるんでいる（攻撃中は受けてもひるまない）
	Dead,    // HP がなくなって倒れた。倒れ終わって少ししたら消える
	Frozen,  // 凍結などで行動できない
	Num,
};

// ステートの調整用の数値。歩く速さなどは CharacterMoveSettings にある。
struct EnemyStateSettings
{
	float moveThreshold = 0.1f;     // 移動入力の長さがこれより小さければ立ち止まっているとみなす。
	float animBlendSeconds = 0.2f;  // アニメーションを切り替えるときの補間時間。
	float walkAnimSpeed = 1.0f;     // Walk の再生の速さ。歩く速さと、Walk の足が地面を送る速さの比にすると、足が滑らない。
	float playSpeed = 1.0f;         // 黒騎士の動き全体の速さ（アニメーションと移動の両方に掛ける）。動きを確かめるときに下げる。
	float vanishDelay = 2.0f;       // 倒れ終わってから消えるまでの秒数。
	float rootMoveScale = 100.0f;   // アニメーションの中で進む距離（モデルの m）を、ゲームの距離にする倍率（100 × モデルの大きさ）。
};

// ゲームを動かしたまま変えたい調整値（F1 の画面で変える）。持ち主が持ち、ステートはポインタで読む。
struct EnemyTuning
{
	float attack1ChargeSeconds = 0.6f;  // 攻撃1で、構え（左腰に剣を構えてかがむ）のコマで止めて溜める秒数。
};

// 攻撃を受けたこと・死んだことを、持ち主（Enemy）とステートの間で受け渡す。持ち主が持ち、ステートはポインタで触る。
struct EnemyDamageState
{
	const Status* status = nullptr;  // HP を見る。
	bool isHitRequested = false;     // 当たった（持ち主が立て、ステートが読んだら下ろす）。
	bool isVanished = false;         // 倒れ終わって消えた（Dead ステートが立てる）。

	bool IsDead() const
	{
		return status != nullptr && status->IsHpEmpty();
	}
	// 当たったかを返し、下ろす。
	bool ConsumeHit()
	{
		const bool hit = isHitRequested;
		isHitRequested = false;
		return hit;
	}
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
	EnemyDamageState* damage = nullptr;  // 当たった・死んだ。持ち主が持つ。
	const EnemyTuning* tuning = nullptr; // F1 の画面で変える調整値。持ち主が持つ。
	// ひとつ前のステート。StateMachine が入れる。
	EnemyStateId previousId = EnemyStateId::Idle;
};

// 敵のステートの基底クラス。中身は共通の State（Character/StateMachine/State.h）にある。
using EnemyState = State<EnemyStateId, EnemyStateContext>;
