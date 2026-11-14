#include "stdafx.h"
#include "SlashMagic.h"

namespace
{
	// 三日月形の板（tools/BlenderScripts/make_slash_wave.py で作る）。色は同じフォルダの SlashWave.png。
	constexpr const char* MODEL_PATH = "Assets/modelData/effect/SlashWave/SlashWave.fbx";
	constexpr float MODEL_SCALE = 1.0f;

	MagicSettings MakeSettings()
	{
		MagicSettings settings;
		settings.speed = 1100.0f;   // プレイヤーがよけられるくらいの速さ。
		settings.range = 1500.0f;
		settings.radius = 45.0f;    // 三日月の幅（160）より小さめにして、端をかすっただけでは当たらないようにする。
		return settings;
	}
}

SlashMagic::SlashMagic()
	: MagicBase(MakeSettings())
{
}

bool SlashMagic::OnStart()
{
	m_model.Init(MODEL_PATH, nullptr, 0, enModelUpAxisZ, false);
	m_isVisible = true;
	return true;
}

void SlashMagic::OnMove()
{
	// 進む向き（GetRotation）に向けてから、その前の軸まわりに回して斜めにする。
	Quaternion roll;
	roll.SetRotationDeg(Vector3::AxisZ, m_rollDeg);
	m_model.SetPosition(GetPosition());
	m_model.SetRotation(GetRotation() * roll);
	m_model.SetScale(Vector3(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE));
	m_model.Update();
}

void SlashMagic::OnExplode(const Vector3& hitPosition)
{
	(void)hitPosition;
	m_isVisible = false;
}

void SlashMagic::Render(RenderContext& renderContext)
{
	if (m_isVisible) {
		m_model.Draw(renderContext);
	}
}
