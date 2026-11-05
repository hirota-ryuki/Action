#pragma once

#include "CharacterInput.h"

// NPCのAI。敵対する相手を探して近づき、射程に入ったら魔法を撃つ入力を作る。
// AIは「どうしたいか」だけを決め、実際の移動や発射はキャラクター側が行う。
class NpcInputSource : public IInputSource
{
public:
	struct Settings
	{
		float searchRange = 900.0f;    // この距離以内の敵対する相手を狙う。
		float stopDistance = 90.0f;    // この距離まで近づいたら止まる。
		float moveRate = 1.0f;         // 移動入力の大きさ（0〜1）。
		bool canFire = true;           // 魔法を撃つか。
		float fireRange = 700.0f;      // この距離以内なら撃つ。
		float fireInterval = 4.0f;     // 撃つ間隔（秒）。
		float firstFireDelay = 1.5f;   // 相手を見つけてから最初に撃つまでの秒数。
		float fireHoldSeconds = 0.0f;  // 発射ボタンを押し続ける秒数。長押しの魔法用。0なら1フレームだけ押す。
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
	Settings m_settings;
	float m_fireTimer = 0.0f;      // 次に撃てるまでの秒数。
	float m_fireHoldTimer = 0.0f;  // 発射ボタンを押し続ける残り秒数。
	bool m_hasTarget = false;      // 前フレームに相手を見つけていたか。
	bool m_isFiring = false;       // 発射ボタンを押している最中か。
};
