#pragma once

#include "PlayerState.h"

// 空中にいる。ジャンプボタンで入ったときは跳び、足場から落ちたときはそのまま落ちる。
// 空中でも入力の方向へ動ける（走る入力なら走る速さ）。
// アニメーションは、跳んだときは JumpStart（踏み切って足を引きつける）を流し、jumpToLoopTime まで進んだら
// JumpLoop（空中のループ）へ移る。足場から落ちたときは、はじめから JumpLoop を流す。
//   着地 → Land（着地してしゃがむ）。移動の入力があり landCancelByMove なら、Walk / Run へ直接移る
class PlayerAirState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Air;
	}
	const char* GetName() const override
	{
		return "Air";
	}

	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;

private:
	bool m_isJumpStart = false;  // JumpStart を流している（まだ JumpLoop へ移っていない）。
	float m_clipTime = 0.0f;     // JumpStart の今の位置（秒）。
};
