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

	input.isRun = IsKeyPressed(VK_LSHIFT);
	input.jump.Update(pad.IsPress(enButtonA));
	// パッド未接続時はXボタンがLキーに割り当てられているので、どちらでも同じ動きになる。
	input.rush.Update(IsKeyPressed('L') || pad.IsPress(enButtonX));
	input.fire.Update((IsGameWindowActive() && IsKeyPressed(VK_LBUTTON)) || pad.IsPress(enButtonY));
	// パッド未接続時は右トリガー(RB2)が8キーに割り当てられている。
	input.subFire.Update((IsGameWindowActive() && IsKeyPressed(VK_RBUTTON)) || pad.IsPress(enButtonRB2));

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

bool PlayerInputSource::IsKeyPressed(int virtualKey)
{
	// 最上位ビットが「今押されている」を表す。
	return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}
