#pragma once

#include "UIElement.h"

// 横向きのバー。SetRate(0〜1) で中身の長さを変える（HP・クールダウンなど）。
// 位置は「バーの左端の中央」。中身は左端から右へ伸びる。
// 遅れて減るバー（useTrail）をオンにすると、減った分が白く残り、少し待ってから追いかけて縮む。
class Gauge : public UIElement
{
public:
	struct Settings
	{
		float width = 400.0f;
		float height = 20.0f;
		Vector4 backColor = { 0.0f, 0.0f, 0.0f, 0.6f };   // 空きの部分。
		Vector4 barColor = { 0.85f, 0.15f, 0.15f, 1.0f }; // 中身。
		bool useTrail = false;                             // 遅れて減るバーを使うか。
		Vector4 trailColor = { 1.0f, 1.0f, 1.0f, 0.9f };
		float trailDelay = 0.5f;                           // 減ってから、白いバーが縮み始めるまでの秒数。
		float trailSpeed = 0.5f;                           // 白いバーが縮む速さ（1秒にゲージ全体の何割か）。
	};

	void Init(const Settings& settings);
	// 中身の割合。0〜1 に収めてから使う。
	// 増えたときは白いバーもすぐそろえる（白が残らないように）。
	void SetRate(float rate);
	// 白いバーを今の割合にそろえる。表示し始めるときに呼ぶ（減っていく演出にならないように）。
	void ResetTrail();
	float GetRate() const
	{
		return m_rate;
	}

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

private:
	// 左端の中央を基準に、横の長さを rate 倍にして置く。
	void UpdateBar(SpriteRender& sprite, float rate, const Vector4& color);

	Settings m_settings;
	SpriteRender m_back;
	SpriteRender m_trail;
	SpriteRender m_bar;
	float m_rate = 1.0f;
	float m_trailRate = 1.0f;
	float m_trailWait = 0.0f;  // 白いバーが縮み始めるまでの残り秒数。
};
