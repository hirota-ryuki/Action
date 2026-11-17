#pragma once

#include "PlayerState.h"
#include "StateMachine.h"

// プレイヤーのステートマシン。切り替えの手順と凍結の扱いは共通の StateMachine にある。
// ここでは9つのステートを登録し、Player だけの前処理を書く。
class PlayerStateMachine : public StateMachine<PlayerStateId, PlayerStateContext>
{
public:
	PlayerStateMachine();

	// mover・animator・statusEffects は初期化済みのものを渡す。Idle から始める。
	void Init(const PlayerStateContext& context)
	{
		StateMachine::Init(context, PlayerStateId::Idle);
	}
	// このフレームに、攻撃が手を突き出したコマに来たか（魔法を撃つか）。
	bool IsAttackFire() const
	{
		return GetContext().attackFire;
	}
	// 三人称の溜め撃ちで構えているか。Update の前に Player が入れる。
	void SetAiming(bool isAiming)
	{
		m_isAiming = isAiming;
	}

protected:
	void OnBeforeUpdate(PlayerStateContext& context, const CharacterInput& input) override;

private:
	bool m_isAiming = false;
};
