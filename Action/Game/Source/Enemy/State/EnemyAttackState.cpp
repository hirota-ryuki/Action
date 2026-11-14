#include "stdafx.h"
#include "EnemyAttackState.h"
#include "EnemyGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "CharacterModel.h"
#include "SlashMagic.h"

namespace
{
	constexpr int ATTACK_NUM = 3;
	constexpr int MAX_SLASH = 5;

	// 攻撃ごとの動き。時間はアニメーションの始まりからの秒数（30コマで1秒）。
	// コマは tools/BlenderScripts/retarget_black_knight.py が書き出すときに出す「出来上がりの何コマ目から」を元にした。
	constexpr float ANIM_FPS = 30.0f;

	// 攻撃1（KinemIQ の dash slash）で、腰が前へ進んだ距離（黒騎士のモデルの m）。30コマで1秒、1コマ目が 0。
	// retarget_black_knight.py が "in_place" で腰の水平の動きを消すときに出す「ROOT [Attack1] … コマごと」の値。
	// 7〜12コマ目で一気に 3m 踏み込み、そのあと少しずつ止まる。ゲームはこの分だけ黒騎士ごと前へ動かす。
	constexpr float ATTACK1_ROOT[] = {
		0.00f, 0.01f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.85f, 1.66f, 2.33f,
		2.80f, 3.05f, 3.19f, 3.30f, 3.40f, 3.47f, 3.53f, 3.58f, 3.61f, 3.63f,
		3.65f, 3.65f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f,
		3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.66f, 3.68f, 3.70f, 3.71f, 3.72f,
		3.72f, 3.72f, 3.72f, 3.72f, 3.72f, 3.72f, 3.72f, 3.72f,
	};
	constexpr int ATTACK1_ROOT_NUM = sizeof(ATTACK1_ROOT) / sizeof(ATTACK1_ROOT[0]);

	struct AttackTiming
	{
		int clip;
		float dashStart;            // この間、前へ dashSpeed で進む。
		float dashEnd;
		float dashSpeed;
		int slashNum;               // 飛ばす斬撃の数。
		float slashTimes[MAX_SLASH];
		float slashRollDeg[MAX_SLASH];  // 斬撃を進む向きのまわりに回す角度。黒騎士から見て、- で左上→右下、+ で右上→左下。
		float chargeAt;             // この時間のコマでアニメーションを止めて溜める（tuning の秒数だけ）。負なら溜めない。
		const float* root;          // コマごとに前へ進んだ距離（モデルの m）。nullptr なら使わない。
		int rootNum;
	};

	constexpr AttackTiming ATTACKS[ATTACK_NUM] = {
		// 攻撃1: 左腰に剣を構えてかがみ（6コマ目）、そこで溜めてから、一気に踏み込んで横に斬り抜ける。
		{ enEnemyAnimClip_Attack1, 0.0f, 0.0f, 0.0f, 0, {}, {}, 5.5f / ANIM_FPS, ATTACK1_ROOT, ATTACK1_ROOT_NUM },
		// 攻撃2: その場で、片手剣の重い連続斬り（KinemIQ の Sword - heavy combo）を振る。
		{ enEnemyAnimClip_Attack2, 0.0f, 0.0f, 0.0f, 0, {}, {}, -1.0f, nullptr, 0 },
		// 攻撃3: 連続斬り（KinemIQ の slash combo）。剣が体の前を通る5回の振りで、斬撃を出す。
		// コマは retarget_black_knight.py --sword の剣先の通り道から読んだ（27〜33コマ目の振りは体の横〜後ろを通るので出さない）。
		//   12 右上→左下、49.5 右下→左上（斬り上げ）、62 右上→左下、78 右→左（横に払う）、90 上→下（振り下ろし）
		{ enEnemyAnimClip_Attack3, 0.0f, 0.0f, 0.0f, 5,
			{ 12.0f / 30.0f, 49.5f / 30.0f, 62.0f / 30.0f, 78.0f / 30.0f, 90.0f / 30.0f },
			{ 45.0f, -45.0f, 45.0f, 0.0f, 90.0f }, -1.0f, nullptr, 0 },
	};

	// コマごとの距離の表を、time 秒のところで読む（間はまっすぐつなぐ）。
	float SampleRoot(const AttackTiming& timing, float time)
	{
		if (timing.root == nullptr || timing.rootNum == 0) {
			return 0.0f;
		}
		const float f = max(0.0f, time * ANIM_FPS);
		const int i = static_cast<int>(f);
		if (i >= timing.rootNum - 1) {
			return timing.root[timing.rootNum - 1];
		}
		const float t = f - static_cast<float>(i);
		return timing.root[i] + (timing.root[i + 1] - timing.root[i]) * t;
	}

	constexpr float SLASH_HEIGHT = 110.0f;   // 足元から、斬撃を出す高さ（黒騎士の胸のあたり）。
	constexpr float SLASH_FORWARD = 80.0f;   // 体の前へ出して、自分のカプセルに当たらないようにする。

	const AttackTiming& GetTiming(int attackIndex)
	{
		if (attackIndex < 0 || attackIndex >= ATTACK_NUM) {
			attackIndex = 0;
		}
		return ATTACKS[attackIndex];
	}
}

void EnemyAttackState::Enter(EnemyStateContext& context, const CharacterInput& input)
{
	m_attackIndex = input.attackIndex;
	m_time = 0.0f;
	m_nextSlash = 0;
	m_isCharging = false;
	m_isChargeDone = false;
	m_chargeLeft = 0.0f;
	// 振り始めに相手の方を向く。あとは向きを変えない。
	context.mover->FaceTo(input.lookDirection);
	context.model->PlayAnimation(GetTiming(m_attackIndex).clip, context.settings.animBlendSeconds);
	context.model->GetModelRender().SetAnimationSpeed(context.settings.playSpeed);  // Walk でゆっくりにした速さを戻す。
}

EnemyStateId EnemyAttackState::CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const
{
	// 攻撃中は当たってもひるまない。死んだときだけ倒れる。
	if (EnemyGroundState::CheckDamage(context, false) == EnemyStateId::Dead) {
		return EnemyStateId::Dead;
	}
	if (context.model->IsPlayingAnimation()) {
		return EnemyStateId::Attack;
	}
	return EnemyGroundState::SelectByInput(context, input);
}

void EnemyAttackState::Update(EnemyStateContext& context, const CharacterInput& input, float deltaTime)
{
	const AttackTiming& timing = GetTiming(m_attackIndex);

	// 溜め: 構えのコマに来たら、アニメーションを止めて、決めた秒数だけ待つ。その間は時間を進めない。
	if (timing.chargeAt >= 0.0f && !m_isChargeDone && m_time >= timing.chargeAt) {
		if (!m_isCharging) {
			m_isCharging = true;
			m_chargeLeft = context.tuning != nullptr ? context.tuning->attack1ChargeSeconds : 0.0f;
			context.model->GetModelRender().SetAnimationSpeed(0.0f);
		}
		m_chargeLeft -= deltaTime;
		if (m_chargeLeft > 0.0f) {
			context.mover->Move(Vector3::Zero, deltaTime);
			return;
		}
		m_isCharging = false;
		m_isChargeDone = true;
		context.model->GetModelRender().SetAnimationSpeed(context.settings.playSpeed);
	}

	const float prevTime = m_time;
	m_time += deltaTime;

	// 突進のあいだは前へ進む。それ以外は止まる（重力はかかる）。
	Vector3 forward = context.mover->GetForward();
	forward.y = 0.0f;
	forward.Normalize();
	Vector3 velocity = Vector3::Zero;
	if (m_time >= timing.dashStart && m_time < timing.dashEnd) {
		velocity = forward * timing.dashSpeed;
	}
	// アニメーションの中で腰が進んだ分（コマごとの表）だけ、黒騎士ごと前へ動かす。
	if (timing.root != nullptr && deltaTime > 0.0f) {
		const float moved = (SampleRoot(timing, m_time) - SampleRoot(timing, prevTime)) * context.settings.rootMoveScale;
		velocity += forward * (moved / deltaTime);
	}
	context.mover->Move(velocity, deltaTime);

	// 斬撃を飛ばすコマを過ぎたら、1つずつ出す。
	while (m_nextSlash < timing.slashNum && m_time >= timing.slashTimes[m_nextSlash]) {
		FireSlash(context, input, timing.slashRollDeg[m_nextSlash]);
		m_nextSlash++;
	}
}

void EnemyAttackState::FireSlash(const EnemyStateContext& context, const CharacterInput& input, float rollDeg) const
{
	Vector3 forward = context.mover->GetForward();
	forward.y = 0.0f;
	forward.Normalize();
	Vector3 origin = context.mover->GetPosition() + forward * SLASH_FORWARD;
	origin.y += SLASH_HEIGHT;
	// 相手の胸のあたりへ飛ばす。狙いがなければ正面へ。
	Vector3 direction = forward;
	if (input.hasAim) {
		direction = input.aimDirection;
	}

	SlashMagic* slash = NewGO<SlashMagic>(1, MagicBase::OBJECT_NAME);
	slash->SetRoll(rollDeg);
	slash->Init(origin, direction, context.owner, context.faction);
}
