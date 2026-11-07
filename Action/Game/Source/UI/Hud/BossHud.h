#pragma once

#include "Gauge.h"
#include "Text.h"

class Character;

// ボス用の HUD。画面上の中央に、名前とHPゲージ（遅れて減るバー付き）を出す。
// Show() でフェードインし、Hide() でフェードアウトする。UIElement は継承しない。UIManager が持つ。
// Boss クラスはまだないので、今は Character と名前を受け取る。Boss ができたら const Boss* に変える。
class BossHud
{
public:
	void Init();
	// 表示を始める。ゲージの白いバーを今のHPにそろえる。
	void Show(const Character* boss, const wchar_t* name);
	// 表示をやめる。ボスへのポインタはすぐ捨てる（このあと DeleteGO してよい）。
	// フェードアウトが終わるまでは、最後の値のまま描く。
	void Hide();
	bool IsShown() const
	{
		return m_boss != nullptr;
	}
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	void SetAlpha(float alpha);

	const Character* m_boss = nullptr;
	Text m_name;
	Gauge m_hp;
	float m_alpha = 0.0f;        // 今の透明度。
	float m_targetAlpha = 0.0f;  // Show で 1、Hide で 0。
};
