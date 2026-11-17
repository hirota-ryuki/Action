#pragma once

#include "Character.h"
#include "PlayerInputSource.h"
#include "PlayerStatus.h"
#include "PlayerAnimator.h"
#include "PlayerStateMachine.h"
#include "PlayerUpperBody.h"
#include "FallRespawn.h"

// プレイヤーキャラクター。
// 共通の部品と1フレームの順番は Character にある。ここでは Player だけの部品を組み立て、「動く」部分を書く。
//   動く: 立ち止まる・歩く・走る・空中・ダッシュ・攻撃・凍結の切り替えは PlayerStateMachine（ステートパターン）
//   アニメーション: PlayerAnimator、落下: FallRespawn
// 調整値は PlayerConfig にある。
class Player : public Character
{
public:
	Player();

	PlayerStatus& GetStatus() override
	{
		return m_status;
	}
	const PlayerStatus& GetStatus() const override
	{
		return m_status;
	}
	// カメラが三人称（寄り・俯瞰）か。GameCamera が毎フレーム渡す。三人称のときだけ、溜め撃ちで上半身を構える。
	void SetThirdPersonCamera(bool isThirdPerson)
	{
		m_isThirdPersonCamera = isThirdPerson;
	}
	// 三人称で溜め撃ちを構えている（溜めている・撃っている）最中か。肩越しのカメラと照準に使う。
	bool IsThirdPersonAiming() const
	{
		return m_isThirdPersonCamera && m_upperBody.IsAiming();
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
	void FilterSkillInput(CharacterInput& input) const override;
	void OnLateUpdate(float deltaTime) override;

private:
	void Respawn();
	// ジャンプ中（Air・Land）の度合いを、体の連動と揺れ物に渡す。入る・出るときは少しずつ変える。
	void UpdateJumpWeight(float deltaTime);
	// 攻撃中（Attack）の度合いを、体の連動に渡す。入る・出るときは少しずつ変える。
	void UpdateAttackWeight(float deltaTime);
	// 三人称で溜め撃ちを構えている間の入力にする（走らない・跳ばない・ゆっくり歩く。ダッシュしたら溜めをやめる）。
	void FilterAimingInput(CharacterInput& input);
	// 溜めている火の玉を置く位置を、両手の前に合わせる。
	void UpdateChargePosition();

	PlayerInputSource m_playerInputSource;
	PlayerStatus m_status;
	PlayerAnimator m_animator;
	PlayerStateMachine m_stateMachine;
	FallRespawn m_fallRespawn;
	float m_jumpWeight = 0.0f;  // ジャンプ中（Air・Land）の度合い（0〜1）。
	float m_attackWeight = 0.0f;  // 攻撃中（Attack）の度合い（0〜1）。
	PlayerUpperBody m_upperBody;
	bool m_isThirdPersonCamera = true;
	int m_handBones[2] = { -1, -1 };          // 左手・右手の骨の番号。
	Vector3 m_chargePosition = Vector3::Zero; // 溜めている火の玉を置く位置。
};
