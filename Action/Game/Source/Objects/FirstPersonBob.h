#pragma once

class Character;

// 一人称カメラの、歩き・走りの揺れと、着地の沈み。
// プレイヤーが進んだ距離で歩みを進め（位相）、足の運びに合わせて、カメラと腕の揺れを計算する。
// - 歩きは、足が着いた瞬間にいちばん低く、片足で立つ真ん中で高くなる。走りは、着いたあと少し遅れて（踏ん張る真ん中で）いちばん低くなる。
// - 左右は、立っている足の側へ寄る（2歩で1往復）。腕は、足と逆の腕が前に出る。
// - 宙にいる間は揺れを止め、着地すると、落ちていた速さに合わせて沈む。
// GameCamera が持ち、一人称のときだけ Update を呼ぶ。
class FirstPersonBob
{
public:
	// 一人称に切り替えたときに呼ぶ。揺れていない状態から始める。
	void Reset(const Character& player);
	// 毎フレーム、カメラを置く前に呼ぶ。
	void Update(const Character& player);

	// カメラを置く位置のずれ（x: カメラの右、y: 上。cm）。
	const Vector3& GetCameraOffset() const
	{
		return m_cameraOffset;
	}
	// カメラの首振り（度。+ で下を向く）。
	float GetPitchDeg() const
	{
		return m_pitchDeg;
	}
	// カメラの傾き（度。+ で右へ傾く）。
	float GetRollDeg() const
	{
		return m_rollDeg;
	}
	// 腕を、カメラに付いた位置からさらにずらす量（x: カメラの右、y: 上、z: 前。cm）。side 0 は画面の右の腕、1 は左の腕。
	const Vector3& GetArmsOffset(int side) const
	{
		return m_armsOffset[side];
	}
	// 腕を前後に振る量（+ で前、- で後ろ。走りで -1〜1、歩きはその半分）。左右の腕で逆向き。
	float GetArmSwing(int side) const
	{
		return m_armSwing[side];
	}

private:
	// 着地の沈みを、ばねで進める。
	void UpdateLanding(float deltaTime);
	// 位相と強さから、カメラと腕のずれを計算する。
	void UpdateOffsets();

	Vector3 m_lastPosition = Vector3::Zero;  // 前のフレームのプレイヤーの位置（速さを測る）。
	bool m_wasOnGround = true;     // 前のフレームに地面に着いていたか（着地を見つける）。
	float m_fallSpeed = 0.0f;      // 宙にいる間の、下へ落ちる速さ（cm/秒）。着地したときの沈む量に使う。
	float m_phase = 0.0f;          // 歩みの進み具合（歩数。0〜2 で、0 で右足、1 で左足が着く）。
	float m_moveWeight = 0.0f;     // 揺れの強さ（0 で止まっている、1 で歩き以上の速さ）。
	float m_runWeight = 0.0f;      // 歩き（0）と走り（1）の混ぜ具合。
	float m_landDip = 0.0f;        // 着地でカメラが沈んでいる量（cm。+ で下）。
	float m_landDipSpeed = 0.0f;   // m_landDip の変わる速さ。
	float m_armsLandDip = 0.0f;    // 着地で腕が、カメラよりさらに沈んでいる量（cm。+ で下）。
	float m_armsLandDipSpeed = 0.0f;
	Vector3 m_cameraOffset = Vector3::Zero;
	float m_pitchDeg = 0.0f;
	float m_rollDeg = 0.0f;
	Vector3 m_armsOffset[2] = { Vector3::Zero, Vector3::Zero };
	float m_armSwing[2] = { 0.0f, 0.0f };
};
