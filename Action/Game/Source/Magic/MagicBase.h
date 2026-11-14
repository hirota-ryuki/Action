#pragma once

#include "SkillBase.h"

class HitTarget;

// 魔法の性能。
struct MagicSettings
{
	float speed = 1500.0f;   // 毎秒の移動距離。
	float range = 2000.0f;   // これだけ飛ぶと消える。
	float radius = 10.0f;    // 当たり判定の半径。相手の当たり判定の半径と足して判定する。
};

// 飛んでいく魔法の基底クラス。スキル（SkillBase）の派生で、撃つたびに1つ作り、飛んでいく1発そのものになる。
// 溜める・離す・中断は SkillBase が扱う。使い始めたら（溜めない魔法は作った直後から）まっすぐ飛び、
// 敵対する相手か地形に当たるか射程が尽きると着弾する。着弾後は演出が終わるのを待ってから自分を削除する。
//
// 派生クラスは見た目と効果だけを書く（SkillBase の関数のほかに）:
//   OnMove()           位置が変わったときに見た目を合わせる。
//   OnHitTarget()      敵対する相手に当たったときの効果（凍結など）。
//   OnExplode()        着弾したときの演出（相手・地形・射程切れのすべてで呼ばれる）。
//   溜め具合で性能を変えるなら、OnRelease() の中で SetSettings() する。
class MagicBase : public SkillBase
{
protected:
	explicit MagicBase(const MagicSettings& settings);

	// 溜め具合などで性能を変えたいときに、OnRelease() の中で呼ぶ。
	void SetSettings(const MagicSettings& settings)
	{
		m_settings = settings;
	}

	virtual void OnMove() {}
	virtual void OnHitTarget(HitTarget& target, const Vector3& hitPosition) { (void)target; (void)hitPosition; }
	virtual void OnExplode(const Vector3& hitPosition) { (void)hitPosition; }

private:
	void OnActivate() final;
	void OnSkillUpdate() final;
	void Explode(const Vector3& hitPosition);

	MagicSettings m_settings;
	float m_flightDistance = 0.0f;  // ここまでに飛んだ距離。射程判定に使う。
};
