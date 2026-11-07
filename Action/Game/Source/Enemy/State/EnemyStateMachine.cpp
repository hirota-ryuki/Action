#include "stdafx.h"
#include "EnemyStateMachine.h"
#include "EnemyIdleState.h"
#include "EnemyMoveState.h"
#include "EnemyFrozenState.h"

EnemyStateMachine::EnemyStateMachine()
{
	Register<EnemyIdleState>();
	Register<EnemyMoveState>();
	Register<EnemyFrozenState>();
}
