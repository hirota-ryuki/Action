#pragma once

class CharacterModel;

// 黒騎士のモーションの見本帳（開発用）。F1 の開発用の画面に窓を出し、選んだモーションを黒騎士に再生させる。
// 見本のモーションは tools/BlenderScripts/make_anim_catalog.py が黒騎士へ移して、
// Assets/modelData/enemy/BlackKnight/AnimCatalog/ に書き出す（数が多いのでリポジトリには入れない）。
// 一覧は同じフォルダの catalog.json。モーションは押したときに初めて読む（起動を遅くしないため）。
// 使い方: 持ち主は毎フレーム UpdateUI() を呼び、IsActive() の間は自分の動きを止めて Update() を呼ぶ。
class EnemyAnimCatalog
{
public:
	// 窓を出す。F1 の画面を出していないときは何もしない。
	void UpdateUI();
	// 見本を再生しているか（再生中は持ち主の AI・ステートを止める）。
	bool IsActive() const
	{
		return m_isActive;
	}
	// 見本の再生を止めたフレームだけ true を返す（持ち主がステートを戻すため）。
	bool ConsumeStopped()
	{
		const bool stopped = m_isStopped;
		m_isStopped = false;
		return stopped;
	}
	// 選んだモーションを読み、再生の時間を進めて、そのコマのポーズにする。
	void Update(CharacterModel& model, float deltaTime);

private:
	struct Entry
	{
		std::string name;
		std::string category;
		std::string path;
		bool isLoop = false;
	};

	void LoadList();
	void Stop();

	std::vector<Entry> m_entries;
	std::vector<std::string> m_categories;  // 一覧に出す順
	bool m_isListLoaded = false;
	std::string m_listError;                // 一覧が読めなかったときの理由

	bool m_isActive = false;
	bool m_isStopped = false;
	int m_selected = -1;     // 選んだモーション（m_entries の番号）
	int m_loaded = -1;       // 枠に読んであるモーション
	bool m_loadFailed = false;
	char m_filter[64] = {};

	bool m_isPlaying = true;
	bool m_isLoop = true;
	float m_speed = 1.0f;
	float m_time = 0.0f;
	float m_length = 0.0f;
};
