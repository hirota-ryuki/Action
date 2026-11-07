#include "stdafx.h"
#include "FireMagic.h"
#include "FireBullet.h"
#include "EffectId.h"

namespace
{
	constexpr float FULL_PULSE_SPEED = 18.0f;  // 最大まで溜まった後に火の玉が脈打つ速さ（ラジアン/秒）。
	constexpr float FULL_PULSE_DEPTH = 0.12f;  // 脈打ちで大きさが変わる割合。
}

FireMagic::FireMagic(float cooldown, float maxChargeSeconds)
	: m_cooldownSeconds(cooldown)
	, m_maxChargeSeconds(maxChargeSeconds > 0.0f ? maxChargeSeconds : 1.0f)
{
}

FireMagic::~FireMagic()
{
	// Effect のデストラクタは再生を止めないため、ここで明示的に止める。
	if (!m_isEffectReady || EffectEngine::GetInstance() == nullptr) {
		return;
	}
	m_chargeEffect.Stop();
	m_chargedEffect.Stop();
}

void FireMagic::OnPress(const MagicCastContext& context)
{
	if (!IsReady()) {
		// クールダウン中に押した分は溜めない。離しても撃たない。
		return;
	}
	if (!m_isEffectReady && FireBullet::RegisterEffects()) {
		m_chargeEffect.Init(static_cast<int>(EffectId::FireCharge));
		m_chargedEffect.Init(static_cast<int>(EffectId::FireCharged));
		m_isEffectReady = true;
	}

	m_isCharging = true;
	m_isFullCharged = false;
	m_chargeSeconds = 0.0f;
	if (m_isEffectReady) {
		m_chargeEffect.Play();
		UpdateChargeEffect(context);
	}
}

void FireMagic::OnHold(const MagicCastContext& context)
{
	if (!m_isCharging) {
		return;
	}
	m_chargeSeconds += context.deltaTime;
	if (m_chargeSeconds >= m_maxChargeSeconds) {
		m_chargeSeconds = m_maxChargeSeconds;
		if (!m_isFullCharged) {
			// 溜まり切った瞬間に一度だけ閃光を出す。
			m_isFullCharged = true;
			m_fullElapsed = 0.0f;
			if (m_isEffectReady) {
				m_chargedEffect.Play();
			}
		}
		else {
			m_fullElapsed += context.deltaTime;
		}
	}
	UpdateChargeEffect(context);
}

void FireMagic::OnRelease(const MagicCastContext& context)
{
	if (!m_isCharging) {
		return;
	}
	FireBullet* bullet = NewGO<FireBullet>(1, BulletBase::OBJECT_NAME);
	bullet->SetCharge(GetChargeRate());
	bullet->Init(context.origin, context.direction, context.caster, context.faction);
	StartCooldown(m_cooldownSeconds);
	StopCharge();
}

void FireMagic::OnCancel()
{
	// 溜めは捨てる（撃たない）。
	StopCharge();
}

float FireMagic::GetChargeRate() const
{
	return m_chargeSeconds / m_maxChargeSeconds;
}

void FireMagic::UpdateChargeEffect(const MagicCastContext& context)
{
	if (!m_isEffectReady) {
		return;
	}
	Quaternion rotation;
	rotation.SetRotationYFromDirectionXZ(context.direction);
	m_chargeEffect.SetPosition(context.origin);
	m_chargeEffect.SetRotation(rotation);
	// 撃つ弾と同じ大きさにして、溜めた火の玉がそのまま飛んでいくように見せる。
	Vector3 scale = FireBullet::GetBulletEffectScale(GetChargeRate());
	if (m_isFullCharged) {
		// 溜まり切った後は脈打たせて、撃てる状態だと分かるようにする。
		scale *= 1.0f + FULL_PULSE_DEPTH * sinf(m_fullElapsed * FULL_PULSE_SPEED);
	}
	m_chargeEffect.SetScale(scale);
	m_chargeEffect.UpdateRoot();

	if (m_isFullCharged) {
		// 閃光は火の玉についていかせる。
		m_chargedEffect.SetPosition(context.origin);
		m_chargedEffect.SetRotation(rotation);
		m_chargedEffect.SetScale(FireBullet::GetBulletEffectScale(1.0f));
		m_chargedEffect.Update();
	}
}

void FireMagic::StopCharge()
{
	m_isCharging = false;
	m_isFullCharged = false;
	m_chargeSeconds = 0.0f;
	if (m_isEffectReady) {
		// 火の玉は即座に消し、立ち上る炎と煙は寿命まで残す。
		m_chargeEffect.StopRoot();
	}
}
