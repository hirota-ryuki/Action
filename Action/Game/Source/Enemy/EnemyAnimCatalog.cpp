#include "stdafx.h"
#include "EnemyAnimCatalog.h"
#include "EnemyState.h"
#include "CharacterModel.h"
#include <imgui/imgui.h>

namespace
{
	constexpr float MIN_SPEED = 0.1f;
	constexpr float MAX_SPEED = 2.0f;
	constexpr float ANIM_FPS = 30.0f;

	// 今の黒騎士に入っているモーション（Enemy.cpp の MakeModelDesc と同じ並び）。
	struct Entry
	{
		const char* name;
		int clip;
		bool isLoop;
	};
	constexpr Entry ENTRIES[] = {
		{ u8"Idle（立ち止まる）", enEnemyAnimClip_Idle, true },
		{ u8"Walk（歩く）", enEnemyAnimClip_Walk, true },
		{ u8"Run（剣を肩に乗せて走る）", enEnemyAnimClip_Run, true },
		{ u8"Attack1（溜めて踏み込み、横に斬り抜ける。溜めと踏み込みは入らない）", enEnemyAnimClip_Attack1, false },
		{ u8"Attack2（連続斬り）", enEnemyAnimClip_Attack2, false },
		{ u8"Attack3（斬撃を5つ飛ばす。斬撃は出ない）", enEnemyAnimClip_Attack3, false },
		{ u8"Hit（やられ）", enEnemyAnimClip_Hit, false },
		{ u8"Dead（死ぬ）", enEnemyAnimClip_Dead, false },
	};
	constexpr int ENTRY_NUM = sizeof(ENTRIES) / sizeof(ENTRIES[0]);
}

void EnemyAnimCatalog::Stop()
{
	if (m_isActive) {
		m_isActive = false;
		m_isStopped = true;
	}
}

void EnemyAnimCatalog::UpdateUI()
{
	// F1 で開発用の画面を出しているときだけ（Release では ImGui を起動しないので、見ないで呼ぶと落ちる）。
	if (!DebugUI::IsVisible()) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(360.0f, 420.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin(u8"黒騎士のモーション");

	if (m_isActive && m_selected >= 0) {
		ImGui::Text(u8"再生中: %s", ENTRIES[m_selected].name);
		if (ImGui::Button(m_isPlaying ? u8"止める" : u8"再生")) {
			m_isPlaying = !m_isPlaying;
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"最初から")) {
			m_time = 0.0f;
			m_isPlaying = true;
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"AI に戻す")) {
			Stop();
		}
		ImGui::SliderFloat(u8"速さ", &m_speed, MIN_SPEED, MAX_SPEED, u8"%.2f 倍");
		ImGui::Checkbox(u8"ループ", &m_isLoop);
		ImGui::SliderFloat(u8"時間", &m_time, 0.0f, max(m_length, 0.001f), u8"%.2f 秒");
		ImGui::Text(u8"長さ %.2f 秒（%d コマ）。コマ %d", m_length,
			static_cast<int>(m_length * ANIM_FPS + 0.5f) + 1, static_cast<int>(m_time * ANIM_FPS + 0.5f));
	}
	else {
		ImGui::TextUnformatted(u8"選ぶと、黒騎士が AI を止めてその場で再生する。");
	}

	ImGui::Separator();
	for (int i = 0; i < ENTRY_NUM; i++) {
		if (ImGui::Selectable(ENTRIES[i].name, m_isActive && m_selected == i)) {
			m_selected = i;
			m_isChanged = true;
			m_isLoop = ENTRIES[i].isLoop;
			m_time = 0.0f;
			m_isPlaying = true;
			m_isActive = true;
		}
	}
	ImGui::End();
}

void EnemyAnimCatalog::Update(CharacterModel& model, float deltaTime)
{
	if (m_selected < 0 || m_selected >= ENTRY_NUM) {
		return;
	}
	const int clip = ENTRIES[m_selected].clip;
	if (m_isChanged) {
		m_isChanged = false;
		m_length = model.GetAnimationClipLength(clip);
	}
	if (m_length <= 0.0f) {
		return;
	}
	if (m_isPlaying) {
		m_time += deltaTime * m_speed;
		if (m_time > m_length) {
			if (m_isLoop) {
				m_time = fmodf(m_time, m_length);
			}
			else {
				m_time = m_length;
				m_isPlaying = false;
			}
		}
	}
	m_time = max(0.0f, min(m_time, m_length));
	// 時間はこちらで進めるので、エンジンでは進めない。
	model.GetModelRender().SetAnimationSpeed(0.0f);
	model.PlayAnimationAtTime(clip, m_time);
}
