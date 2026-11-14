#pragma once

#include "CharacterInput.h"
#include "FirstPersonArms.h"

class Player;

// ゲームカメラを管理するクラス。
// V キー（パッドは B ボタン）で三人称／一人称を切り替える。一人称時は腕と手を描画し、マウスルックが有効になる。
class GameCamera : public IGameObject
{
public:
	GameCamera();
	~GameCamera();
	bool Start();
	void Update();
	void Render(RenderContext& rc);

private:
	enum class EnCameraMode
	{
		ThirdPerson,
		FirstPerson,
	};

	void ToggleCameraMode();
	void UpdateThirdPersonCamera();
	void UpdateFirstPersonCamera();
	// 三人称／一人称を切り替えるボタンを、このフレームで押したか。
	bool IsToggleTriggered();
	void RotateFirstPersonForward();
	void GetCameraRotationInput(float& yawDeg, float& pitchDeg);
	// マウス入力を取得する。カーソルをウィンドウ中央に固定する処理を含む。
	bool TryGetMouseRotationInput(float& yawDeg, float& pitchDeg);
	void RestorePlayerRenderBody();
	void EndMouseLook();

	Player* m_player = nullptr;
	FirstPersonArms m_firstPersonArms;  // 一人称のときに映す腕と手。
	ButtonState m_toggleKey;             // V キー。押した瞬間を見る。
	Vector3 m_toCameraPos = Vector3(0.0f, 250.0f, -250.0f);  // プレイヤーからカメラへのオフセット（三人称）。画角を広げた分、距離は半分に近づけてある。
	Vector3 m_firstPersonForward = Vector3::AxisZ;            // 一人称カメラの向き（正規化済み）。
	EnCameraMode m_cameraMode = EnCameraMode::ThirdPerson;
	float m_defaultNear = 1.0f;         // 三人称時のNearを保持しておき、一人称終了時に復元する。
	bool m_isMouseLookActive = false;   // trueの間カーソルを非表示にして中央固定する。
};
