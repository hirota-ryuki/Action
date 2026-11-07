#pragma once

#include "Faction.h"

// 炎魔法の着弾後に、しばらく燃え続ける床。
// 着弾の飛び散りに合わせて、中心から一気に燃え広がる。
// 燃えている間、範囲に入った敵対する相手を燃焼状態にする（StatusEffects::Burn）。
// 時間が来たら炎の発生を止め、残りの炎が消え切ってから自分を削除する。
class BurningGround : public IGameObject
{
public:
	// NewGO の名前。Game が終了時にまとめて削除するのに使う。
	static constexpr const char* OBJECT_NAME = "burningGround";

	~BurningGround();

	// NewGO 直後、Start() より前に呼ぶ。
	// position: 床の位置。radius: 燃える範囲の半径。seconds: 燃えている時間。
	// owner には効かない。owner は削除済みになることがあるので比較にだけ使う。
	void Init(const Vector3& position, float radius, float seconds, const IGameObject* owner, Faction faction);

	bool Start();
	void Update();

private:
	enum class EnState
	{
		Burning,    // 燃えている。範囲に入った相手を燃やす。
		Vanishing,  // 燃え終わった。残りの炎が消えるのを待つ。
	};

	void BurnTargetsInRange();
	// 今燃えている範囲の半径。燃え広がっている途中は radius より小さい。
	float GetCurrentRadius() const;
	void UpdateEffectScale();

	Effect m_effect;
	Vector3 m_position = Vector3::Zero;
	const IGameObject* m_owner = nullptr;
	float m_radius = 0.0f;
	float m_seconds = 0.0f;
	float m_elapsed = 0.0f;
	Faction m_faction = Faction::Neutral;
	EnState m_state = EnState::Burning;
	bool m_isValid = false;           // エフェクトを読み込めたか。
	bool m_isSpreadFinished = false;  // 最大の広さまで燃え広がったか。
};
