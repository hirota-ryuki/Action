#pragma once

#include "PlayerState.h"

// プレイヤーのステートを持ち、毎フレーム遷移と更新を行う。
// 行動できない（凍結中など）ときは、今のステートによらず Frozen へ移す。
class PlayerStateMachine
{
public:
	PlayerStateMachine();
	~PlayerStateMachine();

	// mover・animator・statusEffects は初期化済みのものを渡す。Idle から始める。
	void Init(const PlayerStateContext& context);
	void Update(const CharacterInput& input, float deltaTime);
	// 最初のステートに戻す（リスポーン時など）。行動できないときは Frozen にする。
	void Reset();

	PlayerStateId GetCurrentId() const;
	// デバッグ表示用。
	const char* GetCurrentName() const;

private:
	void ChangeState(PlayerStateId next, const CharacterInput& input);
	bool CanAct() const;

	PlayerStateContext m_context;
	std::unique_ptr<PlayerState> m_states[static_cast<int>(PlayerStateId::Num)];
	PlayerState* m_current = nullptr;
};
