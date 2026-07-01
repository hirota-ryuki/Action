#pragma once

class Player;

// ゲームカメラを管理するクラス。
// Bボタンで三人称／一人称を切り替える。一人称時は銃モデルを描画し、マウスルックが有効になる。
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
	void InitFirstPersonGun();
	void UpdateFirstPersonGun();
	void RotateFirstPersonForward();
	void GetCameraRotationInput(float& yawDeg, float& pitchDeg);
	// マウス入力を取得する。カーソルをウィンドウ中央に固定する処理を含む。
	bool TryGetMouseRotationInput(float& yawDeg, float& pitchDeg);
	void RestorePlayerRenderBody();
	void EndMouseLook();

	Player* m_player = nullptr;
	ModelRender m_firstPersonGunRender;
	Vector3 m_toCameraPos = Vector3(0.0f, 250.0f, -250.0f);  // プレイヤーからカメラへのオフセット（三人称）。画角を広げた分、距離は半分に近づけてある。
	Vector3 m_firstPersonForward = Vector3::AxisZ;            // 一人称カメラの向き（正規化済み）。
	EnCameraMode m_cameraMode = EnCameraMode::ThirdPerson;
	float m_defaultNear = 1.0f;         // 三人称時のNearを保持しておき、一人称終了時に復元する。
	bool m_isMouseLookActive = false;   // trueの間カーソルを非表示にして中央固定する。
};
