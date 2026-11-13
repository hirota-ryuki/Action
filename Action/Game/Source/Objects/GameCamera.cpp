#include "stdafx.h"
#include "GameCamera.h"

#include "Player.h"
#include "system.h"

namespace
{
	// 右スティックを倒し切ったときの回転の速さ（度/秒）。経過時間を掛けて、FPS によらず同じ速さにする。
	constexpr float CAMERA_ROTATE_SPEED_DEG_PER_SEC = 120.0f;
	// マウスの感度。大きいほど少ない移動量で大きく回転する。
	constexpr float MOUSE_CAMERA_ROTATE_SPEED = 0.12f;
	// 1フレームあたりのマウス回転量の上限。フレームレートの揺れによるカクつきを抑える。
	constexpr float MAX_MOUSE_CAMERA_ROTATE_DEG = 12.0f;
	// カメラをプレイヤーに近づける代わりに画角を広げてプレイヤー全体が収まるようにしてある。
	constexpr float TARGET_HEIGHT = 170.0f;          // 注視点をプレイヤーの足元でなく腰あたりに合わせる高さ。
	constexpr float TARGET_FORWARD_OFFSET = 20.0f;    // 注視点をカメラ正面方向に少しずらして奥行き感を出す。
	constexpr float FIRST_PERSON_EYE_HEIGHT = 90.0f;   // 一人称カメラの目線の高さ（プレイヤー足元からのオフセット）。
	constexpr float VIEW_ANGLE_DEG = 90.0f;           // カメラを近づけた分、画角を通常の60度から広げる。
	constexpr float FIRST_PERSON_LOOK_DISTANCE = 1000.0f;
	constexpr float FIRST_PERSON_NEAR = 5.0f;         // 一人称時はNearを小さくして手元の腕と手が見切れないようにする。
	constexpr int TOGGLE_CAMERA_KEY = 'V';            // 三人称／一人称を切り替えるキー（パッドは B ボタン）。

	// マウス1フレームあたりの回転量を上限値で制限する。
	float ClampMouseRotateDeg(float rotateDeg)
	{
		if (rotateDeg > MAX_MOUSE_CAMERA_ROTATE_DEG) {
			return MAX_MOUSE_CAMERA_ROTATE_DEG;
		}
		if (rotateDeg < -MAX_MOUSE_CAMERA_ROTATE_DEG) {
			return -MAX_MOUSE_CAMERA_ROTATE_DEG;
		}
		return rotateDeg;
	}
}

GameCamera::GameCamera()
{

}

GameCamera::~GameCamera()
{
	RestorePlayerRenderBody();
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

	g_camera3D->SetViewAngle(Math::DegToRad(VIEW_ANGLE_DEG));

	m_firstPersonArms.Init();

	return true;
}

void GameCamera::Update()
{
	m_player = FindGO<Player>("player");
	if (m_player == nullptr || m_player->IsDead()) {
		EndMouseLook();
		return;
	}

	if (getenv("FP_TEST") != nullptr && m_cameraMode == EnCameraMode::ThirdPerson) { ToggleCameraMode(); } // FP_TEST_TEMP
	if (IsToggleTriggered()) {
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

	m_firstPersonArms.Draw(rc);
}

bool GameCamera::IsToggleTriggered()
{
	// ほかのウィンドウを使っている間のキーは拾わない。
	const bool isWindowActive = g_hWnd != nullptr && GetForegroundWindow() == g_hWnd;
	m_toggleKey.Update(isWindowActive && (GetAsyncKeyState(TOGGLE_CAMERA_KEY) & 0x8000) != 0);
	return m_toggleKey.isTrigger || g_pad[0]->IsTrigger(enButtonB);
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
		g_camera3D->SetNear(FIRST_PERSON_NEAR);
		m_firstPersonArms.Reset(*m_player);
	}
	else {
		m_cameraMode = EnCameraMode::ThirdPerson;
		RestorePlayerRenderBody();
		g_camera3D->SetNear(m_defaultNear);
	}
}

void GameCamera::UpdateThirdPersonCamera()
{
	Vector3 target = m_player->GetPosition();
	target.y += TARGET_HEIGHT;
	target += g_camera3D->GetForward() * TARGET_FORWARD_OFFSET;

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

	// カメラが真下や真上を向きすぎるとジンバルロックが起きるため、仰角・俯角を制限する。
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
	eyePosition.y += FIRST_PERSON_EYE_HEIGHT;

	const Vector3 target = eyePosition + m_firstPersonForward * FIRST_PERSON_LOOK_DISTANCE;
	g_camera3D->SetPosition(eyePosition);
	g_camera3D->SetTarget(target);
	g_camera3D->Update();

	// 腕はカメラに付いていくので、カメラを動かしたあとで動かす。
	m_firstPersonArms.Update(*m_player);
}

void GameCamera::RotateFirstPersonForward()
{
	const Vector3 oldForward = m_firstPersonForward;

	float yawDeg = 0.0f;
	float pitchDeg = 0.0f;
	GetCameraRotationInput(yawDeg, pitchDeg);
	pitchDeg = -pitchDeg;  // 一人称の縦を反転。

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
	const float rotateDeg = CAMERA_ROTATE_SPEED_DEG_PER_SEC * g_gameTime->GetFrameDeltaTime();
	yawDeg = rotateDeg * g_pad[0]->GetRStickXF();
	pitchDeg = rotateDeg * g_pad[0]->GetRStickYF();

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

	// ウィンドウが非アクティブなら強制的にマウスルックを終了する。
	if (g_hWnd == nullptr || GetForegroundWindow() != g_hWnd) {
		EndMouseLook();
		return false;
	}

	// 開発用の画面（ImGui）を出している間は、マウスを ImGui に使うので、カメラは回さない。
	if (DebugUI::IsVisible()) {
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

	// マウスルック未開始の場合、カーソルがウィンドウ外ならスルーする。
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

	// 初回のみカーソルを画面中央に戻してから計算開始。前フレームとのずれが出ないようにするため。
	if (!m_isMouseLookActive) {
		m_isMouseLookActive = true;
		ShowCursor(FALSE);
		SetCursorPos(centerScreenPos.x, centerScreenPos.y);
		return false;
	}

	const float mouseDeltaX = static_cast<float>(cursorScreenPos.x - centerScreenPos.x);
	const float mouseDeltaY = static_cast<float>(cursorScreenPos.y - centerScreenPos.y);

	// 毎フレーム中央にリセットして次フレームの移動量をデルタとして使う（無限スクロール方式）。
	SetCursorPos(centerScreenPos.x, centerScreenPos.y);

	yawDeg = ClampMouseRotateDeg(mouseDeltaX * MOUSE_CAMERA_ROTATE_SPEED);
	// マウスは上方向がマイナスなので符号を反転してピッチに変換する。
	pitchDeg = ClampMouseRotateDeg(-mouseDeltaY * MOUSE_CAMERA_ROTATE_SPEED);
	return mouseDeltaX != 0.0f || mouseDeltaY != 0.0f;
}

void GameCamera::RestorePlayerRenderBody()
{
	if (GameObjectManager::GetInstance() == nullptr) {
		return;
	}

	Player* player = FindGO<Player>("player");
	if (player == nullptr || player->IsDead()) {
		return;
	}

	player->SetRenderBody(true);
}

void GameCamera::EndMouseLook()
{
	if (!m_isMouseLookActive) {
		return;
	}

	m_isMouseLookActive = false;
	ShowCursor(TRUE);
}
