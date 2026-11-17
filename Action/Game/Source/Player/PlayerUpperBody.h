#pragma once
// 三人称の溜め撃ちの調整値。時間はどれもクリップ（Attack3）の中の位置（秒）。
struct PlayerChargeSettings
{
	float holdTime = 7.0f / 30.0f;      // 溜めている間、ここで止める（両手を前へ伸ばしかけたところ）。
	float castEndTime = 17.0f / 30.0f;  // 撃ったあと、ここまで流したら上半身を下の動きへ戻し始める（下ろし始め）。
	float fadeInSeconds = 0.15f;        // 上半身を溜める動きに替えるのにかける時間。
	float fadeOutSeconds = 0.25f;       // 上半身を下の動き（歩きなど）へ戻すのにかける時間。
	float moveSpeedRate = 0.6f;         // 溜めている間の歩く速さ（ふだんの歩きの何倍か）。
	float ballForward = 15.0f;          // 火の玉を、両手の真ん中からこれだけ前（cm）に置く。
};

// 三人称で溜め撃ちをするときの、上半身の動きを受け持つ部品。
// 下半身はステートマシン（立ち止まる・歩くなど）のままにし、上半身だけをアニメーションの層で
// 「両手を前へ伸ばして溜める → 撃つ → 下の動きへ戻す」にする。
class PlayerUpperBody
{
public:
	// modelRender は、InitAnimationLayer で上半身の層を作ってあるもの。
	void Init(ModelRender* modelRender, const PlayerChargeSettings& settings);
	// 毎フレーム、モデルを更新したあとに呼ぶ。
	// isCharging: 溜め撃ちを溜めているか。useCount: 溜め撃ちを撃った回数（増えたら撃った）。
	// isEnabled: false（一人称のとき）なら、すぐに上半身を下の動きへ戻す。
	void Update(bool isCharging, int useCount, bool isEnabled, float deltaTime);
	// 溜める・撃つ動きを止め、下の動きへ戻し始める（ダッシュしたときなど）。
	void Stop();
	// 上半身が溜める・撃つ動きの最中か。肩越しのカメラ・照準・体の向きに使う。
	bool IsAiming() const
	{
		return m_phase != Phase::None;
	}
	// 上半身の層の重み（0〜1）。
	float GetWeight() const
	{
		return m_weight;
	}
	const PlayerChargeSettings& GetSettings() const
	{
		return m_settings;
	}

private:
	enum class Phase
	{
		None,    // 下の動きのまま
		Charge,  // 溜める（holdTime で止める）
		Cast,    // 撃つ（castEndTime まで流す）
	};
	void StartCharge();
	void Apply();

	ModelRender* m_modelRender = nullptr;
	PlayerChargeSettings m_settings;
	Phase m_phase = Phase::None;
	float m_time = 0.0f;         // 層のクリップの今の位置（秒）。
	float m_speed = 0.0f;        // 層の再生速度（止めるときは 0）。
	float m_weight = 0.0f;       // 層の重み（0〜1、少しずつ変える）。
	int m_lastUseCount = 0;      // 前のフレームの撃った回数。
};
