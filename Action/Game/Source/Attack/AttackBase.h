#pragma once

#include "SkillBase.h"

// 攻撃ごとの数値。時間はアニメーションの始まりからの秒数（30コマで1秒、クリップの先頭が 0コマ目）。
struct AttackData
{
	int clip = 0;                  // 流すアニメーション。
	float chargeAt = -1.0f;        // この時間のコマでアニメーションを止めて溜める。負なら溜めない。
	float chargeSeconds = 0.0f;    // 溜める秒数。
	const float* root = nullptr;   // コマごとに前へ進んだ距離（モデルの m）。nullptr なら進まない。
	int rootNum = 0;
};

// 剣の攻撃の基底クラス。スキル（SkillBase）の派生で、攻撃するたびに1つ作り、1回の攻撃そのものになる。
// 持ち主の体（SkillBody）を借りて、アニメーションを流し、溜めのコマで止め、アニメーションの中で進む分だけ前へ動かす。
// 振り終わる（アニメーションが終わる）まで、持ち主は手を離せない（ほかのスキルを使えない）。
// キャラクターより先に体を動かすので、優先度0で作る。
//
// 派生クラスが書くもの:
//   GetData()          攻撃ごとの数値（アニメーション・溜め・進む距離の表）。
//   OnSwing()          毎フレーム。斬撃を出すコマなどを見る。
class AttackBase : public SkillBase
{
public:
	static constexpr int PRIORITY = 0;

protected:
	virtual const AttackData& GetData() const = 0;
	virtual void OnSwing(float time) { (void)time; }

	// 体の位置と、体の前（水平、正規化済み）。
	const Vector3& GetBodyPosition() const;
	Vector3 GetBodyForward() const;

private:
	bool UsesOwnerBody() const final
	{
		return true;
	}
	bool OnStart() final;
	void OnActivate() final;
	void OnSkillUpdate() final;
	// コマごとの距離の表を、time 秒のところで読む（間はまっすぐつなぐ）。
	float SampleRoot(float time) const;

	float m_time = 0.0f;           // 攻撃を始めてからの秒数（溜めている間は進めない）。
	bool m_isCharging = false;     // 溜めのコマで止めている。
	bool m_isChargeDone = false;   // 溜め終わった。
	float m_chargeLeft = 0.0f;     // 溜めの残りの秒数。
};
