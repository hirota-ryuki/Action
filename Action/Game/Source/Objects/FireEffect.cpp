#include "stdafx.h"
#include "FireEffect.h"
#include "EffectId.h"

namespace
{
	constexpr int kFireEffectId = static_cast<int>(EffectId::Fire);
	constexpr const char16_t* kFireEffectPath = u"Assets/effect/fire.efk";  // Effekseer サンプル(01_Pierre01/Flame, CC-0)を書き出したもの。
	// エフェクトの長さがこれ以上なら無限ループとみなし、再生を重ねない（Effekseer は無限を INT_MAX 付近で表す）。
	constexpr int kInfiniteTermThreshold = 60 * 60 * 60;
}

FireEffect::FireEffect()
{

}

FireEffect::~FireEffect()
{
	// Effect のデストラクタは再生を止めないため、ここで明示的に止める。
	if (!m_isValid || EffectEngine::GetInstance() == nullptr) {
		return;
	}
	for (Effect& effect : m_effects) {
		effect.Stop();
	}
}

bool FireEffect::Start()
{
	EffectEngine* effectEngine = EffectEngine::GetInstance();
	if (effectEngine == nullptr) {
		// ウィンドウ無しで起動した場合など、EffectEngine が作られていない。
		K2_ASSERT(false, "EffectEngine が未生成のため炎エフェクトを再生できません。");
		return true;
	}

	// 登録済みの番号は内部で無視されるので、複数生成しても読み込みは1回だけ。
	effectEngine->ResistEffect(kFireEffectId, kFireEffectPath);
	Effekseer::EffectRef effect = effectEngine->LoadEffect(kFireEffectId);
	if (effect == nullptr) {
		// ファイルが無い・壊れているなど。毎フレーム空再生しないよう無効のまま終える。
		K2_ASSERT(false, "炎エフェクト(Assets/effect/fire.efk)の読み込みに失敗しました。");
		return true;
	}

	// 1つの再生が最も長く残るフレーム数の半分ごとに次を重ねる。
	// 2枠を交互に使うので、枠を使い回す時点で前の再生は消え切っている。
	const Effekseer::EffectTerm term = effect->CalculateTerm();
	if (term.TermMax > 0 && term.TermMax < kInfiniteTermThreshold) {
		m_replayIntervalFrames = (term.TermMax + 1) / 2;
	}

	for (Effect& slot : m_effects) {
		slot.Init(kFireEffectId);
	}
	m_isValid = true;
	PlayNext();

	return true;
}

void FireEffect::Update()
{
	if (!m_isValid) {
		return;
	}

	// EffectEngine は1ゲームフレームにつき1フレーム進めるので、こちらもフレームで数える。
	m_framesSinceLastPlay++;
	const bool isIntervalElapsed = m_replayIntervalFrames > 0 && m_framesSinceLastPlay >= m_replayIntervalFrames;
	const bool isAllStopped = !m_effects[0].IsPlay() && !m_effects[1].IsPlay();
	if (isIntervalElapsed || isAllStopped) {
		PlayNext();
	}

	// 位置・拡大率が変わった時だけ行列を更新する。
	if (m_isTransformDirty) {
		for (Effect& effect : m_effects) {
			ApplyTransform(effect);
		}
		m_isTransformDirty = false;
	}
}

void FireEffect::PlayNext()
{
	Effect& effect = m_effects[m_nextSlot];
	effect.Play();
	// 新しいハンドルには行列が設定されていないので、再生直後に必ず設定する。
	ApplyTransform(effect);

	m_nextSlot = (m_nextSlot + 1) % kSlotCount;
	m_framesSinceLastPlay = 0;
}

void FireEffect::ApplyTransform(Effect& effect)
{
	effect.SetPosition(m_position);
	effect.SetScale(m_scale);
	effect.Update();
}
