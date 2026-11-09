#include "stdafx.h"
#include "Enemy.h"
#include "IceMagic.h"

namespace
{
	constexpr float HIT_RADIUS = 60.0f;         // 弾に当たる球の半径。黒騎士の胴（肩当てまで）を覆う。
	constexpr float HIT_CENTER_HEIGHT = 110.0f; // 足元から当たり判定の中心までの高さ（黒騎士の胸の下あたり）。

	CharacterMoveSettings MakeMoveSettings()
	{
		CharacterMoveSettings settings;
		settings.walkSpeed = 140.0f;
		settings.gravity = 980.0f;
		settings.capsuleRadius = 30.0f;   // 黒騎士は肩当てが広いので、プレイヤー（20）より太くする。
		settings.capsuleHeight = 140.0f;
		return settings;
	}

	NpcInputSource::Settings MakeAiSettings()
	{
		NpcInputSource::Settings settings;
		// 今は敵を止めておく（相手を探さないので、動かず・向きも変えず・撃たない）。動かすときは 900 に戻す。
		settings.searchRange = 0.0f;     // この距離以内に入ると追跡を開始する。
		settings.stopDistance = 90.0f;   // この距離まで近づいたら停止する（近づきすぎ防止）。
		settings.canFire = true;
		settings.fireRange = 700.0f;
		settings.fireInterval = 4.0f;    // 凍結(2.5秒)より長くして、凍らされ続けないようにする。
		settings.firstFireDelay = 1.5f;
		settings.castHeight = 150.0f;
		return settings;
	}

	CharacterModelDesc MakeModelDesc()
	{
		CharacterModelDesc desc;
		// 黒騎士。SK_Player と同じ骨格なので、プレイヤーの Idle・Run をそのまま流せる
		// （tools/BlenderScripts/make_black_knight.py で作る）。
		// アニメーションの FBX は黒騎士のフォルダに写したものを使う。tka は FBX の隣に1つだけ作られ、
		// ボーンの並びはモデルごとに違うので、プレイヤーと同じ FBX を指すとプレイヤーの tka を上書きしてしまう。
		desc.modelPath = "Assets/modelData/enemy/BlackKnight/BlackKnight.fbx";
		desc.upAxis = enModelUpAxisZ;
		desc.scale = 1.3f;   // プレイヤー（1.128）より一回り大きくする。
		desc.clips.resize(enEnemyAnimClip_Num);
		desc.clips[enEnemyAnimClip_Idle] = { "Assets/modelData/enemy/BlackKnight/Animation/Idle.fbx", true };
		desc.clips[enEnemyAnimClip_Run] = { "Assets/modelData/enemy/BlackKnight/Animation/Run.fbx", true };
		return desc;
	}

	MagicCaster::Settings MakeCasterSettings()
	{
		MagicCaster::Settings settings;
		settings.castHeight = 150.0f;  // 黒騎士の胸の高さ。
		settings.castForward = 80.0f;  // 自分のカプセル(半径30)と肩当てに当たらないよう前に出して撃つ。
		return settings;
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
	config.caster = MakeCasterSettings();
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

	m_magicCaster.AddMagic(std::make_unique<IceMagic>());
}

void Enemy::UpdateAction(float deltaTime)
{
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	m_stateMachine.Update(m_input, deltaTime);
}
