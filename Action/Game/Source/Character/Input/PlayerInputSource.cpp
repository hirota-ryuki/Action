#include "stdafx.h"
#include "PlayerInputSource.h"
#include "system.h"

PlayerInputSource::PlayerInputSource(int padNo)
	: m_padNo(padNo)
{
}

void PlayerInputSource::Update(const InputSourceContext& context, CharacterInput& input)
{
	const GamePad& pad = *g_pad[m_padNo];

	// スティックはカメラから見た向きなので、カメラの水平な前・右を使ってワールドの向きにする。
	Vector3 cameraForward = g_camera3D->GetForward();
	Vector3 cameraRight = g_camera3D->GetRight();
	cameraForward.y = 0.0f;
	cameraRight.y = 0.0f;
	cameraForward.Normalize();
	cameraRight.Normalize();
	input.moveDirection = cameraForward * pad.GetLStickYF() + cameraRight * pad.GetLStickXF();
	input.lookDirection = Vector3::Zero;
	// 攻撃した瞬間に、カメラが向いている方向を向く。
	input.attackDirection = cameraForward;

	// ジャンプはスペース（パッドは A）。パッド未接続時は A ボタンが J キーに割り当てられているので、J でも跳べる。
	input.jump.Update(IsKeyPressed(VK_SPACE) || pad.IsPress(enButtonA));
	// 左Shift（パッドは X）: 押した瞬間にダッシュ、押しっぱなしで走る。
	// 走るのはダッシュが終わったあとなので、単押しを繰り返すとダッシュが続けて出る。
	// パッド未接続時は X ボタンが L キーに割り当てられているので、L でも同じ動きになる。
	const bool isDashPress = IsKeyPressed(VK_LSHIFT) || pad.IsPress(enButtonX);
	input.dash.Update(isDashPress);
	input.isRun = isDashPress;
	input.fire.Update((CanUseMouse() && IsKeyPressed(VK_LBUTTON)) || pad.IsPress(enButtonY));
	// パッド未接続時は右トリガー(RB2)が8キーに割り当てられている。
	input.subFire.Update((CanUseMouse() && IsKeyPressed(VK_RBUTTON)) || pad.IsPress(enButtonRB2));

	m_prevMagicKey.Update(IsKeyPressed('Q'));
	m_nextMagicKey.Update(IsKeyPressed('E'));
	input.magicSwitch = 0;
	if (m_prevMagicKey.isTrigger || pad.IsTrigger(enButtonLB1)) {
		input.magicSwitch -= 1;
	}
	if (m_nextMagicKey.isTrigger || pad.IsTrigger(enButtonRB1)) {
		input.magicSwitch += 1;
	}

	// 一人称では視線の方向へ目の位置から撃つ。三人称ではキャラクターの正面へ撃つ（hasAim = false）。
	input.hasAim = context.isFirstPerson;
	if (input.hasAim) {
		input.aimOrigin = g_camera3D->GetPosition();
		input.aimDirection = g_camera3D->GetForward();
	}
}

bool PlayerInputSource::IsGameWindowActive()
{
	return g_hWnd != nullptr && GetForegroundWindow() == g_hWnd;
}

bool PlayerInputSource::CanUseMouse()
{
	// 開発用の画面（ImGui）を出している間は、クリックを ImGui に使うので、魔法を撃たない。
	return IsGameWindowActive() && !DebugUI::IsVisible();
}

bool PlayerInputSource::IsKeyPressed(int virtualKey)
{
	// 最上位ビットが「今押されている」を表す。
	return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}
