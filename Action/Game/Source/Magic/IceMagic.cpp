#include "stdafx.h"
#include "IceMagic.h"
#include "EffectId.h"
#include "HitTarget.h"
#include "StatusEffects.h"

namespace
{
	// エフェクトは Effekseer 用に自作したもの(Assets/effect/ice_*.efkproj が編集元)。
	// テクスチャは Effekseer サンプル(01_NextSoft01, CC-0)のものを使用。
	constexpr int FLY_EFFECT_ID = static_cast<int>(EffectId::IceFly);
	constexpr int HIT_EFFECT_ID = static_cast<int>(EffectId::IceHit);
	constexpr const char16_t* FLY_EFFECT_PATH = u"Assets/effect/ice_fly.efk";
	constexpr const char16_t* HIT_EFFECT_PATH = u"Assets/effect/ice_hit.efk";

	const Vector3 EFFECT_SCALE = { 10.0f, 10.0f, 10.0f };  // Effekseerの1単位をワールド10単位に拡大。
	constexpr float FREEZE_SECONDS = 2.5f;                  // 命中した相手を凍らせる時間。

	MagicSettings MakeSettings()
	{
		MagicSettings settings;
		settings.speed = 1500.0f;
		settings.range = 2000.0f;
		settings.radius = 10.0f;
		return settings;
	}
}

IceMagic::IceMagic()
	: MagicBase(MakeSettings())
{
}

IceMagic::~IceMagic()
{
	// Effect のデストラクタは再生を止めないため、ここで明示的に止める。
	// 通常は両方とも再生が終わってから削除されるので何も起きない。
	if (!m_isEffectReady || EffectEngine::GetInstance() == nullptr) {
		return;
	}
	m_flyEffect.Stop();
	m_hitEffect.Stop();
}

bool IceMagic::OnStart()
{
	EffectEngine* effectEngine = EffectEngine::GetInstance();
	if (effectEngine == nullptr) {
		K2_ASSERT(false, "EffectEngine が未生成のため氷魔法のエフェクトを再生できません。");
		return false;
	}

	// 登録済みの番号は内部で無視されるので、何発撃っても読み込みは1回だけ。
	effectEngine->ResistEffect(FLY_EFFECT_ID, FLY_EFFECT_PATH);
	effectEngine->ResistEffect(HIT_EFFECT_ID, HIT_EFFECT_PATH);
	if (effectEngine->LoadEffect(FLY_EFFECT_ID) == nullptr || effectEngine->LoadEffect(HIT_EFFECT_ID) == nullptr) {
		K2_ASSERT(false, "氷魔法のエフェクト(Assets/effect/ice_fly.efk, ice_hit.efk)の読み込みに失敗しました。");
		return false;
	}

	m_flyEffect.Init(FLY_EFFECT_ID);
	m_hitEffect.Init(HIT_EFFECT_ID);
	m_isEffectReady = true;

	// 新しいハンドルには行列が設定されていないので、再生直後に OnMove() で必ず設定される。
	m_flyEffect.Play();
	return true;
}

void IceMagic::OnMove()
{
	m_flyEffect.SetPosition(GetPosition());
	m_flyEffect.SetRotation(GetRotation());
	m_flyEffect.SetScale(EFFECT_SCALE);
	// 全体ではなくルートだけを動かし、軌跡のパーティクルを発生した場所に残す。
	m_flyEffect.UpdateRoot();
}

void IceMagic::OnHitTarget(HitTarget& target, const Vector3& hitPosition)
{
	(void)hitPosition;
	if (StatusEffects* statusEffects = target.GetStatusEffects()) {
		statusEffects->Freeze(FREEZE_SECONDS);
	}
}

void IceMagic::OnExplode(const Vector3& hitPosition)
{
	// 飛んでいる氷は即座に消し、軌跡のパーティクルは寿命まで残す。
	m_flyEffect.StopRoot();

	m_hitEffect.Play();
	m_hitEffect.SetPosition(hitPosition);
	m_hitEffect.SetRotation(GetRotation());
	m_hitEffect.SetScale(EFFECT_SCALE);
	m_hitEffect.Update();
}

bool IceMagic::IsVanishFinished() const
{
	// 軌跡のパーティクルと着弾エフェクトが両方消え切ったら終わり。
	return !m_flyEffect.IsPlay() && !m_hitEffect.IsPlay();
}
