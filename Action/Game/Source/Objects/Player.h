#pragma once

#include "CharacterInput.h"
#include "PlayerInputSource.h"
#include "CharacterMover.h"
#include "CharacterModel.h"
#include "StatusEffects.h"
#include "HitTarget.h"
#include "MagicCaster.h"
#include "PlayerStateMachine.h"

// プレイヤーキャラクター。
// 入力元（既定ではパッド・キーボード・マウス）から CharacterInput を受け取って動くだけで、
// 入力デバイスを直接は読まない。移動・魔法・状態異常・当たり判定は部品に任せ、
// 立ち止まる・歩く・走る・空中・ダッシュ・凍結の切り替えは PlayerStateMachine（ステートパターン）に任せる。
// モデルとアニメーションの読み込みは CharacterModel に任せ、どのモデルを使うかのデータだけを持つ。
// このクラスは部品の組み立てと、剣・リスポーン・一人称の表示切り替えを担う。
class Player : public IGameObject
{
public:
	Player();
	~Player();
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
	Vector3 GetForward() const
	{
		return m_mover.GetForward();
	}
	void SetRenderBody(bool isRenderBody)
	{
		m_isRenderBody = isRenderBody;
	}

	// 入力元を差し替える（AIで動かす、記録した入力を再生する、などのデバッグ用）。
	// nullptr を渡すとパッド・キーボード・マウスの入力に戻る。
	void SetInputSource(IInputSource* inputSource)
	{
		m_inputSource = inputSource != nullptr ? inputSource : &m_playerInputSource;
	}
	// このフレームに受け取った入力。デバッグ表示用。
	const CharacterInput& GetInput() const
	{
		return m_input;
	}
	StatusEffects& GetStatusEffects()
	{
		return m_statusEffects;
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

private:
	void InitSword();
	void UpdateSword();
	void RespawnIfNeeded();
	void Respawn();

	PlayerInputSource m_playerInputSource;
	IInputSource* m_inputSource = &m_playerInputSource;
	CharacterInput m_input;
	CharacterMover m_mover;
	StatusEffects m_statusEffects;
	HitTarget m_hitTarget;
	MagicCaster m_magicCaster;
	PlayerStateMachine m_stateMachine;

	CharacterModel m_model;
	ModelRender m_swordModelRender;
	Vector3 m_respawnPosition = Vector3::Zero;  // Start()時の位置を記録。落下後の復帰先。
	int m_rightHandBoneNo = -1;                 // -1は右手ボーンが見つからなかった状態。
	bool m_isRenderBody = true;                 // 一人称カメラ時はfalseにして体を非表示にする。
};
