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

	Move();
	UpdateAnimation();

	m_modelRender.SetTRS(m_position, m_rotation, m_scale);
	m_modelRender.Update();
}

void Enemy::Render(RenderContext& rc)
{
	m_modelRender.Draw(rc);
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
