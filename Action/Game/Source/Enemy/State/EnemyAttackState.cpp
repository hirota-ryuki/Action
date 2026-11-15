#include "stdafx.h"
#include "EnemyAttackState.h"
#include "EnemyGroundState.h"
#include "SkillController.h"

EnemyStateId EnemyAttackState::CheckTransition(const EnemyStateContext& context, const CharacterInput& input) const
{
	// 攻撃中は当たってもひるまない。死んだときだけ倒れる。
	if (EnemyGroundState::CheckDamage(context, false) == EnemyStateId::Dead) {
		return EnemyStateId::Dead;
	}
	// 攻撃が終わるまで待つ。押したボタンに攻撃がない（クールダウン中など）ときは、次のフレームに出る。
	if (context.skills != nullptr && context.skills->IsBusy()) {
		return EnemyStateId::Attack;
	}
	return EnemyGroundState::SelectByInput(context, input);
}

void EnemyAttackState::Exit(EnemyStateContext& context)
{
	// 死んだ・凍ったなどで途中で出るときは、振っている攻撃を止める（終わって出るときは何もしない）。
	if (context.skills != nullptr) {
		context.skills->CancelAll();
	}
}
