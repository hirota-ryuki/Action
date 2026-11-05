#include "stdafx.h"
#include "BurningGround.h"
#include "EffectId.h"
#include "FireBullet.h"
#include "HitTarget.h"
#include "StatusEffects.h"

namespace
{
	constexpr int kGroundEffectId = static_cast<int>(EffectId::FireGround);
	// fire_ground.efk は半径10の円に炎を出す。Scale 10 倍でワールドの半径100になる。
	constexpr float kEffectRadiusAtScaleOne = 10.0f;

	constexpr float kBurnSeconds = 1.5f;     // 範囲に入った相手を燃やす時間。範囲から出ても、この時間は燃え続ける。
	constexpr float kBurnHeight = 150.0f;    // 床からこの高さまでに足元がある相手を燃やす（ジャンプで飛び越えられる）。
	constexpr float kBurnDepth = 50.0f;      // 床よりこれだけ下に足元がある相手も燃やす（坂の下側など）。
	constexpr float kTargetRadiusRate = 0.5f;  // 相手の当たり判定の半径のうち、範囲の判定に足す割合。
	constexpr float kSpreadSeconds = 0.4f;     // 中心から最大の広さまで燃え広がる秒数。着弾の飛び散りに合わせる。
	constexpr float kSpreadStartRate = 0.1f;   // 燃え始めの広さ（最大に対する割合）。
}

BurningGround::~BurningGround()
{
	// Effect のデストラクタは再生を止めないため、ここで明示的に止める。
	if (!m_isValid || EffectEngine::GetInstance() == nullptr) {
		return;
	}
	m_effect.Stop();
}

void BurningGround::Init(const Vector3& position, float radius, float seconds, const IGameObject* owner, Faction faction)
{
	m_position = position;
	m_radius = radius;
	m_seconds = seconds;
	m_owner = owner;
	m_faction = faction;
}

bool BurningGround::Start()
{
	m_isValid = FireBullet::RegisterEffects();
	if (!m_isValid) {
		return true;
	}

	m_effect.Init(kGroundEffectId);
	m_effect.Play();
	m_effect.SetPosition(m_position);
	UpdateEffectScale();
	return true;
}

void BurningGround::Update()
{
	if (!m_isValid) {
		DeleteGO(this);
		return;
	}

	switch (m_state) {
	case EnState::Burning:
		if (!m_isSpreadFinished) {
			// 燃え広がっている間は、出ている炎ごと外へ広げる。最大の広さに達したフレームまで更新する。
			UpdateEffectScale();
			m_isSpreadFinished = m_elapsed >= kSpreadSeconds;
		}
		BurnTargetsInRange();
		m_elapsed += g_gameTime->GetFrameDeltaTime();
		if (m_elapsed >= m_seconds) {
			// 炎の発生だけを止め、出ている炎は寿命まで燃やして自然に消す。
			m_effect.StopRoot();
			m_state = EnState::Vanishing;
		}
		break;
	case EnState::Vanishing:
		if (!m_effect.IsPlay()) {
			DeleteGO(this);
		}
		break;
	}
}

void BurningGround::BurnTargetsInRange()
{
	for (HitTarget* target : HitTargetRegistry::GetTargets()) {
		if (!target->IsActive() || target->GetOwner() == m_owner) {
			continue;
		}
		if (!IsHostile(m_faction, target->GetFaction())) {
			continue;
		}
		StatusEffects* statusEffects = target->GetStatusEffects();
		if (statusEffects == nullptr) {
			continue;
		}

		const Vector3& foot = target->GetFootPosition();
		const float height = foot.y - m_position.y;
		if (height < -kBurnDepth || height > kBurnHeight) {
			continue;
		}
		const float dx = foot.x - m_position.x;
		const float dz = foot.z - m_position.z;
		const float range = GetCurrentRadius() + target->GetRadius() * kTargetRadiusRate;
		if (dx * dx + dz * dz > range * range) {
			continue;
		}
		statusEffects->Burn(kBurnSeconds);
	}
}

float BurningGround::GetCurrentRadius() const
{
	if (m_elapsed >= kSpreadSeconds) {
		return m_radius;
	}
	// 最初に勢いよく広がり、最大に近づくにつれてゆっくりになる。
	const float t = m_elapsed / kSpreadSeconds;
	const float easeOut = 1.0f - (1.0f - t) * (1.0f - t);
	return m_radius * (kSpreadStartRate + (1.0f - kSpreadStartRate) * easeOut);
}

void BurningGround::UpdateEffectScale()
{
	const float scale = GetCurrentRadius() / kEffectRadiusAtScaleOne;
	m_effect.SetScale(Vector3(scale, scale, scale));
	m_effect.Update();
}
