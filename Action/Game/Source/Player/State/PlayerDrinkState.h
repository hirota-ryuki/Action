#pragma once

#include "PlayerState.h"

// ポーションを飲んだ。入った瞬間に1つ減らしてまとめて回復し、drinkSeconds の間は硬直する。
// 硬直の間は、歩く速さの drinkMoveSpeedRate 倍でゆっくり歩ける。魔法（Player::CanUseMagic）・ダッシュ・ジャンプはできない。
// 飲むアニメーションはまだないので、止まっていれば Idle、動いていれば Walk のクリップを流す。
//   足場がない          → Air
//   drinkSeconds たった → 入力に合わせて Idle / Walk / Run
class PlayerDrinkState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Drink;
	}
	const char* GetName() const override
	{
		return "Drink";
	}

	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;

	// 地上のステートから Drink へ移れるか（ポーションを持っていて、HP が満タンでない）。
	static bool CanDrink(const PlayerStateContext& context);

private:
	float m_elapsed = 0.0f;  // 入ってからの時間。
	int m_clip = -1;         // 今流しているクリップ（Idle か Walk）。
};
