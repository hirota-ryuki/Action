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
	constexpr float TARGET_FORWARD_OFFSET = 20.0f;    // 注視点をカメラ正面方向に少しずらして奥行き感を出す。
	// 三人称の寄り（原神くらい）。ほぼ水平の高さから、狭めの画角で見る。キャラクターが画面の下寄りに映り、奥の景色が広く見える。
	constexpr float CLOSE_DISTANCE = 350.0f;
	constexpr float CLOSE_TARGET_HEIGHT = 150.0f;     // 注視点を胸から頭のあたりに合わせる。
	constexpr float CLOSE_PITCH_DEG = 10.0f;
	constexpr float CLOSE_VIEW_ANGLE_DEG = 55.0f;
	// 三人称の俯瞰。前からある三人称の視点（オフセット (0, 250, -250)、画角 90 度）と同じ。
	// カメラをプレイヤーに近づける代わりに画角を広げてプレイヤー全体が収まるようにしてある。
	constexpr float OVERHEAD_DISTANCE = 353.6f;       // (0, 250, -250) の長さ。
	constexpr float OVERHEAD_TARGET_HEIGHT = 170.0f;  // 注視点をプレイヤーの足元でなく腰あたりに合わせる高さ。
	constexpr float OVERHEAD_PITCH_DEG = 45.0f;
	constexpr float OVERHEAD_VIEW_ANGLE_DEG = 90.0f;  // カメラを近づけた分、画角を通常の60度から広げる。
	// 寄りと俯瞰を切り替えたときに、前のカメラからなめらかに移す時間（秒）。一瞬で飛ぶと酔いやすい。
	constexpr float MODE_BLEND_SECONDS = 0.25f;
	// 一人称カメラの目線の高さ（プレイヤー足元からのオフセット）。体のモデルの目の高さ（約 150 × 大きさ 1.128）に合わせる。
	constexpr float FIRST_PERSON_EYE_HEIGHT = 168.0f;
	constexpr float FIRST_PERSON_LOOK_DISTANCE = 1000.0f;
	constexpr float FIRST_PERSON_NEAR = 5.0f;         // 一人称時はNearを小さくして手元の腕と手が見切れないようにする。
	// 一人称の画角（縦、度）。三人称の 90 度のままだと広角すぎて、手が小さく画面の真ん中に寄って見える。
	constexpr float FIRST_PERSON_VIEW_ANGLE_DEG = 50.0f;
	constexpr int TOGGLE_CAMERA_KEY = 'V';            // 三人称／一人称を切り替えるキー（パッドは B ボタン）。
	constexpr int TOGGLE_OVERHEAD_KEY = 'B';          // 俯瞰と寄りを切り替えるキー。

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

	// 最初は寄りで、プレイヤーの後ろから見る。
	ResetThirdPersonOffset(m_player->GetForward());
	g_camera3D->SetViewAngle(Math::DegToRad(GetThirdPersonSetting().viewAngleDeg));

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

	if (IsToggleTriggered()) {
		ToggleCameraMode();
	}
	if (IsOverheadToggleTriggered()) {
		ToggleOverheadMode();
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

bool GameCamera::IsOverheadToggleTriggered()
{
	// ほかのウィンドウを使っている間のキーは拾わない。
	const bool isWindowActive = g_hWnd != nullptr && GetForegroundWindow() == g_hWnd;
	m_overheadKey.Update(isWindowActive && (GetAsyncKeyState(TOGGLE_OVERHEAD_KEY) & 0x8000) != 0);
	return m_overheadKey.isTrigger;
}

void GameCamera::ToggleCameraMode()
{
	if (m_cameraMode != EnCameraMode::FirstPerson) {
		// 一人称から V で戻るときのために、今の三人称のモード（寄りか俯瞰）を覚えておく。
		m_lastThirdPersonMode = m_cameraMode;
		m_cameraMode = EnCameraMode::FirstPerson;
		m_player->SetRenderBody(false);
		m_firstPersonForward = g_camera3D->GetForward();
		m_firstPersonForward.y = 0.0f;
		if (m_firstPersonForward.LengthSq() <= FLT_EPSILON) {
			m_firstPersonForward = m_player->GetForward();
		}
		m_firstPersonForward.Normalize();
		g_camera3D->SetNear(FIRST_PERSON_NEAR);
		g_camera3D->SetViewAngle(Math::DegToRad(FIRST_PERSON_VIEW_ANGLE_DEG));
		m_firstPersonBob.Reset(*m_player);
		m_firstPersonArms.Reset(*m_player);
	}
	else {
		// 一人称に入る前に使っていた三人称に戻る。体を映す・上の向きと Near を戻すのも、この中でする。
		ChangeThirdPersonMode(m_lastThirdPersonMode);
	}
}

void GameCamera::ToggleOverheadMode()
{
	if (m_cameraMode == EnCameraMode::ThirdPersonOverhead) {
		ChangeThirdPersonMode(EnCameraMode::ThirdPersonClose);
	}
	else {
		ChangeThirdPersonMode(EnCameraMode::ThirdPersonOverhead);
	}
}

void GameCamera::ChangeThirdPersonMode(EnCameraMode mode)
{
	const bool isFromFirstPerson = m_cameraMode == EnCameraMode::FirstPerson;
	if (isFromFirstPerson) {
		RestorePlayerRenderBody();
		// 一人称の揺れで傾けた上の向きを戻す。
		g_camera3D->SetUp(Vector3::AxisY);
		g_camera3D->SetNear(m_defaultNear);
	}

	// 寄りと俯瞰の間は、今のカメラからなめらかに移す。
	// 一人称からは移さない（体が急に映るので、移す途中を見せても不自然になる）。
	m_blendTimer = isFromFirstPerson ? 0.0f : MODE_BLEND_SECONDS;
	m_blendFromPosition = g_camera3D->GetPosition();
	m_blendFromTarget = g_camera3D->GetTarget();
	m_blendFromViewAngle = g_camera3D->GetViewAngle();

	m_cameraMode = mode;
	m_lastThirdPersonMode = mode;
	// 今カメラが向いている左右の向きは引き継ぎ、見下ろす角度と距離はモードの値にする（画角は UpdateThirdPersonCamera で渡す）。
	ResetThirdPersonOffset(g_camera3D->GetForward());
}

const GameCamera::ThirdPersonSetting& GameCamera::GetThirdPersonSetting() const
{
	static const ThirdPersonSetting CLOSE_SETTING = { CLOSE_DISTANCE, CLOSE_TARGET_HEIGHT, CLOSE_PITCH_DEG, CLOSE_VIEW_ANGLE_DEG };
	static const ThirdPersonSetting OVERHEAD_SETTING = { OVERHEAD_DISTANCE, OVERHEAD_TARGET_HEIGHT, OVERHEAD_PITCH_DEG, OVERHEAD_VIEW_ANGLE_DEG };
	if (m_cameraMode == EnCameraMode::ThirdPersonOverhead) {
		return OVERHEAD_SETTING;
	}
	return CLOSE_SETTING;
}

void GameCamera::ResetThirdPersonOffset(const Vector3& forward)
{
	Vector3 horizontalForward = forward;
	horizontalForward.y = 0.0f;
	if (horizontalForward.LengthSq() <= FLT_EPSILON) {
		horizontalForward = m_player->GetForward();
		horizontalForward.y = 0.0f;
	}
	horizontalForward.Normalize();

	// forward の後ろへ、見下ろす角度の分だけ上げた位置に置く。
	const ThirdPersonSetting& setting = GetThirdPersonSetting();
	const float pitchRad = Math::DegToRad(setting.pitchDeg);
	m_toCameraPos = horizontalForward * (-cosf(pitchRad) * setting.distance);
	m_toCameraPos.y = sinf(pitchRad) * setting.distance;
}

void GameCamera::UpdateThirdPersonCamera()
{
	Vector3 target = m_player->GetPosition();
	target.y += GetThirdPersonSetting().targetHeight;
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

	Vector3 position = target + m_toCameraPos;
	float viewAngle = Math::DegToRad(GetThirdPersonSetting().viewAngleDeg);

	// 寄りと俯瞰を切り替えた直後は、前のカメラから今のモードのカメラへ、なめらかに移す。
	if (m_blendTimer > 0.0f) {
		m_blendTimer -= g_gameTime->GetFrameDeltaTime();
		if (m_blendTimer < 0.0f) {
			m_blendTimer = 0.0f;
		}
		// 0 から 1 へ進む割合。始めと終わりをゆっくりにする（smoothstep）。
		const float rate = 1.0f - m_blendTimer / MODE_BLEND_SECONDS;
		const float smoothRate = rate * rate * (3.0f - 2.0f * rate);
		position.Lerp(smoothRate, m_blendFromPosition, position);
		target.Lerp(smoothRate, m_blendFromTarget, target);
		viewAngle = m_blendFromViewAngle + (viewAngle - m_blendFromViewAngle) * smoothRate;
	}

	g_camera3D->SetPosition(position);
	g_camera3D->SetTarget(target);
	g_camera3D->SetViewAngle(viewAngle);
	g_camera3D->Update();
}

void GameCamera::UpdateFirstPersonCamera()
{
	RotateFirstPersonForward();

	Vector3 eyePosition = m_player->GetPosition();
	eyePosition.y += FIRST_PERSON_EYE_HEIGHT;

	// 歩き・走りの揺れと着地の沈みを、見た目にだけ足す（m_firstPersonForward は変えない）。
	m_firstPersonBob.Update(*m_player);
	// マウスで向けた向きに直交する、上と右の向き。
	Vector3 up = Vector3::AxisY - m_firstPersonForward * m_firstPersonForward.y;
	up.Normalize();
	Vector3 right;
	right.Cross(up, m_firstPersonForward);
	// 位置は、右と上へずらす。向きは、首振りの分だけ下へ、傾きの分だけ右へ回す。
	const Vector3& offset = m_firstPersonBob.GetCameraOffset();
	eyePosition += right * offset.x + up * offset.y;
	const float pitchRad = Math::DegToRad(m_firstPersonBob.GetPitchDeg());
	const Vector3 forward = m_firstPersonForward * cosf(pitchRad) - up * sinf(pitchRad);
	const float rollRad = Math::DegToRad(m_firstPersonBob.GetRollDeg());
	const Vector3 cameraUp = up * cosf(rollRad) + right * sinf(rollRad);

	const Vector3 target = eyePosition + forward * FIRST_PERSON_LOOK_DISTANCE;
	g_camera3D->SetPosition(eyePosition);
	g_camera3D->SetTarget(target);
	g_camera3D->SetUp(cameraUp);
	g_camera3D->Update();

	// 腕はカメラに付いていくので、カメラを動かしたあとで動かす。
	m_firstPersonArms.Update(*m_player, m_firstPersonBob);
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
