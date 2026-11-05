#include "stdafx.h"
#include "Enemy.h"

#include "Player.h"

namespace
{
	constexpr float kMoveSpeed = 140.0f;
	constexpr float kGravity = 980.0f;
	constexpr float kChaseRange = 900.0f;      // この距離以内に入ると追跡を開始する。
	constexpr float kStopDistance = 90.0f;     // この距離まで近づいたら停止する（近づきすぎ防止）。
	// 距離の比較は sqrt を避けるため二乗値で行う。
	constexpr float kChaseRangeSq = kChaseRange * kChaseRange;
	constexpr float kStopDistanceSq = kStopDistance * kStopDistance;
	// 凍結中の色。乗算で赤を落として青を上げ、さらに水色を足して暗い部分（髪・服）も水色にする。
	const Vector4 kFrozenMulColor = Vector4(0.30f, 0.78f, 1.45f, 1.0f);
	const Vector4 kFrozenAddColor = Vector4(0.06f, 0.22f, 0.34f, 0.0f);
	const Vector4 kNormalMulColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	const Vector4 kNormalAddColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

	// a(t=0) から b(t=1) へ線形補間する。
	Vector4 LerpColor(float t, const Vector4& a, const Vector4& b)
	{
		return Vector4(
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
			a.w + (b.w - a.w) * t);
	}
	constexpr float kUnfreezeFadeSeconds = 0.4f;  // 凍結が解ける直前、この秒数をかけて元の色へ戻す。
}

Enemy::Enemy()
{

}

Enemy::~Enemy()
{

}

bool Enemy::Start()
{
	m_player = FindGO<Player>("player");
	if (m_player == nullptr) {
		return false;
	}

	m_charaCon.Init(20.0f, 100.0f, m_position);
	m_isCharaConReady = true;

	m_animClips[enAnimClip_Idle].Load("Assets/animData/idle.tka");
	m_animClips[enAnimClip_Idle].SetLoopFlag(true);
	m_animClips[enAnimClip_Run].Load("Assets/animData/run.tka");
	m_animClips[enAnimClip_Run].SetLoopFlag(true);

	m_modelRender.Init("Assets/modelData/unityChan.tkm", m_animClips, enAnimClip_Num, enModelUpAxisY);
	m_modelRender.SetShadowCasterFlag(true);
	m_modelRender.SetTRS(m_position, m_rotation, m_scale);
	m_modelRender.PlayAnimation(enAnimClip_Idle);
	m_modelRender.Update();

	return true;
}

void Enemy::Update()
{
	if (m_player == nullptr) {
		m_player = FindGO<Player>("player");
		if (m_player == nullptr) {
			return;
		}
	}

	UpdateFreeze();
	if (!IsFrozen()) {
		Move();
		UpdateAnimation();
	}

	m_modelRender.SetTRS(m_position, m_rotation, m_scale);
	m_modelRender.Update();
}

void Enemy::Render(RenderContext& rc)
{
	m_modelRender.Draw(rc);
}

void Enemy::Freeze(float seconds)
{
	if (seconds <= m_freezeTimer) {
		return;
	}
	if (!IsFrozen()) {
		// 凍った瞬間のポーズで固める。
		m_modelRender.SetAnimationSpeed(0.0f);
	}
	m_modelRender.SetMulColor(kFrozenMulColor);
	m_modelRender.SetAddColor(kFrozenAddColor);
	m_freezeTimer = seconds;
	// 動いていた勢いは凍結で打ち消す（解けた瞬間に滑り出さないように）。
	m_moveSpeed = Vector3::Zero;
}

void Enemy::UpdateFreeze()
{
	if (!IsFrozen()) {
		return;
	}
	m_freezeTimer -= g_gameTime->GetFrameDeltaTime();
	if (m_freezeTimer <= 0.0f) {
		m_freezeTimer = 0.0f;
		m_modelRender.SetAnimationSpeed(1.0f);
		m_modelRender.SetMulColor(kNormalMulColor);
		m_modelRender.SetAddColor(kNormalAddColor);
		return;
	}
	// 解ける直前は水色から元の色へなめらかに戻す。
	if (m_freezeTimer < kUnfreezeFadeSeconds) {
		const float t = m_freezeTimer / kUnfreezeFadeSeconds;  // 1→0
		m_modelRender.SetMulColor(LerpColor(t, kNormalMulColor, kFrozenMulColor));
		m_modelRender.SetAddColor(LerpColor(t, kNormalAddColor, kFrozenAddColor));
	}
}

void Enemy::Move()
{
	Vector3 toPlayer;
	toPlayer.Subtract(m_player->GetPosition(), m_position);
	toPlayer.y = 0.0f;  // 高低差を無視して水平方向の追跡のみ行う。

	const float distanceSq = toPlayer.LengthSq();
	Vector3 horizontalMoveSpeed = Vector3::Zero;
	// 追跡範囲内かつ停止距離の外にいるときだけ追跡する。
	m_isChasing = distanceSq <= kChaseRangeSq && distanceSq > kStopDistanceSq;

	if (m_isChasing) {
		toPlayer.Normalize();
		horizontalMoveSpeed = toPlayer * kMoveSpeed;
		m_rotation.SetRotationYFromDirectionXZ(horizontalMoveSpeed);
	}

	m_moveSpeed.x = horizontalMoveSpeed.x;
	m_moveSpeed.z = horizontalMoveSpeed.z;
	m_moveSpeed.y -= kGravity * g_gameTime->GetFrameDeltaTime();

	m_position = m_charaCon.Execute(m_moveSpeed, g_gameTime->GetFrameDeltaTime());

	if (m_charaCon.IsOnGround()) {
		m_moveSpeed.y = 0.0f;
	}
}

void Enemy::UpdateAnimation()
{
	const int newAnimNo = m_isChasing ? enAnimClip_Run : enAnimClip_Idle;

	if (newAnimNo != m_currentAnimNo) {
		m_modelRender.PlayAnimation(newAnimNo, 0.2f);
		m_currentAnimNo = newAnimNo;
	}
}
