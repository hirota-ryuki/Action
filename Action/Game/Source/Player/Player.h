#pragma once

#include "Character.h"
#include "PlayerInputSource.h"
#include "PlayerStatus.h"
#include "PlayerAnimator.h"
#include "PlayerStateMachine.h"
#include "WeaponAttachment.h"
#include "FallRespawn.h"

// プレイヤーキャラクター。
// 共通の部品と1フレームの順番は Character にある。ここでは Player だけの部品を組み立て、「動く」部分を書く。
//   動く: 立ち止まる・歩く・走る・空中・ダッシュ・凍結の切り替えは PlayerStateMachine（ステートパターン）
//   アニメーション: PlayerAnimator、剣: WeaponAttachment、落下: FallRespawn
// 調整値は PlayerConfig にある。
class Player : public Character
{
public:
	Player();

	PlayerStatus& GetStatus() override
	{
		return m_status;
	}
	// 今のステート。デバッグ表示用。
	PlayerStateId GetStateId() const
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
		return m_playerInputSource;
	}
	void OnStart() override;
	void FillInputContext(InputSourceContext& context) const override;
	void UpdateAction(float deltaTime) override;
	void OnLateUpdate(float deltaTime) override;
	void OnRender(RenderContext& rc) override;

private:
	void Respawn();

	PlayerInputSource m_playerInputSource;
	PlayerStatus m_status;
	PlayerAnimator m_animator;
	PlayerStateMachine m_stateMachine;
	WeaponAttachment m_sword;
	FallRespawn m_fallRespawn;
};
