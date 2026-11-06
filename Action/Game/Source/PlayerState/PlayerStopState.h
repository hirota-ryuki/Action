#pragma once

#include "PlayerState.h"

// 走っていて移動の入力を離した。すぐには止まらず、走っていた勢いで前へ滑りながら踏ん張って止まる。
// アニメーションは、原神の動画（29.78〜30.89秒）から関節の角度を読み取って作った
// 「前の足を突っ張り、腰を落として滑る → 起き上がって立つ」の通しの1本（StopL / StopR、L・R は前へ突っ張る足）。
// どちらの足で踏ん張るかは、止まった瞬間の走りの位置で、前に着こうとしている足に合わせる。
// 進む速さは、入った瞬間の走る速さから stopSlideSeconds かけてまっすぐ遅くなり、0になる。向きは変えない。
//   ダッシュボタン・ジャンプボタン・足場がない → Dash / Air（いつでも割り込める）
//   移動の入力がある → Walk / Run（止まるのをやめて、また動き出す）
//   stopSeconds たった → Idle
class PlayerStopState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Stop;
	}
	const char* GetName() const override
	{
		return "Stop";
	}

	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;
	void Exit(PlayerStateContext& context) override;

private:
	Vector3 m_direction = Vector3::AxisZ;  // 滑る方向（水平・正規化済み）。
	float m_startSpeed = 0.0f;             // 滑り始めの速さ。
	float m_elapsed = 0.0f;                // 入ってからの時間。
};
