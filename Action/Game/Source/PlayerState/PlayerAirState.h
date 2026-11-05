#pragma once

#include "PlayerState.h"

// 空中にいる。ジャンプボタンで入ったときは跳び、足場から落ちたときはそのまま落ちる。
// 空中でも入力の方向へ動ける（走る入力なら走る速さ）。
//   ラッシュボタン → Rush
//   着地           → 入力に合わせて Idle / Walk / Run
class PlayerAirState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Air;
	}
	const char* GetName() const override
	{
		return "Air";
	}

	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;
};
