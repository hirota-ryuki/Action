#pragma once

#include "CharacterInput.h"

// NPCのAI。敵対する相手を探して近づき、届く距離に入ったら攻撃・魔法のボタンを押す入力を作る。
// AIは「どうしたいか」だけを決め、実際の移動や発射はキャラクター側が行う。
// 魔法はキャラクターの MagicCaster に登録したものが出る（登録していなければ、fire を押しても何も出ない）。
class NpcInputSource : public IInputSource
{
public:
	// ボタン1つ分の押し方。
	struct ActionSettings
	{
		bool isEnabled = false;    // このボタンを押すか。
		float range = 700.0f;      // 相手がこの距離以内なら押す。
		float interval = 4.0f;     // 押す間隔（秒）。
		float firstDelay = 1.5f;   // 相手を見つけてから最初に押すまでの秒数。
		float holdSeconds = 0.0f;  // 押し続ける秒数。溜め撃ちの魔法用。0なら1フレームだけ押す。
	};

	struct Settings
	{
		float searchRange = 900.0f;    // この距離以内の敵対する相手を狙う。
		float stopDistance = 90.0f;    // この距離まで近づいたら止まる。
		float moveRate = 1.0f;         // 移動入力の大きさ（0〜1）。
		// 押せるもののうち、range がいちばん短いものを押す（剣が届くなら剣、届かなければ魔法）。
		// どれかを押している間は、ほかのボタンは押さない。
		ActionSettings attack;         // 近接攻撃（剣など）。
		ActionSettings fire;           // メインの魔法（MagicCaster::AddMagic）。
		ActionSettings subFire;        // サブの魔法（MagicCaster::SetSubMagic）。
		float castHeight = 60.0f;      // 足元からこの高さで撃つ。
	};

	NpcInputSource() = default;
	explicit NpcInputSource(const Settings& settings);
	void SetSettings(const Settings& settings)
	{
		m_settings = settings;
	}
	void Update(const InputSourceContext& context, CharacterInput& input) override;

private:
	enum ActionIndex
	{
		ACTION_ATTACK,
		ACTION_FIRE,
		ACTION_SUB_FIRE,
		ACTION_NUM,
	};

	// ボタン1つ分の、押す・待つの状態。
	struct ActionState
	{
		float waitTimer = 0.0f;   // 次に押せるまでの秒数。
		float holdTimer = 0.0f;   // 押し続ける残り秒数。
		bool isPressing = false;  // 押している最中か。
	};

	const ActionSettings& GetActionSettings(int index) const;
	// 待ち時間を進め、押す・離すを決める。distanceSq は相手までの水平の距離の2乗。
	void UpdateActions(float distanceSq, float deltaTime);
	void ReleaseAll();
	void ApplyButtons(CharacterInput& input) const;

	Settings m_settings;
	ActionState m_actions[ACTION_NUM];
	bool m_hasTarget = false;  // 前フレームに相手を見つけていたか。
};
