#include "stdafx.h"
#include "PlayerStateMachine.h"
#include "CharacterInput.h"
#include "StatusEffects.h"
#include "PlayerIdleState.h"
#include "PlayerWalkState.h"
#include "PlayerRunState.h"
#include "PlayerAirState.h"
#include "PlayerDashState.h"
#include "PlayerStopState.h"
#include "PlayerFrozenState.h"

PlayerStateMachine::PlayerStateMachine()
{
	m_states[static_cast<int>(PlayerStateId::Idle)] = std::make_unique<PlayerIdleState>();
	m_states[static_cast<int>(PlayerStateId::Walk)] = std::make_unique<PlayerWalkState>();
	m_states[static_cast<int>(PlayerStateId::Run)] = std::make_unique<PlayerRunState>();
	m_states[static_cast<int>(PlayerStateId::Air)] = std::make_unique<PlayerAirState>();
	m_states[static_cast<int>(PlayerStateId::Dash)] = std::make_unique<PlayerDashState>();
	m_states[static_cast<int>(PlayerStateId::Stop)] = std::make_unique<PlayerStopState>();
	m_states[static_cast<int>(PlayerStateId::Frozen)] = std::make_unique<PlayerFrozenState>();
}

PlayerStateMachine::~PlayerStateMachine()
{
}

void PlayerStateMachine::Init(const PlayerStateContext& context)
{
	m_context = context;
	m_current = nullptr;
	Reset();
}

void PlayerStateMachine::Reset()
{
	const PlayerStateId first = CanAct() ? PlayerStateId::Idle : PlayerStateId::Frozen;
	ChangeState(first, CharacterInput());
}

void PlayerStateMachine::Update(const CharacterInput& input, float deltaTime)
{
	if (m_current == nullptr) {
		return;
	}

	const PlayerStateId next = CanAct() ? m_current->CheckTransition(m_context, input) : PlayerStateId::Frozen;
	if (next != m_current->GetId()) {
		ChangeState(next, input);
	}
	m_current->Update(m_context, input, deltaTime);
}

void PlayerStateMachine::ChangeState(PlayerStateId next, const CharacterInput& input)
{
	if (m_current != nullptr) {
		m_current->Exit(m_context);
		m_context.previousId = m_current->GetId();
	}
	m_current = m_states[static_cast<int>(next)].get();
	m_current->Enter(m_context, input);
}

bool PlayerStateMachine::CanAct() const
{
	return m_context.statusEffects == nullptr || m_context.statusEffects->CanAct();
}

PlayerStateId PlayerStateMachine::GetCurrentId() const
{
	return m_current != nullptr ? m_current->GetId() : PlayerStateId::Idle;
}

const char* PlayerStateMachine::GetCurrentName() const
{
	return m_current != nullptr ? m_current->GetName() : "None";
}
