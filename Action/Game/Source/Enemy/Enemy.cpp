#include "stdafx.h"
#include "Enemy.h"

namespace
{
	// 黒騎士の大きさ。兜のあご〜頭頂（モデルで 0.175）が、Player の顔のあご〜頭頂（0.217 × 1.128）と同じになる 1.128 × 0.217 ÷ 0.175。
	constexpr float MODEL_SCALE = 1.4f;
	// 当たり判定とカプセルは、前の大きさ（1.3）で体に合わせた値を、同じ割合で大きくする。
	constexpr float SIZE_RATIO = MODEL_SCALE / 1.3f;
	constexpr float HIT_RADIUS = 60.0f * SIZE_RATIO;         // 弾に当たる球の半径。黒騎士の胴（肩当てまで）を覆う。
	constexpr float HIT_CENTER_HEIGHT = 110.0f * SIZE_RATIO; // 足元から当たり判定の中心までの高さ（黒騎士の胸の下あたり）。
	constexpr float WALK_SPEED = 140.0f;        // 余裕たっぷりにゆっくり歩く速さ。
	constexpr float WALK_ANIM_FOOT_SPEED = 229.0f;  // Walk を等速で再生したときに、足が地面を送る速さ（Blender で測った）。
	// 黒騎士の動き全体の速さ。アニメーションが崩れていないかを確かめるため、今は半分にしている。確かめ終わったら 1 に戻す。
	constexpr float PLAY_SPEED = 0.5f;

	CharacterMoveSettings MakeMoveSettings()
	{
		CharacterMoveSettings settings;
		settings.walkSpeed = WALK_SPEED;      // Walk は WALK_SPEED / WALK_ANIM_FOOT_SPEED 倍でゆっくり再生する（EnemyMoveState）。
		settings.runSpeedMultiplier = 2.75f;  // 走るときは 385。Run の足が地面を送る速さ（約384）に合わせる。
		settings.gravity = 980.0f;
		settings.capsuleRadius = 30.0f * SIZE_RATIO;   // 黒騎士は肩当てが広いので、プレイヤー（20）より太くする。
		settings.capsuleHeight = 140.0f * SIZE_RATIO;
		return settings;
	}

	BlackKnightInputSource::Settings MakeAiSettings()
	{
		BlackKnightInputSource::Settings settings;
		settings.searchRange = 1800.0f;    // この距離以内に入ると追いかけ始める（ここまでが「遠い」）。
		settings.nearRange = 220.0f;       // この距離以内が「近い」。剣を振る（剣の長さ＋踏み込み）。
		settings.midRange = 1200.0f;       // この距離以内が「中くらい」。
		settings.stopDistance = 150.0f;    // この距離まで近づいたら止まる（剣が届くところで止まる）。
		settings.decideInterval = 2.0f;    // 歩いている間、走るかを決め直す間隔（秒）。
		settings.runChance = 0.5f;         // 中くらい・遠いときに走る確率。残りは攻撃（中くらいなら攻撃1、遠いなら攻撃3）。
		settings.attackInterval = 3.5f;    // 近いときに攻撃2をする間隔（秒）。攻撃2は 2.4秒かかる。
		settings.firstAttackDelay = 0.6f;  // 相手を見つけてから最初に振れるまでの秒数。
		return settings;
	}

	const std::string ANIM_DIR = "Assets/modelData/enemy/BlackKnight/Animation/";
	// クリップの名前（EnEnemyAnimClip の順）。retarget_black_knight.py が書き出す FBX の名前と同じ。
	constexpr const char* CLIP_NAMES[enEnemyAnimClip_Num] = { "Idle", "Walk", "Run", "Attack1", "Attack2", "Attack3" };

	CharacterModelDesc MakeModelDesc()
	{
		CharacterModelDesc desc;
		// 黒騎士。体は Meshy で作り、骨格を SK_Player に合わせる（tools/BlenderScripts/make_black_knight_meshy.py で作る）。
		// アニメーションは既存のモーションを黒騎士の骨格へ移したもの（tools/BlenderScripts/retarget_black_knight.py で作る）。
		desc.modelPath = "Assets/modelData/enemy/BlackKnight/BlackKnight.fbx";
		desc.upAxis = enModelUpAxisZ;
		desc.scale = MODEL_SCALE;
		// 0〜Num-1 がふだん流すクリップ（開発用の画面で打ったキーを焼いたもの）。
		// Num〜 は、モーションを直すときに流す、キーを焼く前のクリップ（Animation/Base/。なければふだんのもの）。
		desc.clips.resize(enEnemyAnimClip_Num * 2);
		for (int i = 0; i < enEnemyAnimClip_Num; i++) {
			const std::string name = CLIP_NAMES[i];
			const bool isLoop = i == enEnemyAnimClip_Idle || i == enEnemyAnimClip_Walk || i == enEnemyAnimClip_Run;
			const std::string path = ANIM_DIR + name + ".fbx";
			const std::string basePath = ANIM_DIR + "Base/" + name + ".fbx";
			desc.clips[i] = { path, isLoop };
			const bool hasBase = GetFileAttributesA(basePath.c_str()) != INVALID_FILE_ATTRIBUTES;
			desc.clips[enEnemyAnimClip_Num + i] = { hasBase ? basePath : path, isLoop };
		}
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
	stateContext.owner = this;
	stateContext.faction = GetFaction();
	stateContext.settings.walkAnimSpeed = WALK_SPEED / WALK_ANIM_FOOT_SPEED;
	stateContext.settings.playSpeed = PLAY_SPEED;
	m_stateContext = stateContext;
	m_stateMachine.Init(stateContext);  // Idle から始める（Idle のアニメーションを流す）。

	// モーションを直す開発用の画面（F1）。
	std::vector<EnemyMotionEditor::ClipEntry> clips;
	for (int i = 0; i < enEnemyAnimClip_Num; i++) {
		clips.push_back({ CLIP_NAMES[i], i, enEnemyAnimClip_Num + i });
	}
	m_motionEditor.Init(&m_model, clips);
	// 黒騎士は魔法を持たない（MagicCaster に何も足さないので、fire を押しても魔法は撃たない）。
	// fire は剣の攻撃（EnemyAttackState）になり、攻撃3の斬撃はステートが飛ばす。
}

void Enemy::UpdateAction(float deltaTime)
{
	// モーションを直している間は、AI を止めてその場に立たせ、選んだクリップを流す。
	if (m_motionEditor.IsEditing()) {
		m_mover.Move(Vector3::Zero, deltaTime);
		m_motionEditor.UpdatePlayback(deltaTime);
		return;
	}
	if (m_motionEditor.ConsumeStopped()) {
		m_stateMachine.Init(m_stateContext);  // 直し終わったら、Idle からふだんの動きに戻す
	}
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	// PLAY_SPEED を掛けて、移動・攻撃の時間（突進・斬撃を出すとき）もアニメーションと同じだけ遅くする。
	m_stateMachine.Update(m_input, deltaTime * PLAY_SPEED);
}

void Enemy::OnLateUpdate(float deltaTime)
{
	// F1 で開発用の画面を出しているときだけ（Release では ImGui を起動しないので、見ないで呼ぶと落ちる）。
	if (DebugUI::IsVisible()) {
		m_motionEditor.DrawUI();
	}}

void Enemy::OnPostAnimationPose(Skeleton& skeleton)
{
	m_motionEditor.ApplyToSkeleton(skeleton, GetModelRotation(), m_model.GetScale());
}
