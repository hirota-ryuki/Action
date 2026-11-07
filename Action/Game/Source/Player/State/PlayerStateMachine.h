#pragma once

#include "PlayerState.h"
#include "StateMachine.h"

// プレイヤーのステートマシン。切り替えの手順と凍結の扱いは共通の StateMachine にある。
// ここでは7つのステートを登録し、Player だけの前処理を書く。
class PlayerStateMachine : public StateMachine<PlayerStateId, PlayerStateContext>
{
public:
	PlayerStateMachine();

	// mover・animator・statusEffects は初期化済みのものを渡す。Idle から始める。
	void Init(const PlayerStateContext& context)
	{
		StateMachine::Init(context, PlayerStateId::Idle);
	}

protected:
	void OnBeforeUpdate(PlayerStateContext& context, const CharacterInput& input) override;
};
