#pragma once

#include "UIElement.h"
#include "UISprite.h"

// 横向きのバー。SetRate(0〜1) で中身の長さを変える（HP・クールダウンなど）。
// 位置は「バーの左端の中央」（枠は含まない）。中身は左端から右へ伸びる。
// 下から 枠 → 空き → 遅れて減るバー → 中身 の順に重ねて描く。
// 遅れて減るバー（useTrail）をオンにすると、減った分が残り、少し待ってから追いかけて縮む。
class Gauge : public UIElement
{
public:
	struct Settings
	{
		float width = 400.0f;
		float height = 20.0f;
		float borderSize = 0.0f;                            // 枠の太さ。0 なら枠を描かない。
		Vector4 borderColor = { 0.0f, 0.0f, 0.0f, 0.8f };
		Vector4 backColor = { 0.0f, 0.0f, 0.0f, 0.6f };     // 空きの部分。
		Vector4 barColor = { 0.85f, 0.15f, 0.15f, 1.0f };   // 中身。
		const char* barImagePath = UISprite::kWhitePath;     // 中身と遅れて減るバーの画像。色はこれに掛ける。
		bool useTrail = false;                               // 遅れて減るバーを使うか。
		Vector4 trailColor = { 1.0f, 1.0f, 1.0f, 0.9f };
		float trailDelay = 0.5f;                             // 減ってから、遅れて減るバーが縮み始めるまでの秒数。
		float trailSpeed = 0.5f;                             // 遅れて減るバーが縮む速さ（1秒にゲージ全体の何割か）。
	};

	void Init(const Settings& settings);
	// 中身の割合。0〜1 に収めてから使う。
	// 増えたときは遅れて減るバーもすぐそろえる（残らないように）。
	void SetRate(float rate);
	// 遅れて減るバーを今の割合にそろえる。表示し始めるときに呼ぶ（減っていく演出にならないように）。
	void ResetTrail();
	float GetRate() const
	{
		return m_rate;
	}
	const Settings& GetSettings() const
	{
		return m_settings;
	}

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

private:
	// 左端の中央を x の位置に合わせ、横の長さを rate 倍にして置く。
	void UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color);

	Settings m_settings;
	SpriteRender m_border;
	SpriteRender m_back;
	SpriteRender m_trail;
	SpriteRender m_bar;
	float m_rate = 1.0f;
	float m_trailRate = 1.0f;
	float m_trailWait = 0.0f;  // 遅れて減るバーが縮み始めるまでの残り秒数。
};
