#pragma once

#include "PlayerState.h"

// 地面にいるステート（Idle・Walk・Run）の共通部分。
// 遷移の決め方は共通で、違うのはアニメーションと移動の速さだけ。
//   ダッシュボタン → Dash
//   ジャンプボタン → Air（跳ぶ）
//   使うボタン（R） → Drink（ポーションを持っていて、HP が満タンでないとき）
//   足場がない     → Air（落ちる）
//   それ以外       → 入力に合わせて Idle / Walk / Run
// 地上にいる間だけ、拾うボタン（F）で落ちているアイテムを拾う（UpdatePickUp）。
class PlayerGroundState : public PlayerState
{
public:
	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;

	// 入力と足場から、Idle / Walk / Run / Air のどれにいるべきかを決める。
	// ほかのステートが、終わったあとの行き先を決めるのにも使う。
	static PlayerStateId SelectByInput(const PlayerStateContext& context, const CharacterInput& input);

	// 拾うボタンを押した瞬間に、いちばん近いアイテムを1つ拾う。
	// 押し続けて pickupHoldDelaySeconds たったら、pickupHoldIntervalSeconds ごとに1つずつ拾い続ける（まとめて拾う）。
	static void UpdatePickUp(PlayerStateContext& context, const CharacterInput& input, float deltaTime);

protected:
	virtual int GetAnimClip() const = 0;
	virtual float GetMoveSpeed(const PlayerStateContext& context) const = 0;
	// アニメーションの再生速度。足が地面の上を滑らないよう、移動の速さに合わせたいときに変える。
	virtual float GetAnimSpeed(const PlayerStateContext& context) const
	{
		return 1.0f;
	}
};
