#pragma once

// プレイヤーのアニメーションクリップの番号。
enum EnPlayerAnimClip
{
	enPlayerAnimClip_Idle,
	enPlayerAnimClip_Walk,
	enPlayerAnimClip_Run,
	enPlayerAnimClip_Jump,
	enPlayerAnimClip_Dash,   // ダッシュ（しゃがむ → 左足で蹴って低く滑る → 右足で着地）を通しで1本。蹴る足はいつも左
	enPlayerAnimClip_StopL,  // 走りから止まる（左足を前へ突っ張り、腰を落として滑る → 起き上がる）
	enPlayerAnimClip_StopR,  // 走りから止まる（右足を前へ突っ張る）
	enPlayerAnimClip_Num,
};

// プレイヤーのアニメーションの切り替えを受け持つ部品。
// ステート同士の受け渡し（次の切り替えだけに使う補間時間、クリップごとの始める位置、走りの位置）をここに持つ。
// ステートは ModelRender を直接触らず、これを通してアニメーションを流す。
class PlayerAnimator
{
public:
	PlayerAnimator();

	// modelRender は初期化済みのもの。defaultBlendSeconds はふだんの補間時間。
	void Init(ModelRender* modelRender, float defaultBlendSeconds);

	// クリップを切り替えて、speed 倍の速さで流す。
	// 予約（SetNextBlend・SetNextStartTime）があれば使い、使ったら全部消す。
	void Play(int clip, float speed = 1.0f);
	// 補間時間と始める位置を指定して切り替える。予約は使わず、消さない。
	void PlayFrom(int clip, float blendSeconds, float startTime, float speed);

	// 次の Play だけで使う補間時間。前のステートが Exit で入れておく。
	void SetNextBlend(float seconds)
	{
		m_nextBlendSeconds = seconds;
	}
	// 次の Play で、そのクリップを始める位置（秒）。
	void SetNextStartTime(int clip, float time);
	// 予約した始める位置。なければ0より小さい値。
	float GetNextStartTime(int clip) const;

	// 走りのアニメーションの今の位置（秒、ループ前の通算）。Run が進め、Stop が踏ん張る足を決めるのに使う。
	float GetRunTime() const
	{
		return m_runTime;
	}
	void SetRunTime(float time)
	{
		m_runTime = time;
	}
	void AddRunTime(float deltaTime)
	{
		m_runTime += deltaTime;
	}

private:
	void ClearNext();

	ModelRender* m_modelRender = nullptr;
	float m_defaultBlendSeconds = 0.2f;
	float m_nextBlendSeconds = -1.0f;            // 0より小さければ、ふだんの補間時間を使う。
	float m_nextStartTimes[enPlayerAnimClip_Num]; // 0より小さければ最初から。
	float m_runTime = 0.0f;
};
