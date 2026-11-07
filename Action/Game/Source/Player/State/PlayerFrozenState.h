#pragma once

#include "PlayerState.h"

// 凍結などで行動できない。入力を無視し、重力で落ちるだけ。
// 行動できなくなったら、どのステートからでも PlayerStateMachine がここへ移す。
// アニメーションの停止と色の変化は StatusEffects が行うので、ここでは触らない。
//   行動できるようになった → 入力と足場に合わせて Idle / Walk / Run / Air
class PlayerFrozenState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Frozen;
	}
	const char* GetName() const override
	{
		return "Frozen";
	}

	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;
};
