#include "stdafx.h"
#include "Enemy.h"
#include <imgui/imgui.h>

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
	// 黒騎士の動き全体の速さ。アニメーションが崩れていないかを確かめるときは下げる（例: 0.5）。
	constexpr float PLAY_SPEED = 1.0f;
	constexpr float MAX_HP = 100.0f;
	// 1回当たったときに減る HP（仮）。4回で倒れる。弾ごとの威力ができたら、そちらを使う。
	constexpr float HIT_DAMAGE = 25.0f;
	// 倒れ終わってから消えるまでの秒数。
	constexpr float VANISH_DELAY = 2.0f;

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
		settings.attackInterval = 4.8f;    // 近いときに攻撃2をする間隔（秒）。攻撃2は 3.7秒かかる（PLAY_SPEED が 1 のとき）。
		settings.firstAttackDelay = 0.6f;  // 相手を見つけてから最初に振れるまでの秒数。
		return settings;
	}

	CharacterModelDesc MakeModelDesc()
	{
		CharacterModelDesc desc;
		// 黒騎士。体は Meshy で作り、骨格を SK_Player に合わせる（tools/BlenderScripts/make_black_knight_meshy.py で作る）。
		// アニメーションは既存のモーションを黒騎士の骨格へ移したもの（tools/BlenderScripts/retarget_black_knight.py で作る）。
		desc.modelPath = "Assets/modelData/enemy/BlackKnight/BlackKnight.fbx";
		desc.upAxis = enModelUpAxisZ;
		desc.scale = MODEL_SCALE;
		desc.clips.resize(enEnemyAnimClip_Num);
		desc.clips[enEnemyAnimClip_Idle] = { "Assets/modelData/enemy/BlackKnight/Animation/Idle.fbx", true };
		desc.clips[enEnemyAnimClip_Walk] = { "Assets/modelData/enemy/BlackKnight/Animation/Walk.fbx", true };
		desc.clips[enEnemyAnimClip_Run] = { "Assets/modelData/enemy/BlackKnight/Animation/Run.fbx", true };
		desc.clips[enEnemyAnimClip_Attack1] = { "Assets/modelData/enemy/BlackKnight/Animation/Attack1.fbx", false };
		desc.clips[enEnemyAnimClip_Attack2] = { "Assets/modelData/enemy/BlackKnight/Animation/Attack2.fbx", false };
		desc.clips[enEnemyAnimClip_Attack3] = { "Assets/modelData/enemy/BlackKnight/Animation/Attack3.fbx", false };
		desc.clips[enEnemyAnimClip_Hit] = { "Assets/modelData/enemy/BlackKnight/Animation/Hit.fbx", false };
		desc.clips[enEnemyAnimClip_Dead] = { "Assets/modelData/enemy/BlackKnight/Animation/Dead.fbx", false };
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
	config.maxHp = MAX_HP;
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
	stateContext.settings.vanishDelay = VANISH_DELAY;
	stateContext.settings.rootMoveScale = 100.0f * MODEL_SCALE;
	m_damage.status = &m_status;
	stateContext.damage = &m_damage;
	stateContext.tuning = &m_tuning;
	// 弾などが当たったら HP を減らし、ステートに知らせる（ひるむか・倒れるかはステートが決める）。
	m_hitTarget.SetOnHit([this](const HitInfo&) {
		if (m_status.IsHpEmpty()) {
			return;
		}
		m_status.Damage(HIT_DAMAGE);
		m_damage.isHitRequested = true;
	});
	m_stateMachine.Init(stateContext);  // Idle から始める（Idle のアニメーションを流す）。
	// 黒騎士は魔法を持たない（MagicCaster に何も足さないので、fire を押しても魔法は撃たない）。
	// fire は剣の攻撃（EnemyAttackState）になり、攻撃3の斬撃はステートが飛ばす。
}

void Enemy::UpdateAction(float deltaTime)
{
	// 倒れ終わって消えたら、体を隠して当たり判定を外し、何もしない。
	if (m_damage.isVanished) {
		SetRenderBody(false);
		m_hitTarget.SetEnabled(false);
		return;
	}
	// モーションの確認（F1 の開発用の画面）。再生している間は AI とステートを止め、その場で立たせる。
	m_animCatalog.UpdateUI();
	if (m_animCatalog.IsActive()) {
		m_mover.Update(CharacterInput(), deltaTime);
		m_animCatalog.Update(m_model, deltaTime);
		return;
	}
	if (m_animCatalog.ConsumeStopped()) {
		m_stateMachine.Reset();  // 止めたら Idle から戻す（アニメーションと速さもステートが設定し直す）。
	}
	// 移動とアニメーションはステートが行う。凍結中は Frozen ステートになり、重力で落ちるだけになる。
	// PLAY_SPEED を掛けて、移動・攻撃の時間（突進・斬撃を出すとき）もアニメーションと同じだけ遅くする。
	CharacterInput input = m_input;
	UpdateTuningUI(input);
	m_stateMachine.Update(input, deltaTime * PLAY_SPEED);
}

void Enemy::UpdateTuningUI(CharacterInput& input)
{
	// 「攻撃1を繰り返す」は、F1 の画面を閉じても続ける（見ながら溜めの長さを決めるため）。
	const bool canAttack = m_stateMachine.GetCurrentId() != EnemyStateId::Attack
		&& m_stateMachine.GetCurrentId() != EnemyStateId::Dead;
	int fireIndex = (m_isRepeatAttack1 && canAttack) ? 0 : -1;  // 出させる攻撃（0〜2）。-1 なら出させない。

	// F1 で開発用の画面を出しているときだけ（Release では ImGui を起動しないので、見ないで呼ぶと落ちる）。
	if (DebugUI::IsVisible()) {
		ImGui::Begin(u8"黒騎士の攻撃");
		ImGui::SliderFloat(u8"攻撃1の溜め", &m_tuning.attack1ChargeSeconds, 0.0f, 3.0f, u8"%.2f 秒");
		ImGui::TextUnformatted(u8"左腰に剣を構えてかがんだところで止める秒数。");
		ImGui::TextUnformatted(u8"黒騎士は今、半分の速さ（PLAY_SPEED）なので、見た目は2倍の長さになる。");
		if (ImGui::Button(u8"攻撃1を出す") && canAttack) {
			fireIndex = 0;
		}
		ImGui::SameLine();
		ImGui::Checkbox(u8"攻撃1を繰り返す", &m_isRepeatAttack1);
		if (ImGui::Button(u8"攻撃2を出す") && canAttack) {
			fireIndex = 1;
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"攻撃3を出す") && canAttack) {
			fireIndex = 2;
		}
		ImGui::Text(u8"今のステート: %s", m_stateMachine.GetCurrentName());
		ImGui::End();
	}
	if (fireIndex >= 0) {
		input.fire.isTrigger = true;
		input.attackIndex = fireIndex;
	}
}
