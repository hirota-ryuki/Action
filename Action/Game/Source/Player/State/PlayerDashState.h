#pragma once

#include "PlayerState.h"

// ダッシュ中。入った瞬間の移動方向（入力がなければ正面）へ、地面を一蹴りして dashDistance だけ飛び出す。
// アニメーションは、原神の動画から関節の角度を読み取って作った「しゃがむ → 蹴って低く滑る → 着地」の通しの1本
// （DashL / DashR、L・R は蹴る足）。途中でクリップを切り替えず、区間（しゃがむ・飛ぶ・着地）は進み方だけを変える。
// 地面から出すときは、dashCrouchSeconds だけしゃがむ。その間は dashCrouchSpeed から0までゆっくり遅くなる。
// かかる時間は dashDistance ÷ dashSpeed。蹴った瞬間がいちばん速く、だんだん遅くなって止まる（ease-out）。
// 途中で向きは変えられない。重力はかかるので、足場から出ると落ちながら進む。空中では出せない（入口のステートで足場を確かめる）。
// 1回ごとに蹴る足を左右入れ替える。
// 飛んだあとは着地（dashLandSeconds）。着地の間は dashEndSpeed で進む。
// 着地まで終わるまでは入力を受け付けない（dashCanChain が true のときだけ、ダッシュ中の押下を覚えて続けて出す）。
// 着地に入ったとき走るボタンを押したままなら、着地の間に走りのアニメーションへ先に補間しておく（見た目だけ）。
//   着地まで終わった → 入力と足場に合わせて Walk / Run / Air。移動の入力がなければ Stop（勢いで滑って止まる）
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
	// ダッシュのアニメーションを、startTime（秒）から流す。
	void PlayKick(PlayerStateContext& context, float blendSeconds, float startTime = 0.0f);
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
	// 着地を始める。走るなら、走りのアニメーションへの補間もここで始める。
	void BeginLanding(PlayerStateContext& context, const CharacterInput& input);

	Vector3 m_direction = Vector3::AxisZ;  // 進む方向（水平・正規化済み）。
	float m_duration = 0.0f;               // 1回分のダッシュにかかる時間。0ならダッシュしない。
	float m_elapsed = 0.0f;                // 1回分のダッシュを始めてからの時間。
	float m_landElapsed = 0.0f;            // 着地を始めてからの時間。
	float m_crouchRemaining = 0.0f;        // しゃがみ込みの残り時間。0になったら蹴る。
	bool m_isLanding = false;              // 着地中。
	bool m_isLeftKick = false;             // 今回、左足で蹴るか。ダッシュのたびに入れ替える（ステートを出ても覚えておく）。
	bool m_isNextDashQueued = false;       // ダッシュ中にもう一度押された。進み切ったら次のダッシュを出す。
	bool m_isFirstFrame = false;           // 入ったフレーム。入るきっかけになった押下を、次のダッシュの予約と数えないため。
	bool m_isRunBlended = false;           // 着地の間に、走りのアニメーションへ補間し始めた。
	float m_runAnimTime = 0.0f;            // 走りのアニメーションの位置（秒）。出るときに PlayerAnimator へ渡す。
};
