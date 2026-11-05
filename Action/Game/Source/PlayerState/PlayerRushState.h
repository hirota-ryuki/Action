#pragma once

#include "PlayerState.h"

// ラッシュ中。入った瞬間の移動方向（入力がなければ正面）へ、rushDistance の距離を rushSpeed で一気に進む。
// 途中で向きは変えられない。ランアニメーションの疾走ポーズで止めておく。
// 重力はかかるので、空中で出すと落ちながら進む。
// ラッシュ中にもう一度ラッシュボタンを押すと覚えておき、進み切ったらすぐ次のラッシュを出す（連打で続けて出る）。
//   進み切った → もう一度押していれば次のラッシュ、そうでなければ入力と足場に合わせて Idle / Walk / Run / Air
//   （ボタンを押しっぱなしなら Run になる）
class PlayerRushState : public PlayerState
{
public:
	PlayerStateId GetId() const override
	{
		return PlayerStateId::Rush;
	}
	const char* GetName() const override
	{
		return "Rush";
	}

	void Enter(PlayerStateContext& context, const CharacterInput& input) override;
	PlayerStateId CheckTransition(const PlayerStateContext& context, const CharacterInput& input) const override;
	void Update(PlayerStateContext& context, const CharacterInput& input, float deltaTime) override;
	void Exit(PlayerStateContext& context) override;

private:
	// 1回分のラッシュを始める。方向と残りの距離を決め直す。
	void Begin(PlayerStateContext& context, const CharacterInput& input);

	Vector3 m_direction = Vector3::AxisZ;  // 進む方向（水平・正規化済み）。
	float m_remainingDistance = 0.0f;      // 残りの距離。0になったら終わり。
	bool m_isNextRushQueued = false;       // ラッシュ中にもう一度押された。進み切ったら次のラッシュを出す。
	bool m_isFirstFrame = false;           // 入ったフレーム。入るきっかけになった押下を、次のラッシュの予約と数えないため。
};
