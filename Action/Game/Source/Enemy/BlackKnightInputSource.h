#pragma once

#include <random>
#include "CharacterInput.h"

// 黒騎士の AI。敵対する相手を見つけたら、余裕たっぷりに歩いて近づく。
// 相手との距離を「近い・中くらい・遠い」に分けて、何をするかを決める。
//   近い         剣を振る（fire を押す）
//   中くらい・遠い  決め直す間隔ごとに、runChance の確率で走る（isRun）。残りは攻撃（攻撃1・3ができるまでは歩き続ける）
//   走っている    近くまで来たら歩きに戻し、すぐに剣を振る
// AI は「どうしたいか」だけを決め、実際の移動や剣を振る動きはステート（EnemyStateMachine）が行う。
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
		float attackInterval = 2.5f;     // 近いときに剣を振る間隔（秒）。
		float firstAttackDelay = 0.6f;   // 相手を見つけてから最初に振れるまでの秒数。
	};

	// 相手との距離の分け方。
	enum class Range
	{
		Near,
		Middle,
		Far,
	};

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
	float m_attackTimer = 0.0f;    // 次に剣を振れるまでの秒数。
};
