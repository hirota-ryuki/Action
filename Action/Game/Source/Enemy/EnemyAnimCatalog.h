#pragma once

class CharacterModel;

// 黒騎士のモーションの確認（開発用）。F1 の画面に窓「黒騎士のモーション」を出し、
// 今の黒騎士に入っているモーション（Idle〜Dead）を選んで、その場で再生させる。
// 使い方: 持ち主は毎フレーム UpdateUI() を呼び、IsActive() の間は自分の動きを止めて Update() を呼ぶ。
class EnemyAnimCatalog
{
public:
	// 窓を出す。F1 の画面を出していないときは何もしない。
	void UpdateUI();
	// 再生しているか（再生中は持ち主の AI・ステートを止める）。
	bool IsActive() const
	{
		return m_isActive;
	}
	// 再生を止めたフレームだけ true を返す（持ち主がステートを戻すため）。
	bool ConsumeStopped()
	{
		const bool stopped = m_isStopped;
		m_isStopped = false;
		return stopped;
	}
	// 再生の時間を進めて、そのコマのポーズにする。
	void Update(CharacterModel& model, float deltaTime);

private:
	void Stop();

	bool m_isActive = false;
	bool m_isStopped = false;
	int m_selected = -1;      // 選んだモーション（一覧の番号）
	bool m_isChanged = false; // 選び直した（長さを読み直す）

	bool m_isPlaying = true;
	bool m_isLoop = true;
	float m_speed = 1.0f;
	float m_time = 0.0f;
	float m_length = 0.0f;
};
