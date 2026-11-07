#pragma once

#include "Character.h"
#include "NpcInputSource.h"
#include "EnemyStateMachine.h"

// 敵キャラクター。
// 入力元（既定ではAI）から CharacterInput を受け取って動くだけで、何をするかはAIが決める。
// 体の動かし方（立ち止まる・進む・凍結）の切り替えは EnemyStateMachine（ステートパターン）が行う。
// 陣営を変えれば（SetFaction）味方や中立のNPCとしても使える。
class Enemy : public Character
{
public:
	Enemy();

	Status& GetStatus() override
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
		return m_npcInputSource;
	}
	void OnStart() override;
	void UpdateAction(float deltaTime) override;

private:
	NpcInputSource m_npcInputSource;
	Status m_status;
	EnemyStateMachine m_stateMachine;
};
