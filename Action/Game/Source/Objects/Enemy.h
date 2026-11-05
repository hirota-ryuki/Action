#pragma once

#include "CharacterInput.h"
#include "NpcInputSource.h"
#include "CharacterMover.h"
#include "CharacterModel.h"
#include "StatusEffects.h"
#include "HitTarget.h"
#include "MagicCaster.h"

// 敵キャラクター。
// 入力元（既定ではAI）から CharacterInput を受け取って動くだけで、どう動くかはAIが決める。
// 陣営を変えれば味方や中立のNPCとしても使える。
class Enemy : public IGameObject
{
public:
	enum EnAnimClip {
		enAnimClip_Idle,
		enAnimClip_Run,
		enAnimClip_Num,
	};

	Enemy();
	~Enemy();
	bool Start();
	void Update();
	void Render(RenderContext& rc);

	void SetPosition(const Vector3& position)
	{
		m_mover.SetPosition(position);
	}
	const Vector3& GetPosition() const
	{
		return m_mover.GetPosition();
	}
	// 陣営。Start() の前後どちらで呼んでもよい。
	void SetFaction(Faction faction);
	Faction GetFaction() const
	{
		return m_faction;
	}

	// 入力元を差し替える（PlayerInputSource を渡すと手で操作できる、などのデバッグ用）。
	// nullptr を渡すとAIに戻る。
	void SetInputSource(IInputSource* inputSource)
	{
		m_inputSource = inputSource != nullptr ? inputSource : &m_npcInputSource;
	}
	const CharacterInput& GetInput() const
	{
		return m_input;
	}
	StatusEffects& GetStatusEffects()
	{
		return m_statusEffects;
	}
	bool IsFrozen() const
	{
		return m_statusEffects.IsFrozen();
	}

private:
	void UpdateAnimation();

	NpcInputSource m_npcInputSource;
	IInputSource* m_inputSource = &m_npcInputSource;
	CharacterInput m_input;
	CharacterMover m_mover;
	StatusEffects m_statusEffects;
	HitTarget m_hitTarget;
	MagicCaster m_magicCaster;

	CharacterModel m_model;
	Faction m_faction = Faction::Enemy;
	int m_currentAnimNo = enAnimClip_Idle;
};
