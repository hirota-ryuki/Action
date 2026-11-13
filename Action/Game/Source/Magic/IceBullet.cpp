#include "stdafx.h"
#include "IceBullet.h"
#include "EffectId.h"
#include "HitTarget.h"
#include "StatusEffects.h"

namespace
{
	// エフェクトは Effekseer 用に自作したもの(Assets/effect/ice_*.efkproj が編集元)。
	// テクスチャは Effekseer サンプル(01_NextSoft01, CC-0)のものを使用。
	constexpr int BULLET_EFFECT_ID = static_cast<int>(EffectId::IceBullet);
	constexpr int HIT_EFFECT_ID = static_cast<int>(EffectId::IceHit);
	constexpr const char16_t* BULLET_EFFECT_PATH = u"Assets/effect/ice_bullet.efk";
	constexpr const char16_t* HIT_EFFECT_PATH = u"Assets/effect/ice_hit.efk";

	const Vector3 EFFECT_SCALE = { 10.0f, 10.0f, 10.0f };  // Effekseerの1単位をワールド10単位に拡大。

	BulletSettings MakeSettings(const IceMagicSettings& magicSettings)
	{
		BulletSettings settings;
		settings.speed = magicSettings.speed;
		settings.range = magicSettings.range;
		settings.radius = magicSettings.radius;
		return settings;
	}
}

IceBullet::IceBullet()
	: BulletBase(MakeSettings(IceMagicSettings()))
	, m_freezeSeconds(IceMagicSettings().freezeSeconds)
{
}

IceBullet::~IceBullet()
{
	// Effect のデストラクタは再生を止めないため、ここで明示的に止める。
	// 通常は両方とも再生が終わってから削除されるので何も起きない。
	if (!m_isEffectReady || EffectEngine::GetInstance() == nullptr) {
		return;
	}
	m_bulletEffect.Stop();
	m_hitEffect.Stop();
}

void IceBullet::Setup(const IceMagicSettings& settings)
{
	SetSettings(MakeSettings(settings));
	m_freezeSeconds = settings.freezeSeconds;
}

bool IceBullet::OnStart()
{
	EffectEngine* effectEngine = EffectEngine::GetInstance();
	if (effectEngine == nullptr) {
		K2_ASSERT(false, "EffectEngine が未生成のため氷魔法のエフェクトを再生できません。");
		return false;
	}

	// 登録済みの番号は内部で無視されるので、弾を何発撃っても読み込みは1回だけ。
	effectEngine->ResistEffect(BULLET_EFFECT_ID, BULLET_EFFECT_PATH);
	effectEngine->ResistEffect(HIT_EFFECT_ID, HIT_EFFECT_PATH);
	if (effectEngine->LoadEffect(BULLET_EFFECT_ID) == nullptr || effectEngine->LoadEffect(HIT_EFFECT_ID) == nullptr) {
		K2_ASSERT(false, "氷魔法のエフェクト(Assets/effect/ice_bullet.efk, ice_hit.efk)の読み込みに失敗しました。");
		return false;
	}

	m_bulletEffect.Init(BULLET_EFFECT_ID);
	m_hitEffect.Init(HIT_EFFECT_ID);
	m_isEffectReady = true;

	// 新しいハンドルには行列が設定されていないので、再生直後に OnMove() で必ず設定される。
	m_bulletEffect.Play();
	return true;
}

void IceBullet::OnMove()
{
	m_bulletEffect.SetPosition(GetPosition());
	m_bulletEffect.SetRotation(GetRotation());
	m_bulletEffect.SetScale(EFFECT_SCALE);
	// 全体ではなくルートだけを動かし、軌跡のパーティクルを発生した場所に残す。
	m_bulletEffect.UpdateRoot();
}

void IceBullet::OnHitTarget(HitTarget& target, const Vector3& hitPosition)
{
	(void)hitPosition;
	if (StatusEffects* statusEffects = target.GetStatusEffects()) {
		statusEffects->Freeze(m_freezeSeconds);
	}
}

void IceBullet::OnExplode(const Vector3& hitPosition)
{
	// 弾本体は即座に消し、軌跡のパーティクルは寿命まで残す。
	m_bulletEffect.StopRoot();

	m_hitEffect.Play();
	m_hitEffect.SetPosition(hitPosition);
	m_hitEffect.SetRotation(GetRotation());
	m_hitEffect.SetScale(EFFECT_SCALE);
	m_hitEffect.Update();
}

bool IceBullet::IsVanishFinished() const
{
	// 軌跡のパーティクルと着弾エフェクトが両方消え切ったら終わり。
	return !m_bulletEffect.IsPlay() && !m_hitEffect.IsPlay();
}
