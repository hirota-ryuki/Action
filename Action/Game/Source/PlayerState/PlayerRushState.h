#pragma once

#include "PlayerState.h"

// ラッシュ中。入った瞬間の移動方向（入力がなければ正面）へ、rushDistance の距離を rushSpeed で一気に進む。
// 途中で向きは変えられない。ランアニメーションの疾走ポーズで止めておく。
// 重力はかかるので、空中で出すと落ちながら進む。
//   進み切った → 入力と足場に合わせて Idle / Walk / Run / Air
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
	Vector3 m_direction = Vector3::AxisZ;  // 進む方向（水平・正規化済み）。
	float m_remainingDistance = 0.0f;      // 残りの距離。0になったら終わり。
};
