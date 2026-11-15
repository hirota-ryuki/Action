#include "stdafx.h"
#include "AttackBase.h"
#include "CharacterModel.h"
#include "CharacterMover.h"

namespace
{
	constexpr float ANIM_FPS = 30.0f;
}

bool AttackBase::OnStart()
{
	// 体を渡していないキャラクター（攻撃を登録していても SetBody していない）は、何もせずに消える。
	const SkillBody& body = GetBody();
	return body.model != nullptr && body.mover != nullptr;
}

void AttackBase::OnActivate()
{
	const SkillBody& body = GetBody();
	// 振り始めに相手の方（撃つ向き）を向く。あとは向きを変えない。
	body.mover->FaceTo(GetDirection());
	body.model->PlayAnimation(GetData().clip, body.blendSeconds);
	body.model->GetModelRender().SetAnimationSpeed(body.timeScale);  // 歩きでゆっくりにした速さを戻す。
	m_time = 0.0f;
	m_isCharging = false;
	m_isChargeDone = false;
	m_chargeLeft = 0.0f;
}

void AttackBase::OnSkillUpdate()
{
	const SkillBody& body = GetBody();
	const AttackData& data = GetData();
	// アニメーションを流し終えたら、攻撃も終わる。
	if (!body.model->IsPlayingAnimation()) {
		End();
		return;
	}
	const float deltaTime = g_gameTime->GetFrameDeltaTime() * body.timeScale;

	// 溜め: 構えのコマに来たら、アニメーションを止めて、決めた秒数だけ待つ。その間は時間を進めない。
	if (data.chargeAt >= 0.0f && !m_isChargeDone && m_time >= data.chargeAt) {
		if (!m_isCharging) {
			m_isCharging = true;
			m_chargeLeft = data.chargeSeconds;
			body.model->GetModelRender().SetAnimationSpeed(0.0f);
		}
		m_chargeLeft -= deltaTime;
		if (m_chargeLeft > 0.0f) {
			body.mover->Move(Vector3::Zero, deltaTime);
			return;
		}
		m_isCharging = false;
		m_isChargeDone = true;
		body.model->GetModelRender().SetAnimationSpeed(body.timeScale);
	}

	const float prevTime = m_time;
	m_time += deltaTime;

	// アニメーションの中で腰が進んだ分（コマごとの表）だけ、体ごと前へ動かす。それ以外は止まる（重力はかかる）。
	Vector3 velocity = Vector3::Zero;
	if (data.root != nullptr && deltaTime > 0.0f) {
		const float moved = (SampleRoot(m_time) - SampleRoot(prevTime)) * body.rootMoveScale;
		velocity = GetBodyForward() * (moved / deltaTime);
	}
	body.mover->Move(velocity, deltaTime);

	OnSwing(m_time);
}

const Vector3& AttackBase::GetBodyPosition() const
{
	return GetBody().mover->GetPosition();
}

Vector3 AttackBase::GetBodyForward() const
{
	Vector3 forward = GetBody().mover->GetForward();
	forward.y = 0.0f;
	forward.Normalize();
	return forward;
}

float AttackBase::SampleRoot(float time) const
{
	const AttackData& data = GetData();
	if (data.root == nullptr || data.rootNum == 0) {
		return 0.0f;
	}
	const float f = max(0.0f, time * ANIM_FPS);
	const int i = static_cast<int>(f);
	if (i >= data.rootNum - 1) {
		return data.root[data.rootNum - 1];
	}
	const float t = f - static_cast<float>(i);
	return data.root[i] + (data.root[i + 1] - data.root[i]) * t;
}
