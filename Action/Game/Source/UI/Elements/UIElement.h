#pragma once

// 画面に出る UI の部品（Gauge・Icon・Text）の基底クラス。
// 部品は「言われた値を描く」だけで、キャラクターのことは知らない。値を入れるのは PlayerHud・BossHud などの係。
// 座標は画面の中心が原点で、右が +x、上が +y（画面は UI_SPACE_WIDTH × UI_SPACE_HEIGHT）。
class UIElement
{
public:
	virtual ~UIElement() = default;

	void SetPosition(const Vector2& position)
	{
		m_position = position;
	}
	const Vector2& GetPosition() const
	{
		return m_position;
	}
	// false の間は Draw() で何も描かない。
	void SetVisible(bool isVisible)
	{
		m_isVisible = isVisible;
	}
	bool IsVisible() const
	{
		return m_isVisible;
	}
	// 透明度（0 で見えない、1 で不透明）。フェードに使う。部品の色のアルファに掛ける。
	void SetAlpha(float alpha)
	{
		m_alpha = min(1.0f, max(0.0f, alpha));
	}
	float GetAlpha() const
	{
		return m_alpha;
	}

	// 毎フレーム、描く前に1回呼ぶ。位置・大きさ・色を描画用のものに反映する。
	virtual void Update(float deltaTime) = 0;
	// 毎フレーム、IGameObject::Render() から呼ぶ。見えないときは何もしない。
	void Draw(RenderContext& rc)
	{
		if (!m_isVisible || m_alpha <= 0.0f) {
			return;
		}
		OnDraw(rc);
	}

protected:
	virtual void OnDraw(RenderContext& rc) = 0;

	// 色のアルファに、この部品の透明度を掛けたもの。
	Vector4 ApplyAlpha(const Vector4& color) const
	{
		return Vector4(color.x, color.y, color.z, color.w * m_alpha);
	}

	Vector2 m_position = Vector2::Zero;

private:
	bool m_isVisible = true;
	float m_alpha = 1.0f;
};
