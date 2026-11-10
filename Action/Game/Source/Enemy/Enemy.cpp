#include "stdafx.h"
#include "Enemy.h"

namespace
{
	constexpr float HIT_RADIUS = 60.0f;         // 弾に当たる球の半径。黒騎士の胴（肩当てまで）を覆う。
	constexpr float HIT_CENTER_HEIGHT = 110.0f; // 足元から当たり判定の中心までの高さ（黒騎士の胸の下あたり）。

	CharacterMoveSettings MakeMoveSettings()
	{
		CharacterMoveSettings settings;
		settings.walkSpeed = 140.0f;          // 余裕たっぷりにゆっくり歩く。Walk の足が地面を送る速さ（約145）に合わせる。
		settings.runSpeedMultiplier = 2.75f;  // 走るときは 385。Run の足が地面を送る速さ（約384）に合わせる。
		settings.gravity = 980.0f;
		settings.capsuleRadius = 30.0f;   // 黒騎士は肩当てが広いので、プレイヤー（20）より太くする。
		settings.capsuleHeight = 140.0f;
		return settings;
	}

	BlackKnightInputSource::Settings MakeAiSettings()
	{
		BlackKnightInputSource::Settings settings;
		settings.searchRange = 900.0f;     // この距離以内に入ると追いかけ始める（ここまでが「遠い」）。
		settings.nearRange = 220.0f;       // この距離以内が「近い」。剣を振る（剣の長さ＋踏み込み）。
		settings.midRange = 600.0f;        // この距離以内が「中くらい」。
		settings.stopDistance = 150.0f;    // この距離まで近づいたら止まる（剣が届くところで止まる）。
		settings.decideInterval = 2.0f;    // 歩いている間、走るかを決め直す間隔（秒）。
		settings.runChance = 0.5f;         // 中くらい・遠いときに走る確率。残りは攻撃（攻撃1・3ができるまでは歩き続ける）。
		settings.attackInterval = 2.5f;    // 近いときに剣を振る間隔（秒）。振るのに 1.1秒かかる。
		settings.firstAttackDelay = 0.6f;  // 相手を見つけてから最初に振れるまでの秒数。
		return settings;
	}

	CharacterModelDesc MakeModelDesc()
	{
		CharacterModelDesc desc;
		// 黒騎士。体は Meshy で作り、骨格を SK_Player に合わせる（tools/BlenderScripts/make_black_knight_meshy.py で作る）。
		// アニメーションは既存のモーションを黒騎士の骨格へ移したもの（tools/BlenderScripts/retarget_black_knight.py で作る）。
		// Attack は、攻撃1〜3のモーションができるまでの仮（Kevin Iglesias の片手剣の攻撃）。
		desc.modelPath = "Assets/modelData/enemy/BlackKnight/BlackKnight.fbx";
		desc.upAxis = enModelUpAxisZ;
		desc.scale = 1.3f;   // プレイヤー（1.128）より一回り大きくする。
		desc.clips.resize(enEnemyAnimClip_Num);
		desc.clips[enEnemyAnimClip_Idle] = { "Assets/modelData/enemy/BlackKnight/Animation/Idle.fbx", true };
		desc.clips[enEnemyAnimClip_Walk] = { "Assets/modelData/enemy/BlackKnight/Animation/Walk.fbx", true };
		desc.clips[enEnemyAnimClip_Run] = { "Assets/modelData/enemy/BlackKnight/Animation/Run.fbx", true };
		desc.clips[enEnemyAnimClip_Attack] = { "Assets/modelData/enemy/BlackKnight/Animation/Attack.fbx", false };
		return desc;
	}

}

Enemy::Enemy()
	: Character(Faction::Enemy)
	, m_aiInputSource(MakeAiSettings())
{
}

CharacterConfig Enemy::MakeConfig() const
{
	CharacterConfig config;
	config.move = MakeMoveSettings();
	config.model = MakeModelDesc();
	config.hitRadius = HIT_RADIUS;
	config.hitCenterHeight = HIT_CENTER_HEIGHT;
	config.maxHp = 100.0f;
	return config;
}

void Enemy::OnStart()
{
	EnemyStateContext stateContext;
	stateContext.mover = &m_mover;
	stateContext.model = &m_model;
	stateContext.statusEffects = &m_statusEffects;
	m_stateMachine.Init(stateContext);  // Idle から始める（Idle のアニメーションを流す）。
	// 黒騎士は魔法を持たない（MagicCaster に何も足さないので、fire を押しても何も撃たない）。
}

void Enemy::UpdateAction(float deltaTime)
{
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	m_stateMachine.Update(m_input, deltaTime);
}
