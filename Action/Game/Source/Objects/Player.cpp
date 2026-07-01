#include "stdafx.h"
#include "Player.h"

namespace
{
	constexpr float kMoveSpeed = 300.0f;
	constexpr float kRunSpeedMultiplier = 1.6f; // 左Shift押下中の移動速度倍率。
	constexpr float kJumpSpeed = 400.0f;
	constexpr float kGravity = 980.0f;
	// スティック入力のデッドゾーン閾値（二乗値）。sqrt を避けるため平方根前の値で比較。
	constexpr float kStickDeadZoneSq = 0.01f;
	constexpr float kRushDistance = 300.0f;    // ラッシュスキルの総移動距離。
	constexpr float kRushSpeed = 1500.0f;      // ラッシュ中の移動速度（通常の5倍）。
	// ラッシュ中に停止させるランアニメーションの再生位置（秒）。疾走ポーズに合った時刻。
	constexpr float kRushRunPoseTime = 0.23f;
	constexpr float kRespawnHeight = -1000.0f; // この高さを下回るとステージ外と判断してリスポーン。
	constexpr float kModelScale = 3.0f;        // player.tkmは元のモデルより小さいため拡大する。
	const Vector3 kModelScaleVec = Vector3(kModelScale, kModelScale, kModelScale);
	const Vector3 kSwordOffset = Vector3(5.0f, -3.0f, 10.0f);   // 右手ボーンローカル空間での剣の位置オフセット。
	const Vector3 kSwordScale = Vector3(1.35f, 1.35f, 1.35f);
	constexpr float kSwordRotOffsetX = 0.0f;
	constexpr float kSwordRotOffsetY = 0.0f;
	constexpr float kSwordRotOffsetZ = -90.0f; // 剣モデルのZ軸が手の向きと90度ずれているため補正。
}

Player::Player()
{

}

Player::~Player()
{

}

bool Player::Start()
{
	m_respawnPosition = m_position;

	m_charaCon.Init(20.0f, 100.0f, m_position);
	m_isCharaConReady = true;

	// idle/walk/run/jumpのアニメーションデータが揃った。
	m_animClips[enAnimClip_Idle].Load("Assets/animData/player/idle.tka");
	m_animClips[enAnimClip_Idle].SetLoopFlag(true);
	m_animClips[enAnimClip_Walk].Load("Assets/animData/player/walk.tka");
	m_animClips[enAnimClip_Walk].SetLoopFlag(true);
	m_animClips[enAnimClip_Run].Load("Assets/animData/player/run.tka");
	m_animClips[enAnimClip_Run].SetLoopFlag(true);
	m_animClips[enAnimClip_Jump].Load("Assets/animData/player/jump.tka");
	m_animClips[enAnimClip_Jump].SetLoopFlag(false);

	// player.tkmは3ds Max(Z-up)で作成されているため、enModelUpAxisZを指定して起こす（Yのままだと仰向けに倒れた状態になる）。
	m_modelRender.Init("Assets/modelData/player/player.tkm", m_animClips, enAnimClip_Num, enModelUpAxisZ);
	m_modelRender.SetShadowCasterFlag(true);
	m_modelRender.SetPosition(m_position);
	m_modelRender.SetRotation(m_rotation);
	m_modelRender.SetScale(kModelScaleVec);
	m_modelRender.PlayAnimation(enAnimClip_Idle);
	m_modelRender.Update();

	InitSword();

	return true;
}

void Player::Update()
{
	Move();
	RespawnIfNeeded();
	UpdateAnimation();

	m_modelRender.SetPosition(m_position);
	m_modelRender.SetRotation(m_rotation);
	m_modelRender.Update();

	UpdateSword();
}

void Player::Render(RenderContext& rc)
{
	if (!m_isRenderBody) {
		return;
	}

	m_modelRender.Draw(rc);
	m_swordModelRender.Draw(rc);
}

Vector3 Player::GetForward() const
{
	Vector3 forward = Vector3::AxisZ;
	m_rotation.Apply(forward);
	return forward;
}

void Player::UpdateAnimation()
{
	const bool isRushActive = m_rushRemainingDistance > 0.0f;
	if (isRushActive) {
		if (m_currentAnimNo != enAnimClip_Run || !m_isRushPoseActive) {
			m_modelRender.PlayAnimationAtTime(enAnimClip_Run, kRushRunPoseTime);
			m_modelRender.SetAnimationSpeed(0.0f);
			m_currentAnimNo = enAnimClip_Run;
			m_isRushPoseActive = true;
		}
		return;
	}

	if (m_isRushPoseActive) {
		m_modelRender.SetAnimationSpeed(1.0f);
		m_isRushPoseActive = false;
	}

	const float stickX = g_pad[0]->GetLStickXF();
	const float stickY = g_pad[0]->GetLStickYF();
	const float stickLen = sqrtf(stickX * stickX + stickY * stickY);

	int newAnimNo;
	if (!m_charaCon.IsOnGround()) {
		newAnimNo = enAnimClip_Jump;
	}
	else if (stickLen < 0.1f) {
		newAnimNo = enAnimClip_Idle;
	}
	else if (IsRunKeyPressed()) {
		newAnimNo = enAnimClip_Run;
	}
	else {
		newAnimNo = enAnimClip_Walk;
	}

	// Avoid rewinding the clip if it is already playing
	if (newAnimNo != m_currentAnimNo) {
		m_modelRender.PlayAnimation(newAnimNo, 0.2f);
		m_currentAnimNo = newAnimNo;
	}
}

void Player::InitSword()
{
	// 右手ボーンの名前はモデルのセットアップによって異なる。
	// 1つ目で見つからなければ別名で再検索する。
	m_rightHandBoneNo = m_modelRender.FindBoneID(L"Character1_RightHand");
	if (m_rightHandBoneNo < 0) {
		m_rightHandBoneNo = m_modelRender.FindBoneID(L"J_R_ForeArm_00_tw");
	}

	m_swordModelRender.Init("Assets/modelData/weapon/sowrd/sowrd.tkm");
	m_swordModelRender.SetShadowCasterFlag(true);
	UpdateSword();
}

void Player::UpdateSword()
{
	if (m_rightHandBoneNo < 0) {
		return;
	}

	Bone* rightHandBone = m_modelRender.GetBone(m_rightHandBoneNo);
	if (rightHandBone == nullptr) {
		return;
	}

	Vector3 swordPosition;
	Vector3 handScale;
	Quaternion swordRotation;
	rightHandBone->CalcWorldTRS(swordPosition, swordRotation, handScale);

	Vector3 offset = kSwordOffset;
	swordRotation.Apply(offset);
	swordPosition += offset;

	Quaternion offsetRotationX;
	offsetRotationX.SetRotationDegX(kSwordRotOffsetX);
	Quaternion offsetRotationY;
	offsetRotationY.SetRotationDegY(kSwordRotOffsetY);
	Quaternion offsetRotationZ;
	offsetRotationZ.SetRotationDegZ(kSwordRotOffsetZ);
	swordRotation = offsetRotationX * offsetRotationY * offsetRotationZ * swordRotation;

	m_swordModelRender.SetTRS(swordPosition, swordRotation, kSwordScale);
	m_swordModelRender.Update();
}

bool Player::IsRushSkillKeyTrigger()
{
	// ラッシュスキルはキーボードLキーに割り当てているためGetAsyncKeyStateを使用。
	// 前フレームの状態と比較して「押した瞬間」だけtrueを返す。
	const bool isRushKeyPressed = GetAsyncKeyState('L') != 0;
	const bool isTrigger = isRushKeyPressed && !m_isRushKeyPressed;
	m_isRushKeyPressed = isRushKeyPressed;
	return isTrigger;
}

bool Player::IsRunKeyPressed() const
{
	// 左Shiftを押している間だけ走る。
	return GetAsyncKeyState(VK_LSHIFT) != 0;
}

void Player::StartRushSkill(const Vector3& direction)
{
	m_rushDirection = direction;
	m_rushDirection.y = 0.0f;
	// スティック入力がデッドゾーン以下なら、プレイヤーが向いている方向へラッシュする。
	if (m_rushDirection.LengthSq() <= kStickDeadZoneSq) {
		m_rushDirection = GetForward();
		m_rushDirection.y = 0.0f;
	}
	if (m_rushDirection.LengthSq() > FLT_EPSILON) {
		m_rushDirection.Normalize();
		m_rushRemainingDistance = kRushDistance;
	}
}

void Player::RespawnIfNeeded()
{
	if (m_position.y < kRespawnHeight) {
		Respawn();
	}
}

void Player::Respawn()
{
	m_position = m_respawnPosition;
	m_moveSpeed = Vector3::Zero;
	m_rushRemainingDistance = 0.0f;
	m_isRushPoseActive = false;
	// m_modelRender.SetAnimationSpeed(1.0f);

	if (m_isCharaConReady) {
		m_charaCon.SetPosition(m_position);
	}
}

void Player::Move()
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
	const float stickX = g_pad[0]->GetLStickXF();
	const float stickY = g_pad[0]->GetLStickYF();

	Vector3 cameraForward = g_camera3D->GetForward();
	Vector3 cameraRight = g_camera3D->GetRight();

	cameraForward.y = 0.0f;
	cameraRight.y = 0.0f;
	cameraForward.Normalize();
	cameraRight.Normalize();

	// 左Shift押下中は移動速度も上げる。
	const float currentMoveSpeed = IsRunKeyPressed() ? kMoveSpeed * kRunSpeedMultiplier : kMoveSpeed;

	Vector3 horizontalMoveSpeed = Vector3::Zero;
	horizontalMoveSpeed.x += cameraForward.x * stickY * currentMoveSpeed;
	horizontalMoveSpeed.z += cameraForward.z * stickY * currentMoveSpeed;
	horizontalMoveSpeed.x += cameraRight.x * stickX * currentMoveSpeed;
	horizontalMoveSpeed.z += cameraRight.z * stickX * currentMoveSpeed;

	if (IsRushSkillKeyTrigger() && m_rushRemainingDistance <= 0.0f) {
		StartRushSkill(horizontalMoveSpeed);
	}

	if (m_rushRemainingDistance > 0.0f && deltaTime > FLT_EPSILON) {
		// 今フレームの移動距離が残り距離を超えないよう、小さい方の値を使う。
		const float maxRushMoveDistance = kRushSpeed * deltaTime;
		const float rushMoveDistance = maxRushMoveDistance < m_rushRemainingDistance ? maxRushMoveDistance : m_rushRemainingDistance;
		horizontalMoveSpeed = m_rushDirection * (rushMoveDistance / deltaTime);
		m_rushRemainingDistance -= rushMoveDistance;
		if (m_rushRemainingDistance < 0.0f) {
			m_rushRemainingDistance = 0.0f;
		}
	}

	m_moveSpeed.x = horizontalMoveSpeed.x;
	m_moveSpeed.z = horizontalMoveSpeed.z;

	if (g_pad[0]->IsTrigger(enButtonA) && m_charaCon.IsOnGround()) {
		m_moveSpeed.y = kJumpSpeed;
	}

	m_moveSpeed.y -= kGravity * deltaTime;

	if (horizontalMoveSpeed.LengthSq() > kStickDeadZoneSq) {
		m_rotation.SetRotationYFromDirectionXZ(horizontalMoveSpeed);
	}

	m_position = m_charaCon.Execute(m_moveSpeed, deltaTime);

	if (m_charaCon.IsOnGround()) {
		m_moveSpeed.y = 0.0f;
	}
}
