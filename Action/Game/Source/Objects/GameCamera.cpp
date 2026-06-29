#include "stdafx.h"
#include "GameCamera.h"

#include "Player.h"
#include "system.h"

namespace
{
	constexpr float kCameraRotateSpeed = 2.0f;
	constexpr float kMouseCameraRotateSpeed = 0.12f;
	constexpr float kMaxMouseCameraRotateDeg = 12.0f;
	constexpr float kTargetHeight = 80.0f;
	constexpr float kTargetForwardOffset = 20.0f;
	constexpr float kFirstPersonEyeHeight = 90.0f;
	constexpr float kFirstPersonLookDistance = 1000.0f;
	constexpr float kFirstPersonNear = 5.0f;
	const Vector3 kFirstPersonGunOffset = Vector3(52.0f, -26.0f, 42.0f);
	const Vector3 kFirstPersonGunScale = Vector3(2.35f, 2.35f, 2.35f);
	constexpr float kFirstPersonGunRotOffsetX = 0.0f;
	constexpr float kFirstPersonGunRotOffsetY = -7.0f;
	constexpr float kFirstPersonGunRotOffsetZ = 0.0f;

	float ClampMouseRotateDeg(float rotateDeg)
	{
		if (rotateDeg > kMaxMouseCameraRotateDeg) {
			return kMaxMouseCameraRotateDeg;
		}
		if (rotateDeg < -kMaxMouseCameraRotateDeg) {
			return -kMaxMouseCameraRotateDeg;
		}
		return rotateDeg;
	}
}

GameCamera::GameCamera()
{

}

GameCamera::~GameCamera()
{
	if (m_player != nullptr) {
		m_player->SetRenderBody(true);
	}
	EndMouseLook();
}

bool GameCamera::Start()
{
	m_player = FindGO<Player>("player");

	if (m_player == nullptr) {
		return false;
	}

	m_defaultNear = g_camera3D->GetNear();
	m_firstPersonForward = m_player->GetForward();

	InitFirstPersonGun();

	return true;
}

void GameCamera::Update()
{
	if (m_player == nullptr) {
		EndMouseLook();
		return;
	}

	if (g_pad[0]->IsTrigger(enButtonB)) {
		ToggleCameraMode();
	}

	if (m_cameraMode == EnCameraMode::FirstPerson) {
		UpdateFirstPersonCamera();
	}
	else {
		UpdateThirdPersonCamera();
	}
}

void GameCamera::Render(RenderContext& rc)
{
	if (m_cameraMode != EnCameraMode::FirstPerson) {
		return;
	}

	m_firstPersonGunRender.Draw(rc);
}

void GameCamera::ToggleCameraMode()
{
	if (m_cameraMode == EnCameraMode::ThirdPerson) {
		m_cameraMode = EnCameraMode::FirstPerson;
		m_player->SetRenderBody(false);
		m_firstPersonForward = g_camera3D->GetForward();
		m_firstPersonForward.y = 0.0f;
		if (m_firstPersonForward.LengthSq() <= FLT_EPSILON) {
			m_firstPersonForward = m_player->GetForward();
		}
		m_firstPersonForward.Normalize();
		g_camera3D->SetNear(kFirstPersonNear);
	}
	else {
		m_cameraMode = EnCameraMode::ThirdPerson;
		m_player->SetRenderBody(true);
		g_camera3D->SetNear(m_defaultNear);
	}
}

void GameCamera::UpdateThirdPersonCamera()
{
	Vector3 target = m_player->GetPosition();
	target.y += kTargetHeight;
	target += g_camera3D->GetForward() * kTargetForwardOffset;

	const Vector3 oldToCameraPos = m_toCameraPos;

	float yawDeg = 0.0f;
	float pitchDeg = 0.0f;
	GetCameraRotationInput(yawDeg, pitchDeg);

	Quaternion rotation;
	rotation.SetRotationDeg(Vector3::AxisY, yawDeg);
	rotation.Apply(m_toCameraPos);

	Vector3 axisX;
	axisX.Cross(Vector3::AxisY, m_toCameraPos);
	if (axisX.LengthSq() > FLT_EPSILON) {
		axisX.Normalize();
		rotation.SetRotationDeg(axisX, pitchDeg);
		rotation.Apply(m_toCameraPos);
	}

	Vector3 toCameraDir = m_toCameraPos;
	toCameraDir.Normalize();
	if (toCameraDir.y < -0.5f || toCameraDir.y > 0.8f) {
		m_toCameraPos = oldToCameraPos;
	}

	const Vector3 position = target + m_toCameraPos;
	g_camera3D->SetPosition(position);
	g_camera3D->SetTarget(target);
	g_camera3D->Update();
}

void GameCamera::UpdateFirstPersonCamera()
{
	RotateFirstPersonForward();

	Vector3 eyePosition = m_player->GetPosition();
	eyePosition.y += kFirstPersonEyeHeight;

	const Vector3 target = eyePosition + m_firstPersonForward * kFirstPersonLookDistance;
	g_camera3D->SetPosition(eyePosition);
	g_camera3D->SetTarget(target);
	g_camera3D->Update();

	UpdateFirstPersonGun();
}

void GameCamera::InitFirstPersonGun()
{
	m_firstPersonGunRender.SetRaytracingWorld(false);
	m_firstPersonGunRender.Init("Assets/modelData/weapon/gun/gun.tkm");
	m_firstPersonGunRender.SetShadowCasterFlag(false);
	UpdateFirstPersonGun();
}

void GameCamera::UpdateFirstPersonGun()
{
	const Vector3& cameraPosition = g_camera3D->GetPosition();
	const Vector3& cameraForward = g_camera3D->GetForward();
	const Vector3& cameraRight = g_camera3D->GetRight();
	const Matrix& cameraRotationMatrix = g_camera3D->GetCameraRotation();
	const Vector3 cameraUp(cameraRotationMatrix.m[1][0], cameraRotationMatrix.m[1][1], cameraRotationMatrix.m[1][2]);
	Vector3 horizontalForward = cameraForward;
	horizontalForward.y = 0.0f;
	if (horizontalForward.LengthSq() <= FLT_EPSILON) {
		horizontalForward = Vector3::AxisZ;
	}
	horizontalForward.Normalize();

	Vector3 gunPosition = cameraPosition;
	gunPosition += cameraRight * kFirstPersonGunOffset.x;
	gunPosition += cameraUp * kFirstPersonGunOffset.y;
	gunPosition += cameraForward * kFirstPersonGunOffset.z;
	const float baseGunLocalY = kFirstPersonGunOffset.y;
	const float currentGunLocalY = gunPosition.y - cameraPosition.y;
	gunPosition.y = cameraPosition.y + baseGunLocalY - (currentGunLocalY - baseGunLocalY);

	Quaternion gunRotation;
	gunRotation.SetRotationYFromDirectionXZ(horizontalForward);

	Quaternion offsetRotationX;
	offsetRotationX.SetRotationDegX(kFirstPersonGunRotOffsetX);
	Quaternion offsetRotationY;
	offsetRotationY.SetRotationDegY(kFirstPersonGunRotOffsetY);
	Quaternion offsetRotationZ;
	offsetRotationZ.SetRotationDegZ(kFirstPersonGunRotOffsetZ);
	gunRotation = offsetRotationX * offsetRotationY * offsetRotationZ * gunRotation;

	m_firstPersonGunRender.SetTRS(gunPosition, gunRotation, kFirstPersonGunScale);
	m_firstPersonGunRender.Update();
}

void GameCamera::RotateFirstPersonForward()
{
	const Vector3 oldForward = m_firstPersonForward;

	float yawDeg = 0.0f;
	float pitchDeg = 0.0f;
	GetCameraRotationInput(yawDeg, pitchDeg);

	Quaternion rotation;
	rotation.SetRotationDeg(Vector3::AxisY, yawDeg);
	rotation.Apply(m_firstPersonForward);

	Vector3 axisX;
	axisX.Cross(Vector3::AxisY, m_firstPersonForward);
	if (axisX.LengthSq() > FLT_EPSILON) {
		axisX.Normalize();
		rotation.SetRotationDeg(axisX, pitchDeg);
		rotation.Apply(m_firstPersonForward);
	}

	m_firstPersonForward.Normalize();
	if (m_firstPersonForward.y < -0.6f || m_firstPersonForward.y > 0.8f) {
		m_firstPersonForward = oldForward;
	}
}

void GameCamera::GetCameraRotationInput(float& yawDeg, float& pitchDeg)
{
	yawDeg = kCameraRotateSpeed * g_pad[0]->GetRStickXF();
	pitchDeg = kCameraRotateSpeed * g_pad[0]->GetRStickYF();

	float mouseYawDeg = 0.0f;
	float mousePitchDeg = 0.0f;
	if (TryGetMouseRotationInput(mouseYawDeg, mousePitchDeg)) {
		yawDeg += mouseYawDeg;
		pitchDeg += mousePitchDeg;
	}
}

bool GameCamera::TryGetMouseRotationInput(float& yawDeg, float& pitchDeg)
{
	yawDeg = 0.0f;
	pitchDeg = 0.0f;

	if (g_hWnd == nullptr || GetForegroundWindow() != g_hWnd) {
		EndMouseLook();
		return false;
	}

	RECT clientRect;
	if (!GetClientRect(g_hWnd, &clientRect)) {
		EndMouseLook();
		return false;
	}

	POINT cursorScreenPos;
	if (!GetCursorPos(&cursorScreenPos)) {
		EndMouseLook();
		return false;
	}

	POINT cursorClientPos = cursorScreenPos;
	ScreenToClient(g_hWnd, &cursorClientPos);
	if (!m_isMouseLookActive && !PtInRect(&clientRect, cursorClientPos)) {
		return false;
	}

	POINT centerClientPos = {
		(clientRect.right - clientRect.left) / 2,
		(clientRect.bottom - clientRect.top) / 2,
	};
	POINT centerScreenPos = centerClientPos;
	ClientToScreen(g_hWnd, &centerScreenPos);

	if (!m_isMouseLookActive) {
		m_isMouseLookActive = true;
		ShowCursor(FALSE);
		SetCursorPos(centerScreenPos.x, centerScreenPos.y);
		return false;
	}

	const float mouseDeltaX = static_cast<float>(cursorScreenPos.x - centerScreenPos.x);
	const float mouseDeltaY = static_cast<float>(cursorScreenPos.y - centerScreenPos.y);

	SetCursorPos(centerScreenPos.x, centerScreenPos.y);

	yawDeg = ClampMouseRotateDeg(mouseDeltaX * kMouseCameraRotateSpeed);
	pitchDeg = ClampMouseRotateDeg(mouseDeltaY * kMouseCameraRotateSpeed);
	return mouseDeltaX != 0.0f || mouseDeltaY != 0.0f;
}

void GameCamera::EndMouseLook()
{
	if (!m_isMouseLookActive) {
		return;
	}

	m_isMouseLookActive = false;
	ShowCursor(TRUE);
}
