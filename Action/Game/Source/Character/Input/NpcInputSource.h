#pragma once

#include <random>
#include "CharacterInput.h"

// NPCのAI。敵対する相手を見つけたら近づき、相手との距離を「近い・中くらい・遠い」の3段に分けて、押すボタンを決める。
//   近い          攻撃の間隔ごとに、近いときのボタン（nearAction）を押す
//   中くらい・遠い  決め直す間隔ごとに、runChance の確率で走る（isRun）。残りはその段のボタン（midAction・farAction）を押す
//   走っている     近くまで来たら歩きに戻し、すぐに近いときのボタンを押す
// 段ごとのボタンは、そのキャラクターが SkillController に登録したスキル（剣の攻撃でも魔法でもよい）を押す。
// 溜める魔法（炎魔法）のために、ボタンを押し続ける秒数を段ごとに決められる。押し続けている間は、ほかのボタンを押さない。
// AIは「どうしたいか」だけを決め、実際の移動や攻撃・魔法はキャラクター側が行う。
class NpcInputSource : public IInputSource
{
public:
	// 1つの段で押すボタン。
	struct RangeAction
	{
		ActionButton button = ActionButton::Num;  // 押すボタン。Num ならこの段では何も押さない。
		float holdSeconds = 0.0f;                 // 押し続ける秒数。溜める魔法用。0なら1フレームだけ押す。
	};

	struct Settings
	{
		float searchRange = 900.0f;      // この距離以内の敵対する相手を追う（ここまでが「遠い」）。
		float nearRange = 220.0f;        // この距離以内が「近い」。
		float midRange = 600.0f;         // この距離以内が「中くらい」。
		float stopDistance = 150.0f;     // この距離まで近づいたら止まる。
		float moveRate = 1.0f;           // 移動入力の大きさ（0〜1）。
		float decideInterval = 2.0f;     // 中くらい・遠いとき、行動を決め直す間隔（秒）。
		float runChance = 0.5f;          // 中くらい・遠いときに走る確率（0〜1）。
		float actionInterval = 3.5f;     // ボタンを押してから、次に押せるまでの秒数。
		float firstActionDelay = 0.6f;   // 相手を見つけてから最初に押せるまでの秒数。
		float castHeight = 60.0f;        // 足元からこの高さから狙う（魔法を撃つ高さ）。
		RangeAction nearAction;          // 近いときに押すボタン。
		RangeAction midAction;           // 中くらいのときに押すボタン。
		RangeAction farAction;           // 遠いときに押すボタン。
	};

	// 相手との距離の分け方。
	enum class Range
	{
		Near,
		Middle,
		Far,
	};

	NpcInputSource() = default;
	explicit NpcInputSource(const Settings& settings);
	void SetSettings(const Settings& settings)
	{
		m_settings = settings;
	}
	void Update(const InputSourceContext& context, CharacterInput& input) override;

private:
	Range ClassifyRange(float distance) const;
	const RangeAction& GetRangeAction(Range range) const;
	// 押すボタンを決める。押さないなら ActionButton::Num を返す。
	ActionButton DecideButton(Range range, float deltaTime);

	Settings m_settings;
	std::mt19937 m_random{ std::random_device{}() };
	bool m_hasTarget = false;                     // 前フレームに相手を見つけていたか。
	bool m_isRunning = false;                     // 走って近づいている最中か。
	float m_decideTimer = 0.0f;                   // 次に行動を決め直すまでの秒数。
	float m_actionTimer = 0.0f;                   // 次にボタンを押せるまでの秒数。
	ActionButton m_holdButton = ActionButton::Num;  // 押し続けているボタン。Num なら押していない。
	float m_holdTimer = 0.0f;                     // ボタンを押し続ける残り秒数。
};
