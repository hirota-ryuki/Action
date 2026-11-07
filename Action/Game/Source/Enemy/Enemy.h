#pragma once

#include "Character.h"
#include "NpcInputSource.h"

// 敵キャラクター。
// 入力元（既定ではAI）から CharacterInput を受け取って動くだけで、どう動くかはAIが決める。
// 陣営を変えれば（SetFaction）味方や中立のNPCとしても使える。
class Enemy : public Character
{
public:
	enum EnAnimClip {
		enAnimClip_Idle,
		enAnimClip_Run,
		enAnimClip_Num,
	};

	Enemy();

	Status& GetStatus() override
	{
		return m_status;
	}
	bool IsFrozen() const
	{
		return m_statusEffects.IsFrozen();
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
	void UpdateAnimation();

	NpcInputSource m_npcInputSource;
	Status m_status;
	int m_currentAnimNo = enAnimClip_Idle;
};
