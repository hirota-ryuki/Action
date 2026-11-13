#pragma once

#include "PlayerState.h"

// 空中から着地した。JumpLand（足を着いて深くしゃがむ → 立つ）を landStartTime から流し、その場で止まる。
// 流す範囲・速さは調整値（landStartTime・landEndTime・landAnimSpeed）で決める。
//   ダッシュボタン・ジャンプボタン・足場がない → Dash / Air（いつでも割り込める）
//   移動の入力がある（landCancelByMove のとき） → Walk / Run
//   JumpLand が landEndTime まで進んだ → 入力に合わせて Idle / Walk / Run
class PlayerLandState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Land;
	}
	const char* GetName() const override
	{
		return "Land";
	}

	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;
	void Exit(PlayerStateContext& context) override;

	// 着地した瞬間に、Land へ入るか、着地を飛ばして Idle / Walk / Run へ移るかを決める。Air が使う。
	static PlayerStateId SelectOnLand(const PlayerStateContext& context, const CharacterInput& input);

private:
	float m_clipTime = 0.0f;  // JumpLand の今の位置（秒）。
};
