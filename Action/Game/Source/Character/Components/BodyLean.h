#pragma once

// 体の傾きの調整値。
struct BodyLeanSettings
{
	bool isEnabled = false;
	// 速くなるときに前へ、遅くなるときに後ろへ傾く量。実際の物理の傾き（加速と重力の釣り合い）の何倍にするか。
	float accelLeanScale = 0.12f;
	float maxAccelLeanDeg = 8.0f;
	// 速さをならす時間（秒）。移動の速さは一瞬で変わるので、ならしてから加速を求める。
	float velocitySmoothSeconds = 0.15f;
	// 傾きが目標に追いつく時間（秒）。
	float leanSmoothSeconds = 0.1f;
};

// 速くなるときに前へ、遅くなるときに後ろへ体を傾けて、重心が動いているように見せる部品。
// 見た目だけを傾ける（移動・当たり判定の向きは変えない）。足元を中心に傾ける。
class BodyLean
{
public:
	void Init(const BodyLeanSettings& settings);
	// 移動したあとの位置と向きから傾きを求め、描画に使う回転を返す。毎フレーム1回呼ぶ。
	Quaternion Update(const Vector3& position, const Quaternion& rotation, float deltaTime);

private:
	BodyLeanSettings m_settings;
	bool m_isFirstFrame = true;
	Vector3 m_prevPosition = Vector3::Zero;
	Vector3 m_smoothedVelocity = Vector3::Zero;
	float m_pitch = 0.0f;  // 前後の傾き（ラジアン）。
};
