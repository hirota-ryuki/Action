#include "stdafx.h"
#include "EnemyAnimCatalog.h"
#include "EnemyState.h"
#include "CharacterModel.h"
#include "System/JsonFile.h"
#include <imgui/imgui.h>

namespace
{
	// 見本帳の一覧（make_anim_catalog.py が書く）。
	constexpr const char* CATALOG_LIST_PATH = "Assets/modelData/enemy/BlackKnight/AnimCatalog/catalog.json";
	constexpr float MIN_SPEED = 0.1f;
	constexpr float MAX_SPEED = 2.0f;

	// 大文字・小文字を区別せずに、name に filter が含まれるか。
	bool ContainsNoCase(const std::string& name, const char* filter)
	{
		if (filter[0] == '\0') {
			return true;
		}
		auto lower = [](std::string s) {
			for (char& c : s) {
				c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
			}
			return s;
		};
		return lower(name).find(lower(filter)) != std::string::npos;
	}
}

void EnemyAnimCatalog::LoadList()
{
	m_isListLoaded = true;
	m_entries.clear();
	m_categories.clear();
	m_listError.clear();

	JsonFile::Json json;
	if (!JsonFile::TryLoad(CATALOG_LIST_PATH, json) || !json.contains("clips")) {
		m_listError = u8"一覧（AnimCatalog/catalog.json）がありません。make_anim_catalog.py で作ってください。";
		return;
	}
	for (const auto& clip : json["clips"]) {
		Entry entry;
		entry.name = clip.value("name", "");
		entry.category = clip.value("category", u8"その他");
		entry.path = clip.value("path", "");
		entry.isLoop = clip.value("loop", false);
		if (entry.name.empty() || entry.path.empty()) {
			continue;
		}
		if (std::find(m_categories.begin(), m_categories.end(), entry.category) == m_categories.end()) {
			m_categories.push_back(entry.category);
		}
		m_entries.push_back(entry);
	}
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
	if (!m_isListLoaded) {
		LoadList();
	}

	ImGui::SetNextWindowSize(ImVec2(360.0f, 560.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin(u8"黒騎士のモーション");

	if (!m_listError.empty()) {
		ImGui::TextWrapped("%s", m_listError.c_str());
	}
	if (ImGui::Button(u8"一覧を読み直す")) {
		LoadList();
		m_selected = -1;
		m_loaded = -1;
		Stop();
	}
	ImGui::SameLine();
	ImGui::Text(u8"%d 本", static_cast<int>(m_entries.size()));

	// 再生の操作
	ImGui::Separator();
	if (m_selected >= 0 && m_selected < static_cast<int>(m_entries.size())) {
		ImGui::Text(u8"再生中: %s", m_entries[m_selected].name.c_str());
	}
	else {
		ImGui::TextUnformatted(u8"下の一覧から選ぶと、黒騎士がその場で再生する。");
	}
	if (m_loadFailed) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), u8"読めませんでした（ファイルがないか、変換に失敗）。");
	}
	if (m_isActive) {
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
		const float fps = 30.0f;
		ImGui::SliderFloat(u8"時間", &m_time, 0.0f, max(m_length, 0.001f), u8"%.2f 秒");
		ImGui::Text(u8"長さ %.2f 秒（%d コマ）。コマ %d", m_length,
			static_cast<int>(m_length * fps + 0.5f) + 1, static_cast<int>(m_time * fps + 0.5f));
	}

	// 一覧
	ImGui::Separator();
	ImGui::InputText(u8"検索", m_filter, sizeof(m_filter));
	ImGui::BeginChild("list");
	for (const std::string& category : m_categories) {
		const bool isFiltering = m_filter[0] != '\0';
		if (isFiltering) {
			ImGui::SetNextItemOpen(true);
		}
		if (!ImGui::TreeNode(category.c_str())) {
			continue;
		}
		for (int i = 0; i < static_cast<int>(m_entries.size()); i++) {
			const Entry& entry = m_entries[i];
			if (entry.category != category || !ContainsNoCase(entry.name, m_filter)) {
				continue;
			}
			if (ImGui::Selectable(entry.name.c_str(), m_selected == i && m_isActive)) {
				m_selected = i;
				m_isLoop = entry.isLoop;
				m_time = 0.0f;
				m_isPlaying = true;
				m_isActive = true;
			}
		}
		ImGui::TreePop();
	}
	ImGui::EndChild();
	ImGui::End();
}

void EnemyAnimCatalog::Update(CharacterModel& model, float deltaTime)
{
	if (m_selected < 0 || m_selected >= static_cast<int>(m_entries.size())) {
		return;
	}
	// 選び直したら、枠に読み直す（押したときに初めて読む）。
	if (m_loaded != m_selected) {
		const Entry& entry = m_entries[m_selected];
		m_loadFailed = !model.ReloadAnimationClip(enEnemyAnimClip_Catalog, entry.path, false);
		m_loaded = m_selected;
		m_length = m_loadFailed ? 0.0f : model.GetAnimationClipLength(enEnemyAnimClip_Catalog);
		m_time = 0.0f;
		// 時間はこちらで進めるので、エンジンでは進めない。
		model.GetModelRender().SetAnimationSpeed(0.0f);
	}
	if (m_loadFailed || m_length <= 0.0f) {
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
	model.GetModelRender().SetAnimationSpeed(0.0f);
	model.PlayAnimationAtTime(enEnemyAnimClip_Catalog, m_time);
}
