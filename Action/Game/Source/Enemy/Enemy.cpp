#include "stdafx.h"
#include "Enemy.h"

namespace
{
	constexpr float HIT_RADIUS = 60.0f;         // 弾に当たる球の半径。黒騎士の胴（肩当てまで）を覆う。
	constexpr float HIT_CENTER_HEIGHT = 110.0f; // 足元から当たり判定の中心までの高さ（黒騎士の胸の下あたり）。

	CharacterMoveSettings MakeMoveSettings()
	{
		CharacterMoveSettings settings;
		settings.walkSpeed = 120.0f;   // 黒騎士は走らず、Walk の歩幅に合わせてゆっくり歩く。
		settings.gravity = 980.0f;
		settings.capsuleRadius = 30.0f;   // 黒騎士は肩当てが広いので、プレイヤー（20）より太くする。
		settings.capsuleHeight = 140.0f;
		return settings;
	}

	NpcInputSource::Settings MakeAiSettings()
	{
		NpcInputSource::Settings settings;
		// 今は敵を止めておく（相手を探さないので、動かず・向きも変えず・撃たない）。動かすときは 900 に戻す。
		settings.searchRange = 900.0f;     // この距離以内に入ると追跡を開始する。
		settings.stopDistance = 150.0f;   // この距離まで近づいたら停止する（剣が届くところで止まる）。
		// 黒騎士は魔法を持たず、fire（撃つ）の入力で剣を振る（EnemyAttackState）。
		settings.canFire = true;
		settings.fireRange = 220.0f;      // この距離以内なら剣を振る（剣の長さ＋踏み込み）。
		settings.fireInterval = 2.5f;     // 剣を振る間隔（秒）。振るのに 1.1秒かかる。
		settings.firstFireDelay = 0.6f;   // 近づいてから最初に振るまでの秒数。
		return settings;
	}

	CharacterModelDesc MakeModelDesc()
	{
		CharacterModelDesc desc;
		// 黒騎士。骨格は SK_Player と同じ（tools/BlenderScripts/make_black_knight.py で作る）。
		// アニメーションは黒騎士用に作ったもの（tools/BlenderScripts/make_black_knight_anims.py で作る）。
		desc.modelPath = "Assets/modelData/enemy/BlackKnight/BlackKnight.fbx";
		desc.upAxis = enModelUpAxisZ;
		desc.scale = 1.3f;   // プレイヤー（1.128）より一回り大きくする。
		desc.clips.resize(enEnemyAnimClip_Num);
		desc.clips[enEnemyAnimClip_Idle] = { "Assets/modelData/enemy/BlackKnight/Animation/Idle.fbx", true };
		desc.clips[enEnemyAnimClip_Walk] = { "Assets/modelData/enemy/BlackKnight/Animation/Walk.fbx", true };
		desc.clips[enEnemyAnimClip_Attack] = { "Assets/modelData/enemy/BlackKnight/Animation/Attack.fbx", false };
		return desc;
	}

}

Enemy::Enemy()
	: Character(Faction::Enemy)
	, m_npcInputSource(MakeAiSettings())
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
