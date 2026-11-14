#pragma once

#include "PlayerState.h"

// 立ち止まって氷魔法を撃つ3段攻撃。右手で撃つ（Attack1）→ 左手で撃つ（Attack2）→ 両手で撃つ（Attack3）。
// 段に入った瞬間に、カメラが向いている方向（CharacterInput::attackDirection）を向く。
// 弾はクリックした瞬間ではなく、手を突き出したコマ（attackFireTime）で撃つ。撃つフレームに context.attackFire を立てる。
// 1回撃つごとに、撃ち終わり（attackCancelTime）から次の入力を受け付ける。
//   段の途中で発射ボタンを押していた → 撃ち終わりで次の段へ（3段目のあとは続かない）
//   撃ち終わりからは、ダッシュボタン・ジャンプボタン・移動の入力で打ち切れる → Dash / Air / Walk / Run
//   足場がない → Air（いつでも）
//   クリップを最後まで流した → Idle
class PlayerAttackState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Attack;
	}
	const char* GetName() const override
	{
		return "Attack";
	}

	// 地面にいるステート（Idle・Walk・Run・Stop・Land）から、攻撃を始めるか。
	// 一人称（照準がある）ときは攻撃のモーションを出さず、今までどおりクリックした瞬間に撃つ。
	static bool CanStart(const PlayerStateContext& context, const CharacterInput& input);

	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;
	void Exit(PlayerStateContext& context) override;

private:
	// stage 段目（0〜2）を始める。
	void StartStage(PlayerStateContext& context, const CharacterInput& input, int stage);
	// 撃ち終わりを過ぎたか。
	bool IsCancelable(const PlayerStateContext& context) const;
	// 次の段へ進むか（段の途中で押していて、撃ち終わりを過ぎ、まだ最後の段でない）。
	bool WillChain(const PlayerStateContext& context) const;

	int m_stage = 0;             // 今の段（0〜2）。
	float m_clipTime = 0.0f;     // 今の段のクリップの中の位置（秒）。
	bool m_isFirstFrame = false; // 段に入ったフレームか（そのフレームの発射ボタンは、この段を始めた入力なので数えない）。
	bool m_hasFired = false;     // 今の段で、もう撃ったか。
	bool m_isNextQueued = false; // 段の途中で発射ボタンが押されたか（撃ち終わりで次の段へ）。
};
