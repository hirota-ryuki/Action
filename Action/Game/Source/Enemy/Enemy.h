#pragma once

#include "Character.h"
#include "BlackKnightInputSource.h"
#include "EnemyStateMachine.h"
#include "EnemyAnimCatalog.h"

// 敵キャラクター（黒騎士）。
// 入力元（既定では黒騎士の AI）から CharacterInput を受け取って動くだけで、何をするかは AI が決める。
// 体の動かし方（立ち止まる・歩く・走る・剣を振る・凍結）の切り替えは EnemyStateMachine（ステートパターン）が行う。
// 陣営を変えれば（SetFaction）味方や中立のNPCとしても使える。
class Enemy : public Character
{
public:
	Enemy();

	Status& GetStatus() override
	{
		return m_status;
	}
	const Status& GetStatus() const override
	{
		return m_status;
	}
	bool IsFrozen() const
	{
		return m_statusEffects.IsFrozen();
	}
	// 今のステート。デバッグ表示用。
	EnemyStateId GetStateId() const
	{
		return m_stateMachine.GetCurrentId();
	}
	const char* GetStateName() const
	{
		return m_stateMachine.GetCurrentName();
	}

protected:
	CharacterConfig MakeConfig() const override;
	IInputSource& GetDefaultInputSource() override
	{
		return m_aiInputSource;
	}
	void OnStart() override;
	void UpdateAction(float deltaTime) override;

private:
	BlackKnightInputSource m_aiInputSource;
	Status m_status;
	EnemyStateMachine m_stateMachine;
	EnemyAnimCatalog m_animCatalog;
	EnemyDamageState m_damage;  // 当たった・倒れて消えた（ステートと受け渡す）。
};
