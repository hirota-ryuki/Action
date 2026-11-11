#include "stdafx.h"
#include "EnemyAttackState.h"
#include "EnemyGroundState.h"
#include "CharacterInput.h"
#include "CharacterMover.h"
#include "CharacterModel.h"
#include "SlashBullet.h"

namespace
{
	constexpr int ATTACK_NUM = 3;
	constexpr int MAX_SLASH = 5;

	// 攻撃ごとの動き。時間はアニメーションの始まりからの秒数（30コマで1秒）。
	// コマは tools/BlenderScripts/retarget_black_knight.py が書き出すときに出す「出来上がりの何コマ目から」を元にした。
	struct AttackTiming
	{
		int clip;
		float dashStart;            // この間、前へ dashSpeed で進む。
		float dashEnd;
		float dashSpeed;
		int slashNum;               // 飛ばす斬撃の数。
		float slashTimes[MAX_SLASH];
		float slashRollDeg[MAX_SLASH];  // 斬撃を進む向きのまわりに回す角度。黒騎士から見て、- で左上→右下、+ で右上→左下。
	};

	constexpr AttackTiming ATTACKS[ATTACK_NUM] = {
		// 攻撃1: 槍の突きの踏み込み（5〜12コマ目）で前へ出る。回転斬りは 23〜33コマ目。
		{ enEnemyAnimClip_Attack1, 5.0f / 30.0f, 12.0f / 30.0f, 900.0f, 0, {}, {} },
		// 攻撃2: その場で3回振る（17・32・45コマ目あたり）。
		{ enEnemyAnimClip_Attack2, 0.0f, 0.0f, 0.0f, 0, {}, {} },
		// 攻撃3: 左上→右下から始めて、左右交互に5回。剣が前を通るコマで斬撃を出す。
		{ enEnemyAnimClip_Attack3, 0.0f, 0.0f, 0.0f, 5,
			{ 17.0f / 30.0f, 28.0f / 30.0f, 38.0f / 30.0f, 50.0f / 30.0f, 61.0f / 30.0f },
			{ -45.0f, 45.0f, -45.0f, 45.0f, -45.0f } },
	};

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
	m_time += deltaTime;

	// 突進のあいだは前へ進む。それ以外は止まる（重力はかかる）。
	Vector3 velocity = Vector3::Zero;
	if (m_time >= timing.dashStart && m_time < timing.dashEnd) {
		velocity = context.mover->GetForward();
		velocity.y = 0.0f;
		velocity.Normalize();
		velocity *= timing.dashSpeed;
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

	SlashBullet* slash = NewGO<SlashBullet>(1, BulletBase::OBJECT_NAME);
	slash->SetRoll(rollDeg);
	slash->Init(origin, direction, context.owner, context.faction);
}
