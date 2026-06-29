#pragma once

class Player;

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
	bool TryGetMouseRotationInput(float& yawDeg, float& pitchDeg);
	void EndMouseLook();

	Player* m_player = nullptr;
	ModelRender m_firstPersonGunRender;
	Vector3 m_toCameraPos = Vector3(0.0f, 200.0f, -400.0f);
	Vector3 m_firstPersonForward = Vector3::AxisZ;
	EnCameraMode m_cameraMode = EnCameraMode::ThirdPerson;
	float m_defaultNear = 1.0f;
	bool m_isMouseLookActive = false;
};
