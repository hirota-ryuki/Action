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
	// 三人称で溜め撃ちを溜めている最中か。肩越しのカメラに使う（離したらすぐ戻し始める）。
	bool IsThirdPersonCharging() const
	{
		return m_isThirdPersonCamera && m_upperBody.IsCharging();
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
	void OnAddPoseLayers(PoseLayers& layers) override;
	void OnDebugMeasurePose(Skeleton& skeleton, int stage) override;  // 【仮・測る用】

private:
	// ───── 【仮・測る用／比べる用】ここから（コミットしない） ─────
	void UpdateDebugMeasure(float deltaTime);
	void DrawDashCompareUI();
	void SetDebugBeforeFix(bool isBefore);  // true で、直す前（main のダッシュ・腕の調整あり・揺れ物の新しい値なし）にする。
	FILE* m_measureFile = nullptr;
	bool m_isMeasureMode = false;
	bool m_isMeasureBefore = false;
	bool m_isCaptureMode = false;
	bool m_isCaptureWaiting = false;
	float m_measureTimer = 0.0f;
	int m_measureFrame = -1;
	std::vector<std::pair<int, int>> m_skirtBones;
	std::vector<Vector3> m_skirtAnimDir;
	std::vector<Vector3> m_skirtPrevAnimDir;
	std::vector<Vector3> m_skirtPrevFinalDir;
	bool m_isDebugDashRequested = false;
	bool m_isDebugBeforeFix = false;
	bool m_hasDebugSpringBackup = false;
	SpringBonesSettings m_debugSpringBackup;
	float m_debugPlaySpeed = 1.0f;
	int m_debugPoseClip = -1;
	float m_debugPoseFrame = 0.0f;
	bool m_isDebugPoseStopped = false;
	// ───── 【仮】ここまで ─────
	void Respawn();
	// ジャンプ中（Air・Land）の度合いを、体の連動と揺れ物に渡す。入る・出るときは少しずつ変える。
	void UpdateJumpWeight(float deltaTime);
	// 攻撃中（Attack）の度合いを、体の連動に渡す。入る・出るときは少しずつ変える。
	void UpdateAttackWeight(float deltaTime);
	// ダッシュ中（Dash）の度合いを、体の連動に渡す。入る・出るときは少しずつ変える。
	void UpdateDashWeight(float deltaTime);
	// 三人称で溜め撃ちを構えている間の入力にする（走らない・跳ばない・ゆっくり歩く。ダッシュしたら溜めをやめる）。
	void FilterAimingInput(CharacterInput& input);
	// 構えて前・横へ動くときの、腰から下を向ける向き。進む向き（moveDirection）を、
	// カメラの向き（aimForward）から AIM_LEGS_MAX_DEG までに抑える。どちらも水平で長さ 1。
	static Vector3 TurnLegsToward(const Vector3& aimForward, const Vector3& moveDirection);
	// 溜めている火の玉を置く位置を、両手の前に合わせる。
	void UpdateChargePosition();
	// 溜め撃ちを構えている間、脚が横を向いていても、上半身をカメラの向きへひねる。
	void TwistUpperBodyToAim(Skeleton& skeleton);
	// 骨から先を、その骨の位置を中心に、縦の軸まわりに回す。
	static void RotateBoneTreeY(Skeleton& skeleton, int boneId, float angle);
	// 溜め撃ちの構えの調整値を変える窓（F1）。
	void DrawChargeDebugUI();

	PlayerInputSource m_playerInputSource;
	PlayerStatus m_status;
	PlayerAnimator m_animator;
	PlayerStateMachine m_stateMachine;
	FallRespawn m_fallRespawn;
	float m_jumpWeight = 0.0f;  // ジャンプ中（Air・Land）の度合い（0〜1）。
	float m_attackWeight = 0.0f;  // 攻撃中（Attack）の度合い（0〜1）。
	float m_dashWeight = 0.0f;    // ダッシュ中（Dash）の度合い（0〜1）。
	PlayerUpperBody m_upperBody;
	bool m_isThirdPersonCamera = true;
	int m_handBones[2] = { -1, -1 };          // 左手・右手の骨の番号。
	int m_twistBones[3] = { -1, -1, -1 };     // 上半身をひねる骨（背骨・胸・上の胸）の番号。
	int m_rightUpperArmBone = -1;             // 右の上腕の骨の番号（右肩を引いたあと、腕を狙う向きへ戻す）。
	int m_neckBone = -1;                      // 首の骨の番号（右肩を引いたあと、顔を狙う向きへ戻す）。
	Vector3 m_chargePosition = Vector3::Zero; // 溜めている火の玉を置く位置。
};
