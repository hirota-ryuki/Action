#pragma once

#include <random>
#include "CharacterInput.h"

// 黒騎士の AI。敵対する相手を見つけたら、余裕たっぷりに歩いて近づく。
// 相手との距離を「近い・中くらい・遠い」に分けて、何をするかを決める。
//   近い          攻撃2（重い連続斬り。Action2）
//   中くらい・遠い  決め直す間隔ごとに、runChance の確率で走る（isRun）。残りは攻撃
//                  （中くらいなら攻撃1: 溜めてから踏み込んで斬る。Action1、遠いなら攻撃3: 斬撃を5つ飛ばす。Action3）
//   走っている     近くまで来たら歩きに戻し、すぐに攻撃2
// 攻撃は、その攻撃のボタンを1フレーム押す。どのボタンでどの攻撃をするかは、Enemy が SkillController に登録している。
// AI は「どうしたいか」だけを決め、実際の移動や剣の動きはステート（EnemyStateMachine）が行う。
class BlackKnightInputSource : public IInputSource
{
public:
	struct Settings
	{
		float searchRange = 900.0f;      // この距離以内の敵対する相手を追う（ここまでが「遠い」）。
		float nearRange = 220.0f;        // この距離以内が「近い」。
		float midRange = 600.0f;         // この距離以内が「中くらい」。
		float stopDistance = 150.0f;     // この距離まで近づいたら止まる。
		float decideInterval = 2.0f;     // 歩いている間、行動を決め直す間隔（秒）。
		float runChance = 0.5f;          // 中くらい・遠いときに走る確率（0〜1）。
		float attackInterval = 3.5f;     // 近いときに攻撃する間隔（秒）。
		float firstAttackDelay = 0.6f;   // 相手を見つけてから最初に攻撃できるまでの秒数。
		float aimHeight = 110.0f;        // 足元からこの高さから狙う（斬撃を出す高さ）。
	};

	// 相手との距離の分け方。
	enum class Range
	{
		Near,
		Middle,
		Far,
	};

	// 攻撃のボタン（Enemy が SkillController に登録している攻撃）。
	static constexpr ActionButton ATTACK_DASH_SLASH = ActionButton::Action1;   // 攻撃1
	static constexpr ActionButton ATTACK_COMBO = ActionButton::Action2;        // 攻撃2
	static constexpr ActionButton ATTACK_SLASH_WAVE = ActionButton::Action3;   // 攻撃3

	BlackKnightInputSource() = default;
	explicit BlackKnightInputSource(const Settings& settings);
	void Update(const InputSourceContext& context, CharacterInput& input) override;

private:
	Range ClassifyRange(float distance) const;

	Settings m_settings;
	std::mt19937 m_random{ std::random_device{}() };
	bool m_hasTarget = false;      // 前フレームに相手を見つけていたか。
	bool m_isRunning = false;      // 走って近づいている最中か。
	float m_decideTimer = 0.0f;    // 次に行動を決め直すまでの秒数。
	float m_attackTimer = 0.0f;    // 次に攻撃できるまでの秒数。
};
