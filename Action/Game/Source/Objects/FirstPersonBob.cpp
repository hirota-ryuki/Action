#include "stdafx.h"
#include "FirstPersonBob.h"

#include "Character.h"

namespace
{
	// 歩きと走りの速さ（cm/秒）。player.json の move の walkSpeed・runSpeed と同じにする。
	constexpr float WALK_SPEED = 170.0f;
	constexpr float RUN_SPEED = 960.0f;
	// 1歩の長さ（cm）。歩きは 1秒に約1.9歩、走りは約3歩になる。速く走るほど、歩幅が広がり、足の回転も速くなる。
	constexpr float WALK_STEP_LENGTH = 90.0f;
	constexpr float RUN_STEP_LENGTH = 320.0f;
	// 走りでいちばん低くなるところ（1歩の中の割合。0 で足が着いた瞬間）。歩きは 0（着いた瞬間）。
	constexpr float RUN_LOWEST_STEP_RATE = 0.2f;
	// 揺れの大きさ（片側の振れ幅。cm・度）。歩きと走りの間は、走りの混ぜ具合で混ぜる。
	// Apex Legends くらいに抑える: 歩きはほとんど揺れず、走りでも小さく揺れる（動きの大半は腕に任せる）。
	constexpr float WALK_VERTICAL = 0.6f;
	constexpr float RUN_VERTICAL = 1.5f;
	constexpr float WALK_HORIZONTAL = 0.4f;
	constexpr float RUN_HORIZONTAL = 0.8f;
	constexpr float WALK_ROLL_DEG = 0.0f;  // 傾き。歩きでは傾けない。
	constexpr float RUN_ROLL_DEG = 0.4f;
	constexpr float RUN_PITCH_DEG = 0.3f;  // 走りで、足が着いた瞬間に下を向く角度。歩きでは向かない。
	// 腕の揺れの大きさ（cm）。腕全体は8の字（左右は2歩で1往復、上下は1歩で1往復）。
	constexpr float WALK_ARMS_HORIZONTAL = 0.8f;
	constexpr float RUN_ARMS_HORIZONTAL = 2.0f;
	constexpr float WALK_ARMS_VERTICAL = 0.5f;
	constexpr float RUN_ARMS_VERTICAL = 1.5f;
	// 腕を前後に振る量（1 で FirstPersonArms の振り切った形）。左右の腕で逆向き。歩きは軽く振る。
	constexpr float WALK_ARMS_SWING = 0.5f;
	constexpr float RUN_ARMS_SWING = 1.0f;
	// 腕が、カメラの揺れより遅れる量（1歩の中の割合）。
	constexpr float ARMS_LAG_STEP_RATE = 0.1f;
	// 揺れの強さ・走りの混ぜ具合を、目標へ近づける速さ（1秒あたり）。歩き出し・止まりで急に揺れないようにする。
	constexpr float MOVE_WEIGHT_SPEED = 8.0f;
	constexpr float RUN_WEIGHT_SPEED = 6.0f;
	// 着地の沈み。落ちていた速さが LAND_MIN_FALL_SPEED を超えた分に LAND_DIP_PER_SPEED を掛け、LAND_DIP_MAX で止める（cm）。
	// ジャンプ（400）で着くと、約3.6cm 沈む。
	constexpr float LAND_MIN_FALL_SPEED = 100.0f;
	constexpr float LAND_DIP_PER_SPEED = 0.012f;
	constexpr float LAND_DIP_MAX = 8.0f;
	// 沈んでから戻るばねの速さ（1/秒）。いちばん沈むのは 1 / これ 秒後（約0.07秒）、0.3秒くらいでほぼ戻る。
	constexpr float LAND_SPRING = 14.0f;
	constexpr float LAND_PITCH_DEG_PER_CM = 0.2f;  // 沈んだ 1cm ごとに下を向く角度（8cm で 1.6度）。
	// 腕は、カメラより少し遅れて、さらに沈む（腕の重さで下へ振れる）。
	constexpr float ARMS_LAND_DIP_RATE = 0.6f;    // カメラの沈む量の何倍、さらに沈むか。
	constexpr float ARMS_LAND_SPRING = 9.0f;
	// これより速く動いたら、ワープ（落ちて戻ったときなど）とみなし、揺れには数えない（cm/秒）。
	constexpr float MAX_TRACK_SPEED = 5000.0f;
	// ばねを進める1回の長さの上限（秒）。重いフレームでもばねが暴れないようにする。
	constexpr float MAX_SPRING_STEP = 1.0f / 120.0f;

	float Saturate(float x)
	{
		return min(1.0f, max(0.0f, x));
	}
	float Lerp(float a, float b, float rate)
	{
		return a + (b - a) * rate;
	}
	// 沈む量 dip だけ沈むよう、ばね（速さ spring）の最初の速さを返す。
	// 減衰しきるばね x(t) = v t e^(-spring t) は、t = 1/spring で v / (spring e) まで沈む。
	float DipToSpeed(float dip, float spring)
	{
		return dip * spring * 2.718281828f;
	}
	// ばね（行き過ぎずに 0 へ戻る）を deltaTime だけ進める。
	void StepSpring(float& x, float& v, float spring, float deltaTime)
	{
		while (deltaTime > 0.0f) {
			const float dt = min(deltaTime, MAX_SPRING_STEP);
			v += (-spring * spring * x - 2.0f * spring * v) * dt;
			x += v * dt;
			deltaTime -= dt;
		}
	}
}

void FirstPersonBob::Reset(const Character& player)
{
	m_lastPosition = player.GetPosition();
	m_wasOnGround = player.IsOnGround();
	m_fallSpeed = 0.0f;
	m_phase = 0.0f;
	m_moveWeight = 0.0f;
	m_runWeight = 0.0f;
	m_landDip = 0.0f;
	m_landDipSpeed = 0.0f;
	m_armsLandDip = 0.0f;
	m_armsLandDipSpeed = 0.0f;
	UpdateOffsets();
}

void FirstPersonBob::Update(const Character& player)
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
	if (deltaTime <= 0.0f) {
		return;
	}
	// 速さは、キャラコンの速度ではなく、本当に進んだ距離から測る（壁に向かって歩いても揺れないように）。
	const Vector3 position = player.GetPosition();
	Vector3 move = position - m_lastPosition;
	m_lastPosition = position;
	const float verticalSpeed = move.y / deltaTime;
	move.y = 0.0f;
	float speed = move.Length() / deltaTime;
	if (speed > MAX_TRACK_SPEED) {
		speed = 0.0f;
	}

	const bool isOnGround = player.IsOnGround();
	if (!isOnGround) {
		m_fallSpeed = max(0.0f, -verticalSpeed);
	}
	else if (!m_wasOnGround) {
		// 着地した: 落ちていた速さに合わせて沈む。
		const float dip = min(LAND_DIP_MAX, max(0.0f, m_fallSpeed - LAND_MIN_FALL_SPEED) * LAND_DIP_PER_SPEED);
		m_landDipSpeed += DipToSpeed(dip, LAND_SPRING);
		m_armsLandDipSpeed += DipToSpeed(dip * ARMS_LAND_DIP_RATE, ARMS_LAND_SPRING);
		m_fallSpeed = 0.0f;
	}
	m_wasOnGround = isOnGround;

	// 宙にいる間は、揺れを止める（位相も進めない）。
	const float moveTarget = isOnGround ? Saturate(speed / WALK_SPEED) : 0.0f;
	m_moveWeight += (moveTarget - m_moveWeight) * min(1.0f, deltaTime * MOVE_WEIGHT_SPEED);
	if (isOnGround) {
		const float runTarget = Saturate((speed - WALK_SPEED) / (RUN_SPEED - WALK_SPEED));
		m_runWeight += (runTarget - m_runWeight) * min(1.0f, deltaTime * RUN_WEIGHT_SPEED);
		// 進んだ距離で歩みを進める。ダッシュの速さでは、足の回転を走りより速くしない。
		const float stepLength = Lerp(WALK_STEP_LENGTH, RUN_STEP_LENGTH, m_runWeight);
		m_phase += min(speed, RUN_SPEED) * deltaTime / stepLength;
		m_phase = fmodf(m_phase, 2.0f);
	}

	UpdateLanding(deltaTime);
	UpdateOffsets();
}

void FirstPersonBob::UpdateLanding(float deltaTime)
{
	StepSpring(m_landDip, m_landDipSpeed, LAND_SPRING, deltaTime);
	StepSpring(m_armsLandDip, m_armsLandDipSpeed, ARMS_LAND_SPRING, deltaTime);
}

void FirstPersonBob::UpdateOffsets()
{
	const float run = m_runWeight;
	const float w = m_moveWeight;
	// 1歩の中の進み具合（0 で足が着いた瞬間）。
	const float step = m_phase - floorf(m_phase);
	// いちばん低くなるところ: 歩きは足が着いた瞬間、走りは踏ん張る真ん中。
	const float lowest = RUN_LOWEST_STEP_RATE * run;
	// 片足で立つ真ん中: 歩きは 1歩の真ん中、走りは いちばん低いところ。左右はここで、立っている足の側へいちばん寄る。
	const float midStance = Lerp(0.5f, RUN_LOWEST_STEP_RATE, run);

	// 上下: いちばん低いところで -1、半歩あとで +1。
	const float vertical = -cosf(Math::PI2 * (step - lowest));
	// 左右: 右足で立つ真ん中（m_phase = midStance）で +1（右）、左足で立つ真ん中で -1（左）。
	const float horizontal = sinf(Math::PI * (m_phase - midStance + 0.5f));
	// 首振り: 足が着いた瞬間に 1、半歩あとで 0。
	const float nod = 0.5f + 0.5f * cosf(Math::PI2 * step);

	m_cameraOffset.x = Lerp(WALK_HORIZONTAL, RUN_HORIZONTAL, run) * w * horizontal;
	m_cameraOffset.y = Lerp(WALK_VERTICAL, RUN_VERTICAL, run) * w * vertical;
	m_cameraOffset.z = 0.0f;
	m_rollDeg = Lerp(WALK_ROLL_DEG, RUN_ROLL_DEG, run) * w * horizontal;
	m_pitchDeg = RUN_PITCH_DEG * run * w * nod * nod;

	// 腕全体: カメラより少し遅れて、8の字に動く。
	const float armsPhase = m_phase - ARMS_LAG_STEP_RATE;
	const float armsStep = armsPhase - floorf(armsPhase);
	const float armsX = Lerp(WALK_ARMS_HORIZONTAL, RUN_ARMS_HORIZONTAL, run) * w * sinf(Math::PI * (armsPhase - midStance + 0.5f));
	const float armsY = Lerp(WALK_ARMS_VERTICAL, RUN_ARMS_VERTICAL, run) * w * -cosf(Math::PI2 * (armsStep - lowest));
	// 前後: 右足が着く（m_phase = 0）とき、画面の左の腕（side 1）がいちばん前に出る。
	const float swing = Lerp(WALK_ARMS_SWING, RUN_ARMS_SWING, run) * w * cosf(Math::PI * armsPhase);
	m_armsOffset[0] = Vector3(armsX, armsY, 0.0f);
	m_armsOffset[1] = Vector3(armsX, armsY, 0.0f);
	m_armSwing[0] = -swing;
	m_armSwing[1] = swing;

	// 着地の沈み: カメラを下げて少し下を向かせる。腕は、カメラより遅れて、さらに沈む。
	m_cameraOffset.y -= m_landDip;
	m_pitchDeg += m_landDip * LAND_PITCH_DEG_PER_CM;
	m_armsOffset[0].y -= m_armsLandDip;
	m_armsOffset[1].y -= m_armsLandDip;
}
