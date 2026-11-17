#include "stdafx.h"
#include "SlashMagic.h"
#include "MagicConfig.h"

namespace
{
	// 三日月形の板（tools/BlenderScripts/make_slash_wave.py で作る）。色は同じフォルダの SlashWave.png。
	constexpr const char* MODEL_PATH = "Assets/modelData/effect/SlashWave/SlashWave.fbx";
	constexpr float MODEL_SCALE = 1.0f;
}

SlashMagic::SlashMagic()
	: MagicBase(MagicConfig::GetSlash().settings)
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
