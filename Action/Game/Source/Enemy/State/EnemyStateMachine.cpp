#include "stdafx.h"
#include "EnemyStateMachine.h"
#include "EnemyIdleState.h"
#include "EnemyMoveState.h"
#include "EnemyRunState.h"
#include "EnemyFrozenState.h"
#include "EnemyAttackState.h"

EnemyStateMachine::EnemyStateMachine()
{
	Register<EnemyIdleState>();
	Register<EnemyMoveState>();
	Register<EnemyRunState>();
	Register<EnemyAttackState>();
	Register<EnemyFrozenState>();
}
