#pragma once

#include "CameraCollision.h"
#include "CharacterInput.h"
#include "FirstPersonArms.h"
#include "FirstPersonBob.h"

class Player;

// ゲームカメラを管理するクラス。
// 三人称は、近くから見る「寄り」（原神くらい。最初はこれ）と、上から見下ろす「俯瞰」の2つ。
// V キー（パッドは B ボタン）で三人称／一人称を切り替える。一人称時は腕と手を描画し、マウスルックが有効になる。
// B キーで俯瞰に切り替え、もう一度押すと寄りに戻る。一人称のときに押すと俯瞰に移る。
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
		ThirdPersonClose,     // 三人称の寄り。
		ThirdPersonOverhead,  // 三人称の俯瞰。
		FirstPerson,
	};

	// 三人称のモードごとの、カメラの置き方。
	struct ThirdPersonSetting
	{
		float distance;      // 注視点からカメラまでの距離。
		float targetHeight;  // 注視点の、プレイヤーの足元からの高さ。
		float pitchDeg;      // モードに入ったときに、注視点を見下ろす角度（度）。
		float viewAngleDeg;  // 画角（縦、度）。
	};

	void ToggleCameraMode();
	// 俯瞰と寄りを入れ替える。一人称のときは俯瞰に移る。
	void ToggleOverheadMode();
	// 三人称の寄りか俯瞰に移る。一人称からも、もう片方の三人称からも呼ぶ。
	void ChangeThirdPersonMode(EnCameraMode mode);
	// 今のモードの、三人称のカメラの置き方。
	const ThirdPersonSetting& GetThirdPersonSetting() const;
	// forward の向きの後ろから、今のモードの距離と角度で見るように m_toCameraPos を置き直す。
	void ResetThirdPersonOffset(const Vector3& forward);
	void UpdateThirdPersonCamera();
	// カメラが頭のすぐ近くまで寄ったら体を隠し、離れたら映す。三人称のときに呼ぶ。
	void UpdateBodyHiddenByCollision();
	// 壁よけで隠した体を映す。
	void ShowBodyHiddenByCollision();
	void UpdateFirstPersonCamera();
	// 三人称／一人称を切り替えるボタンを、このフレームで押したか。
	bool IsToggleTriggered();
	// 俯瞰と寄りを切り替えるキーを、このフレームで押したか。
	bool IsOverheadToggleTriggered();
	void RotateFirstPersonForward();
	void GetCameraRotationInput(float& yawDeg, float& pitchDeg);
	// マウス入力を取得する。カーソルをウィンドウ中央に固定する処理を含む。
	bool TryGetMouseRotationInput(float& yawDeg, float& pitchDeg);
	void RestorePlayerRenderBody();
	void EndMouseLook();

	Player* m_player = nullptr;
	FirstPersonArms m_firstPersonArms;  // 一人称のときに映す腕と手。
	FirstPersonBob m_firstPersonBob;    // 一人称のカメラと腕の、歩き・走りの揺れと着地の沈み。
	CameraCollision m_cameraCollision;  // 三人称のカメラを、壁の手前に寄せる。
	ButtonState m_toggleKey;             // V キー。押した瞬間を見る。
	ButtonState m_overheadKey;           // B キー。押した瞬間を見る。
	Vector3 m_toCameraPos = Vector3(0.0f, 250.0f, -250.0f);  // 注視点からカメラへのオフセット（三人称）。モードに入るときに置き直す。
	Vector3 m_firstPersonForward = Vector3::AxisZ;            // 一人称カメラの向き（正規化済み）。
	EnCameraMode m_cameraMode = EnCameraMode::ThirdPersonClose;
	EnCameraMode m_lastThirdPersonMode = EnCameraMode::ThirdPersonClose;  // 一人称から V で戻るときの三人称のモード。
	// 寄りと俯瞰を切り替えたときに、前のカメラの位置からなめらかに移すための値。
	float m_blendTimer = 0.0f;          // 移し終わるまでの残りの秒数。0 なら移し終わっている。
	Vector3 m_blendFromPosition;        // 切り替える前のカメラの位置。
	Vector3 m_blendFromTarget;          // 切り替える前の注視点。
	float m_blendFromViewAngle = 0.0f;  // 切り替える前の画角（ラジアン）。
	float m_defaultNear = 1.0f;         // 三人称時のNearを保持しておき、一人称終了時に復元する。
	bool m_isMouseLookActive = false;   // trueの間カーソルを非表示にして中央固定する。
	bool m_isBodyHiddenByCollision = false;  // 壁よけでカメラが頭のすぐ近くまで寄ったので、体を隠しているか。
};
