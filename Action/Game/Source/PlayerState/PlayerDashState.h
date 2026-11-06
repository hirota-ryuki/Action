#pragma once

#include "PlayerState.h"

// ダッシュ中。入った瞬間の移動方向（入力がなければ正面）へ、地面を一蹴りして dashDistance だけ飛び出す。
// 地面から出すときは、蹴る前に dashCrouchSeconds だけしゃがみ込む（DashCrouchL / DashCrouchR）。その間は dashCrouchSpeed で進む。
// かかる時間は dashDistance ÷ dashSpeed。蹴った瞬間がいちばん速く、だんだん遅くなって止まる（ease-out）。
// 途中で向きは変えられない。重力はかかるので、空中で出すと落ちながら進む。
// アニメーションは、体全体を前へ倒した「一蹴り」（DashL / DashR、ループしない）を1回流し、宙へ飛び出したポーズで止める。
// 1回ごとに蹴る足を左右入れ替える。
// 飛んだあとは、前の足の膝を曲げて着地する（DashLandL / DashLandR、dashLandSeconds）。着地の間は dashEndSpeed で進む。
// ダッシュ中にもう一度ダッシュボタンを押すと覚えておき、進み切ったらすぐ次のダッシュを出す（連打で続けて出る）。
//   進み切った → もう一度押していれば次のダッシュ、そうでなければ入力と足場に合わせて Idle / Walk / Run / Air
//   （ボタンを押しっぱなしなら Run になる）
class PlayerDashState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Dash;
	}
	const char* GetName() const override
	{
		return "Dash";
	}

	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;
	void Exit(PlayerStateContext& context) override;

private:
	// 1回分のダッシュを始める。方向を決め直し、蹴る足を入れ替える。アニメーションは流さない。
	void Begin(PlayerStateContext& context, const CharacterInput& input);
	// 蹴るアニメーションを流す。
	void PlayKick(PlayerStateContext& context, float blendSeconds);
	// しゃがみ込んでいる間か。
	bool IsCrouching() const
	{
		return m_crouchRemaining > 0.0f;
	}
	// 蹴って飛ぶところ（進む距離が決まっているところ）が終わったか。
	bool IsKickFinished() const
	{
		return m_elapsed >= m_duration;
	}
	// 着地まで終わったか。
	bool IsFinished() const
	{
		return IsKickFinished() && !m_isLanding;
	}
	// 着地を始める。
	void BeginLanding(PlayerStateContext& context);

	Vector3 m_direction = Vector3::AxisZ;  // 進む方向（水平・正規化済み）。
	float m_duration = 0.0f;               // 1回分のダッシュにかかる時間。0ならダッシュしない。
	float m_elapsed = 0.0f;                // 1回分のダッシュを始めてからの時間。
	float m_landElapsed = 0.0f;            // 着地を始めてからの時間。
	float m_crouchRemaining = 0.0f;        // しゃがみ込みの残り時間。0になったら蹴る。
	bool m_isLanding = false;              // 着地中。
	bool m_isLeftKick = false;             // 今回、左足で蹴るか。ダッシュのたびに入れ替える（ステートを出ても覚えておく）。
	bool m_isNextDashQueued = false;       // ダッシュ中にもう一度押された。進み切ったら次のダッシュを出す。
	bool m_isFirstFrame = false;           // 入ったフレーム。入るきっかけになった押下を、次のダッシュの予約と数えないため。
};
