# UIの構成の手順

設計: [UIの構成の設計](UIの構成の設計.md)
経緯: [UIの経緯](UIの経緯.md)

画面の UI の部品（`UIElement`・`Text`・`Icon`）と、UI をまとめて持つ `UIManager` を作り、`Game` から作るまでの手順です。

**出来上がるもの:** `Game` が `UIManager` を作り、毎フレーム更新して描く流れができます。まだ何も描かないので、画面は変わりません。[HUDの手順](HUDの手順.md) で、ここで作った部品を使って HP ゲージなどを出します。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- ゲームの手順書は [README の「ゲームを作る順番」](../../README.md#ゲームを作る順番) の順に進める。この手順の前に、[炎魔法発射後の燃える床の手順](../魔法/炎魔法発射後の燃える床の手順.md) まで済ませておく。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | 部品の基底クラス `UIElement` | ビルドが通る |
| 2 | 文字の部品 `Text`（エンジンに文字の幅を測る関数を足す） | ビルドが通る |
| 3 | 画像の部品 `Icon` と、画像のパス `UISprite` | ビルドが通る |
| 4 | `UIManager` を作り、`Game` から作る | 起動して落ちない |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- 書き換えるところは、「前」と「後」の枠を並べています。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) | 新しく作る | 1 |
| [Text.h](../../../Game/Source/UI/Elements/Text.h) | 新しく作る | 2 |
| [FontEngine.h](../../../k2EngineLow/graphics/font/FontEngine.h) | 追加 | 2 |
| [Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) | 新しく作る | 2 |
| [UISprite.h](../../../Game/Source/UI/Elements/UISprite.h) | 新しく作る | 3 |
| [Icon.h](../../../Game/Source/UI/Elements/Icon.h) | 新しく作る | 3 |
| [Icon.cpp](../../../Game/Source/UI/Elements/Icon.cpp) | 新しく作る | 3 |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 新しく作る | 4 |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 新しく作る | 4 |
| [Game.h](../../../Game/Source/Core/Game.h) | 追加 | 4 |
| [Game.cpp](../../../Game/Source/Core/Game.cpp) | 追加・書き換え | 4 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: 部品の基底クラス UIElement を作る

部品は「言われた値を描く」だけで、キャラクターのことは知りません。座標は画面の中心が原点で、右が +x、上が +y です（画面は `UI_SPACE_WIDTH` × `UI_SPACE_HEIGHT` = 1920 × 1080）。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) | 新しく作る（位置・見えるか・透明度、描く流れ） |

`Source` フィルターの下に `UI` フィルター、その下に `Elements` フィルターを作り、`UIElement.h` を作ります。ファイルの場所は `Action/Game/Source/UI/Elements/` です。

### 1-1. UIElement クラスの形を書く

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) を新しく作り、次の8行を書きます（1〜8行目）。

<!-- new: Action/Game/Source/UI/Elements/UIElement.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
```

</td><td>

```cpp
#pragma once

class UIElement
{
public:
	virtual ~UIElement() = default;

};
```

</td></tr></table>

### 1-2. 位置を決める・読む関数を足す

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) の **7行目**（`virtual ~UIElement() = default;` の下の空の行）の下、`};` の上に、8行足します（8〜15行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 8 -->
<table class="code-lines"><tr><td>

```cpp
8
9
10
11
12
13
14
15
```

</td><td>

```cpp
	void SetPosition(const Vector2& position)
	{
		m_position = position;
	}
	const Vector2& GetPosition() const
	{
		return m_position;
	}
```

</td></tr></table>

### 1-3. 見えるかを決める・読む関数を足す

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) の **15行目** `}` の下、`};` の上に、9行足します（16〜24行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 16 -->
<table class="code-lines"><tr><td>

```cpp
16
17
18
19
20
21
22
23
24
```

</td><td>

```cpp
	// false の間は Draw() で何も描かない。
	void SetVisible(bool isVisible)
	{
		m_isVisible = isVisible;
	}
	bool IsVisible() const
	{
		return m_isVisible;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 17〜20 | `false` の間は、描くときに何もしない |

### 1-4. 透明度を決める・読む関数を足す

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) の **24行目** `}` の下、`};` の上に、9行足します（25〜33行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 25 -->
<table class="code-lines"><tr><td>

```cpp
25
26
27
28
29
30
31
32
33
```

</td><td>

```cpp
	// 透明度（0 で見えない、1 で不透明）。フェードに使う。部品の色のアルファに掛ける。
	void SetAlpha(float alpha)
	{
		m_alpha = min(1.0f, max(0.0f, alpha));
	}
	float GetAlpha() const
	{
		return m_alpha;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 28 | 0（見えない）〜1（不透明）の外に出ないようにする。フェードに使う |

### 1-5. 更新と、描く関数を足す

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) の **33行目** `}` の下、`};` の上に、11行足します（34〜44行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 34 -->
<table class="code-lines"><tr><td>

```cpp
34
35
36
37
38
39
40
41
42
43
44
```

</td><td>

```cpp

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
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 36 | 毎フレーム、描く前に1回呼ぶ。派生クラスが必ず書く |
| 38〜44 | 見えないとき（見えない設定か、透明度 0）は何もしない。見えるときだけ派生クラスの `OnDraw()` を呼ぶ。この関数は派生クラスで変えない |

### 1-6. 派生クラスが書く、描く中身の関数を足す

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) の **44行目** `}` の下、`};` の上に、4行足します（45〜48行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 45 -->
<table class="code-lines"><tr><td>

```cpp
45
46
47
48
```

</td><td>

```cpp

protected:
	virtual void OnDraw(RenderContext& rc) = 0;

```

</td></tr></table>

### 1-7. 色に透明度を掛ける関数を足す

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) の **48行目**（`virtual void OnDraw(RenderContext& rc) = 0;` の下の空の行）の下、`};` の上に、5行足します（49〜53行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 49 -->
<table class="code-lines"><tr><td>

```cpp
49
50
51
52
53
```

</td><td>

```cpp
	// 色のアルファに、この部品の透明度を掛けたもの。
	Vector4 ApplyAlpha(const Vector4& color) const
	{
		return Vector4(color.x, color.y, color.z, color.w * m_alpha);
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 52 | 色のアルファに透明度を掛ける。部品ごとにフェードの処理を書かずに済む |

### 1-8. 持つ値を足す

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) の **53行目** `}` の下、`};` の上に、6行足します（54〜59行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 54 -->
<table class="code-lines"><tr><td>

```cpp
54
55
56
57
58
59
```

</td><td>

```cpp

	Vector2 m_position = Vector2::Zero;

private:
	bool m_isVisible = true;
	float m_alpha = 1.0f;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 55 | 位置。`protected` なので、派生クラスが描くときに使える |

### 1-9. コメントを書く

[UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) の **2行目**（`#pragma once` の下の空の行）の下、`class UIElement` の上に、3行足します（3〜5行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 3 -->
<table class="code-lines"><tr><td>

```cpp
3
4
5
```

</td><td>

```cpp
// 画面に出る UI の部品（Gauge・Icon・Text）の基底クラス。
// 部品は「言われた値を描く」だけで、キャラクターのことは知らない。値を入れるのは PlayerHud・BossHud などの係。
// 座標は画面の中心が原点で、右が +x、上が +y（画面は UI_SPACE_WIDTH × UI_SPACE_HEIGHT）。
```

</td></tr></table>

### 出来上がった UIElement.h

<!-- full: Action/Game/Source/UI/Elements/UIElement.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
50
51
52
53
54
55
56
57
58
59
60
61
62
63
```

</td><td>

```cpp
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
```

</td></tr></table>

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: 文字の部品 Text を作る

文字を出す部品です。右寄せ・中央寄せをするために、文字の幅を測る関数をエンジンに足します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Text.h](../../../Game/Source/UI/Elements/Text.h) | 新しく作る（文字・色・大きさ・寄せ方・影） |
| [FontEngine.h](../../../k2EngineLow/graphics/font/FontEngine.h) | 追加（文字の幅と高さを測る関数） |
| [Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) | 新しく作る（文字を測る、寄せる） |

### 2-1. Text クラスの形を書く

[Text.h](../../../Game/Source/UI/Elements/Text.h) を新しく作り、次の8行を書きます（1〜8行目）。

<!-- new: Action/Game/Source/UI/Elements/Text.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
```

</td><td>

```cpp
#pragma once

#include "UIElement.h"

class Text : public UIElement
{
public:
};
```

</td></tr></table>

### 2-2. 寄せ方の種類を書く

[Text.h](../../../Game/Source/UI/Elements/Text.h) の **7行目** `public:` の下、`};` の上に、7行足します（8〜14行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.h 8 -->
<table class="code-lines"><tr><td>

```cpp
8
9
10
11
12
13
14
```

</td><td>

```cpp
	enum class Align
	{
		Left,
		Center,
		Right,
	};

```

</td></tr></table>

### 2-3. 文字を決める関数を宣言する

[Text.h](../../../Game/Source/UI/Elements/Text.h) の **14行目**（`};` の下の空の行）の下、`};` の上に、3行足します（15〜17行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.h 15 -->
<table class="code-lines"><tr><td>

```cpp
15
16
17
```

</td><td>

```cpp
	// 値が変わったときだけ描画用の文字を書き換える。毎フレーム呼んでよい。
	// FontRender は文字を書式（swprintf の書式）として扱うので、% は %% と書く。
	void SetText(const wchar_t* text);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 17 | 文字が変わったときだけ書き換える。毎フレーム呼んでよい。`%` は `%%` と書く（書式として扱われるため） |

### 2-4. 色と大きさを決める関数を足す

[Text.h](../../../Game/Source/UI/Elements/Text.h) の **17行目** `void SetText(const wchar_t* text);` の下に、8行足します（18〜25行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.h 18 -->
<table class="code-lines"><tr><td>

```cpp
18
19
20
21
22
23
24
25
```

</td><td>

```cpp
	void SetColor(const Vector4& color)
	{
		m_color = color;
	}
	void SetScale(float scale)
	{
		m_scale = scale;
	}
```

</td></tr></table>

### 2-5. 寄せ方と影を決める関数を足す

[Text.h](../../../Game/Source/UI/Elements/Text.h) の **25行目** `}` の下、`};` の上に、10行足します（26〜35行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.h 26 -->
<table class="code-lines"><tr><td>

```cpp
26
27
28
29
30
31
32
33
34
35
```

</td><td>

```cpp
	void SetAlign(Align align)
	{
		m_align = align;
	}
	// 文字の周りに影（縁取り）を付ける。offset はずらすピクセル数。
	void SetShadow(float offset, const Vector4& color)
	{
		m_shadowOffset = offset;
		m_shadowColor = color;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 31〜35 | 文字の周りに影（縁取り）を付ける。`offset` はずらすピクセル数 |

### 2-6. 更新と、描く関数を宣言する

[Text.h](../../../Game/Source/UI/Elements/Text.h) の **35行目** `}` の下、`};` の上に、6行足します（36〜41行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.h 36 -->
<table class="code-lines"><tr><td>

```cpp
36
37
38
39
40
41
```

</td><td>

```cpp

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

```

</td></tr></table>

### 2-7. 持つ値を足す

[Text.h](../../../Game/Source/UI/Elements/Text.h) の **41行目**（`void OnDraw(RenderContext& rc) override;` の下の空の行）の下、`};` の上に、9行足します（42〜50行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.h 42 -->
<table class="code-lines"><tr><td>

```cpp
42
43
44
45
46
47
48
49
50
```

</td><td>

```cpp
private:
	FontRender m_font;
	std::wstring m_text;
	Vector2 m_textSize = Vector2::Zero;  // 大きさ 1 のときの文字の幅と高さ。SetText のときに測る。
	Vector4 m_color = g_vec4White;
	float m_scale = 1.0f;
	Align m_align = Align::Center;
	float m_shadowOffset = 0.0f;  // 0 なら影なし。
	Vector4 m_shadowColor = { 0.0f, 0.0f, 0.0f, 0.8f };
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 43 | エンジンの文字を描くもの |
| 45 | 大きさ 1 のときの文字の幅と高さ。文字を決めたときに測る |
| 49 | 影をずらすピクセル数。0 なら影なし |

### 2-8. コメントを書く

[Text.h](../../../Game/Source/UI/Elements/Text.h) の **4行目**（`#include "UIElement.h"` の下の空の行）の下、`class Text : public UIElement` の上に、1行足します（5〜5行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.h 5 -->
<table class="code-lines"><tr><td>

```cpp
5
```

</td><td>

```cpp
// 文字を出す。位置は、横は Align で決めた端（左端・中央・右端）、縦は文字の中央。
```

</td></tr></table>

### 2-9. エンジンに、文字の幅を測る関数を足す

エンジンの [FontEngine.h](../../../k2EngineLow/graphics/font/FontEngine.h) に足します。

[FontEngine.h](../../../k2EngineLow/graphics/font/FontEngine.h) の **52行目** `void EndDraw(RenderContext& rc);` の下に、9行足します（53〜61行目になります）。

<!-- add: Action/k2EngineLow/graphics/font/FontEngine.h 53 -->
<table class="code-lines"><tr><td>

```cpp
53
54
55
56
57
58
59
60
61
```

</td><td>

```cpp
		/// <summary>
		/// 文字列を大きさ 1 で描いたときの幅と高さ（ピクセル）を測る。右寄せ・中央寄せに使う。
		/// </summary>
		Vector2 MeasureString(const wchar_t* text) const
		{
			DirectX::XMFLOAT2 size;
			DirectX::XMStoreFloat2(&size, m_spriteFont->MeasureString(text));
			return Vector2(size.x, size.y);
		}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 56〜61 | 文字列を大きさ 1 で描いたときの幅と高さ（ピクセル）を、DirectXTK の `SpriteFont` で測る |

### 2-10. Text.cpp を作り、使うものを読み込む

[Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) を新しく作り、次の3行を書きます（1〜3行目）。

<!-- new: Action/Game/Source/UI/Elements/Text.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
```

</td><td>

```cpp
#include "stdafx.h"
#include "Text.h"

```

</td></tr></table>

### 2-11. 文字の位置の基準を決める

[Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) の **3行目**（`#include "Text.h"` の下の空の行）の下（ファイルの最後）に、7行足します（4〜10行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.cpp 4 -->
<table class="code-lines"><tr><td>

```cpp
4
5
6
7
8
9
10
```

</td><td>

```cpp
namespace
{
	// エンジンの Font は、pivot を割合ではなく「文字の左上からのピクセル数」として扱う（y は 1 - pivot.y になる）。
	// { 0, 1 } を渡すと、位置が文字の左上になる。寄せるのは、測った幅で位置をずらして行う。
	const Vector2 TOP_LEFT_PIVOT = { 0.0f, 1.0f };
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 6〜8 | エンジンの `Font` は、基準（pivot）を割合ではなく「文字の左上からのピクセル数」として扱う。`{ 0, 1 }` で、位置が文字の左上になる。寄せるのは、測った幅で位置をずらして行う |

### 2-12. 文字を決める関数を書く

[Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) の **10行目**（`}` の下の空の行）の下（ファイルの最後）に、7行足します（11〜17行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.cpp 11 -->
<table class="code-lines"><tr><td>

```cpp
11
12
13
14
15
16
17
```

</td><td>

```cpp
void Text::SetText(const wchar_t* text)
{
	m_text = text;
	m_font.SetText(m_text.c_str());
	m_textSize = m_text.empty() ? Vector2::Zero : g_graphicsEngine->GetFontEngine().MeasureString(m_text.c_str());
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13〜14 | 文字を覚え、描くものに渡す |
| 15 | 大きさ 1 のときの幅と高さを測る（2-9） |

### 2-13. 文字が変わっていなければ何もしない

[Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) の **12行目** `{` の下、`m_text = text;` の上に、6行足します（13〜18行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.cpp 13 -->
<table class="code-lines"><tr><td>

```cpp
13
14
15
16
17
18
```

</td><td>

```cpp
	if (text == nullptr) {
		text = L"";
	}
	if (m_text == text) {
		return;
	}
```

</td></tr></table>

### 2-14. 毎フレームの関数を書き、幅と高さを求める

[Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) の **23行目**（`}` の下の空の行）の下（ファイルの最後）に、7行足します（24〜30行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.cpp 24 -->
<table class="code-lines"><tr><td>

```cpp
24
25
26
27
28
29
30
```

</td><td>

```cpp
void Text::Update(float deltaTime)
{
	const float width = m_textSize.x * m_scale;
	const float height = m_textSize.y * m_scale;

}

```

</td></tr></table>

### 2-15. 寄せ方に合わせて、左端と上端を求める

[Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) の **28行目**（`const float height = m_textSize.y * m_scale;` の下の空の行）の下、`}` の上に、9行足します（29〜37行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.cpp 29 -->
<table class="code-lines"><tr><td>

```cpp
29
30
31
32
33
34
35
36
37
```

</td><td>

```cpp
	float left = m_position.x;
	if (m_align == Align::Center) {
		left -= width * 0.5f;
	}
	else if (m_align == Align::Right) {
		left -= width;
	}
	// 位置の y は文字の中央なので、上端は高さの半分だけ上。
	const float top = m_position.y + height * 0.5f;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 29〜35 | 位置の x は、寄せ方の端。中央なら幅の半分、右なら幅だけ左へずらして、左端にする |
| 37 | 位置の y は文字の中央なので、上端は高さの半分だけ上 |

### 2-16. 描くものに渡す

[Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) の **37行目** `const float top = m_position.y + height * 0.5f;` の下に、6行足します（38〜43行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.cpp 38 -->
<table class="code-lines"><tr><td>

```cpp
38
39
40
41
42
43
```

</td><td>

```cpp

	m_font.SetPivot(TOP_LEFT_PIVOT);
	m_font.SetScale(m_scale);
	m_font.SetPosition(left, top, 0.0f);
	m_font.SetColor(ApplyAlpha(m_color));
	m_font.SetShadowParam(m_shadowOffset > 0.0f, m_shadowOffset, ApplyAlpha(m_shadowColor));
```

</td></tr></table>

### 2-17. 描く中身の関数を書く

[Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) の **45行目**（`}` の下の空の行）の下（ファイルの最後）に、7行足します（46〜52行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Text.cpp 46 -->
<table class="code-lines"><tr><td>

```cpp
46
47
48
49
50
51
52
```

</td><td>

```cpp
void Text::OnDraw(RenderContext& rc)
{
	if (m_text.empty()) {
		return;
	}
	m_font.Draw(rc);
}
```

</td></tr></table>

### 出来上がった Text.h

<!-- full: Action/Game/Source/UI/Elements/Text.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
50
51
52
```

</td><td>

```cpp
#pragma once

#include "UIElement.h"

// 文字を出す。位置は、横は Align で決めた端（左端・中央・右端）、縦は文字の中央。
class Text : public UIElement
{
public:
	enum class Align
	{
		Left,
		Center,
		Right,
	};

	// 値が変わったときだけ描画用の文字を書き換える。毎フレーム呼んでよい。
	// FontRender は文字を書式（swprintf の書式）として扱うので、% は %% と書く。
	void SetText(const wchar_t* text);
	void SetColor(const Vector4& color)
	{
		m_color = color;
	}
	void SetScale(float scale)
	{
		m_scale = scale;
	}
	void SetAlign(Align align)
	{
		m_align = align;
	}
	// 文字の周りに影（縁取り）を付ける。offset はずらすピクセル数。
	void SetShadow(float offset, const Vector4& color)
	{
		m_shadowOffset = offset;
		m_shadowColor = color;
	}

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

private:
	FontRender m_font;
	std::wstring m_text;
	Vector2 m_textSize = Vector2::Zero;  // 大きさ 1 のときの文字の幅と高さ。SetText のときに測る。
	Vector4 m_color = g_vec4White;
	float m_scale = 1.0f;
	Align m_align = Align::Center;
	float m_shadowOffset = 0.0f;  // 0 なら影なし。
	Vector4 m_shadowColor = { 0.0f, 0.0f, 0.0f, 0.8f };
};
```

</td></tr></table>

### 出来上がった Text.cpp

<!-- full: Action/Game/Source/UI/Elements/Text.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
50
51
52
```

</td><td>

```cpp
#include "stdafx.h"
#include "Text.h"

namespace
{
	// エンジンの Font は、pivot を割合ではなく「文字の左上からのピクセル数」として扱う（y は 1 - pivot.y になる）。
	// { 0, 1 } を渡すと、位置が文字の左上になる。寄せるのは、測った幅で位置をずらして行う。
	const Vector2 TOP_LEFT_PIVOT = { 0.0f, 1.0f };
}

void Text::SetText(const wchar_t* text)
{
	if (text == nullptr) {
		text = L"";
	}
	if (m_text == text) {
		return;
	}
	m_text = text;
	m_font.SetText(m_text.c_str());
	m_textSize = m_text.empty() ? Vector2::Zero : g_graphicsEngine->GetFontEngine().MeasureString(m_text.c_str());
}

void Text::Update(float deltaTime)
{
	const float width = m_textSize.x * m_scale;
	const float height = m_textSize.y * m_scale;

	float left = m_position.x;
	if (m_align == Align::Center) {
		left -= width * 0.5f;
	}
	else if (m_align == Align::Right) {
		left -= width;
	}
	// 位置の y は文字の中央なので、上端は高さの半分だけ上。
	const float top = m_position.y + height * 0.5f;

	m_font.SetPivot(TOP_LEFT_PIVOT);
	m_font.SetScale(m_scale);
	m_font.SetPosition(left, top, 0.0f);
	m_font.SetColor(ApplyAlpha(m_color));
	m_font.SetShadowParam(m_shadowOffset > 0.0f, m_shadowOffset, ApplyAlpha(m_shadowColor));
}

void Text::OnDraw(RenderContext& rc)
{
	if (m_text.empty()) {
		return;
	}
	m_font.Draw(rc);
}
```

</td></tr></table>

**確かめ方:** ソリューションのビルドが通る。

---

## 手順3: 画像の部品 Icon と、画像のパス UISprite を作る

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [UISprite.h](../../../Game/Source/UI/Elements/UISprite.h) | 新しく作る（UI で使う画像のパス） |
| [Icon.h](../../../Game/Source/UI/Elements/Icon.h) | 新しく作る（画像を1枚出す） |
| [Icon.cpp](../../../Game/Source/UI/Elements/Icon.cpp) | 新しく作る（位置と色を渡して描く） |

### 3-1. UI で使う画像のパスを書く

[UISprite.h](../../../Game/Source/UI/Elements/UISprite.h) を新しく作り、次の10行を書きます（1〜10行目）。

<!-- new: Action/Game/Source/UI/Elements/UISprite.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
```

</td><td>

```cpp
#pragma once

// UI の部品で使う画像。
namespace UISprite
{
	// 4×4 の白い画像。SetMulColor で色を付けて、ゲージや仮のアイコンに使う。
	constexpr const char* WHITE_PATH = "Assets/sprite/white.dds";
	// 4×16 の、上が白く下へ少し暗くなる画像。ゲージの中身に使うと、上に光が当たったような立体感が出る。
	constexpr const char* GAUGE_PATH = "Assets/sprite/gauge.dds";
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 7 | 4×4 の白い画像。色を掛けて、ゲージや仮のアイコンに使う |
| 9 | 上が白く下へ少し暗くなる画像。ゲージの中身に使う |

### 3-2. Icon クラスの形を書く

[Icon.h](../../../Game/Source/UI/Elements/Icon.h) を新しく作り、次の9行を書きます（1〜9行目）。

<!-- new: Action/Game/Source/UI/Elements/Icon.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
```

</td><td>

```cpp
#pragma once

#include "UIElement.h"

class Icon : public UIElement
{
public:
	void Init(const char* filePath, float width, float height);
};
```

</td></tr></table>

### 3-3. 色を決める関数と、更新の関数を足す

[Icon.h](../../../Game/Source/UI/Elements/Icon.h) の **8行目** `void Init(const char* filePath, float width, float height);` の下に、7行足します（9〜15行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Icon.h 9 -->
<table class="code-lines"><tr><td>

```cpp
9
10
11
12
13
14
15
```

</td><td>

```cpp
	void SetColor(const Vector4& color)
	{
		m_color = color;
	}

	void Update(float deltaTime) override;

```

</td></tr></table>

### 3-4. 描く関数と、持つ値を足す

[Icon.h](../../../Game/Source/UI/Elements/Icon.h) の **15行目**（`void Update(float deltaTime) override;` の下の空の行）の下、`};` の上に、6行足します（16〜21行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Icon.h 16 -->
<table class="code-lines"><tr><td>

```cpp
16
17
18
19
20
21
```

</td><td>

```cpp
protected:
	void OnDraw(RenderContext& rc) override;

private:
	SpriteRender m_sprite;
	Vector4 m_color = g_vec4White;
```

</td></tr></table>

### 3-5. コメントを書く

[Icon.h](../../../Game/Source/UI/Elements/Icon.h) の **4行目**（`#include "UIElement.h"` の下の空の行）の下、`class Icon : public UIElement` の上に、2行足します（5〜6行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Icon.h 5 -->
<table class="code-lines"><tr><td>

```cpp
5
6
```

</td><td>

```cpp
// 画像を1枚出す（状態異常のマーク、魔法の絵など）。位置は画像の中央。
// 画像がまだないものは、白い画像（UISprite::WHITE_PATH）に色を付けて四角で出す。
```

</td></tr></table>

### 3-6. Icon.cpp を作り、準備の関数を書く

[Icon.cpp](../../../Game/Source/UI/Elements/Icon.cpp) を新しく作り、次の8行を書きます（1〜8行目）。

<!-- new: Action/Game/Source/UI/Elements/Icon.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
```

</td><td>

```cpp
#include "stdafx.h"
#include "Icon.h"

void Icon::Init(const char* filePath, float width, float height)
{
	m_sprite.Init(filePath, width, height);
}

```

</td></tr></table>

### 3-7. 更新と、描く関数を書く

[Icon.cpp](../../../Game/Source/UI/Elements/Icon.cpp) の **8行目**（`}` の下の空の行）の下（ファイルの最後）に、11行足します（9〜19行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Icon.cpp 9 -->
<table class="code-lines"><tr><td>

```cpp
9
10
11
12
13
14
15
16
17
18
19
```

</td><td>

```cpp
void Icon::Update(float deltaTime)
{
	m_sprite.SetPosition(Vector3(m_position.x, m_position.y, 0.0f));
	m_sprite.SetMulColor(ApplyAlpha(m_color));
	m_sprite.Update();
}

void Icon::OnDraw(RenderContext& rc)
{
	m_sprite.Draw(rc);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11〜13 | 位置と、透明度を掛けた色を渡して、更新する |

### 出来上がった UISprite.h

<!-- full: Action/Game/Source/UI/Elements/UISprite.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
```

</td><td>

```cpp
#pragma once

// UI の部品で使う画像。
namespace UISprite
{
	// 4×4 の白い画像。SetMulColor で色を付けて、ゲージや仮のアイコンに使う。
	constexpr const char* WHITE_PATH = "Assets/sprite/white.dds";
	// 4×16 の、上が白く下へ少し暗くなる画像。ゲージの中身に使うと、上に光が当たったような立体感が出る。
	constexpr const char* GAUGE_PATH = "Assets/sprite/gauge.dds";
}
```

</td></tr></table>

### 出来上がった Icon.h

<!-- full: Action/Game/Source/UI/Elements/Icon.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
```

</td><td>

```cpp
#pragma once

#include "UIElement.h"

// 画像を1枚出す（状態異常のマーク、魔法の絵など）。位置は画像の中央。
// 画像がまだないものは、白い画像（UISprite::WHITE_PATH）に色を付けて四角で出す。
class Icon : public UIElement
{
public:
	void Init(const char* filePath, float width, float height);
	void SetColor(const Vector4& color)
	{
		m_color = color;
	}

	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

private:
	SpriteRender m_sprite;
	Vector4 m_color = g_vec4White;
};
```

</td></tr></table>

### 出来上がった Icon.cpp

<!-- full: Action/Game/Source/UI/Elements/Icon.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
```

</td><td>

```cpp
#include "stdafx.h"
#include "Icon.h"

void Icon::Init(const char* filePath, float width, float height)
{
	m_sprite.Init(filePath, width, height);
}

void Icon::Update(float deltaTime)
{
	m_sprite.SetPosition(Vector3(m_position.x, m_position.y, 0.0f));
	m_sprite.SetMulColor(ApplyAlpha(m_color));
	m_sprite.Update();
}

void Icon::OnDraw(RenderContext& rc)
{
	m_sprite.Draw(rc);
}
```

</td></tr></table>

**確かめ方:** ソリューションのビルドが通る。

---

## 手順4: UIManager を作り、Game から作る

画面の UI をまとめて持ち、毎フレーム更新して描きます。UI はゲームを読むだけで、ゲーム側は UI のことを知りません。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 新しく作る（UI をまとめて持つ） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 新しく作る（まだ何もしない） |
| [Game.h](../../../Game/Source/Core/Game.h) | 追加（前方宣言と、メンバー） |
| [Game.cpp](../../../Game/Source/Core/Game.cpp) | 追加（`#include`、作る、消す）・書き換え（優先度のコメント） |

`Source\UI` フィルターに `UIManager.h` と `UIManager.cpp` を作ります。

### 4-1. UIManager クラスの形を書く

[UIManager.h](../../../Game/Source/UI/UIManager.h) を新しく作り、次の9行を書きます（1〜9行目）。

<!-- new: Action/Game/Source/UI/UIManager.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
```

</td><td>

```cpp
#pragma once

class UIManager : public IGameObject
{
public:
	bool Start() override;
	void Update() override;
	void Render(RenderContext& rc) override;
};
```

</td></tr></table>

### 4-2. コメントを書く

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **2行目**（`#pragma once` の下の空の行）の下、`class UIManager : public IGameObject` の上に、4行足します（3〜6行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 3 -->
<table class="code-lines"><tr><td>

```cpp
3
4
5
6
```

</td><td>

```cpp
// 画面の UI をまとめて持ち、毎フレーム更新して描く。ゲームに1つだけ。
// UI はゲームを読むだけで、ゲーム側は UI のことを知らない。UIManager に話しかけるのは Game だけにする
// （シングルトンにしない。キャラクターが自分で UI を呼べると、ゲームが UI を知ることになるため）。
// キャラクターが動いたあとの値を出すため、NewGO の優先度はキャラクター（1）・カメラ（2）より後の 3 にする。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 5 | シングルトンにしない。キャラクターが自分で UI を呼べると、ゲームが UI を知ることになるため |
| 6 | キャラクターが動いたあとの値を出すため、優先度はキャラクター（1）・カメラ（2）より後にする |

### 4-3. UIManager.cpp を作り、最初の関数を書く

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) を新しく作り、次の8行を書きます（1〜8行目）。

<!-- new: Action/Game/Source/UI/UIManager.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
```

</td><td>

```cpp
#include "stdafx.h"
#include "UIManager.h"

bool UIManager::Start()
{
	return true;
}

```

</td></tr></table>

### 4-4. 毎フレームの関数と、描く関数を書く

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **8行目**（`}` の下の空の行）の下（ファイルの最後）に、9行足します（9〜17行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 9 -->
<table class="code-lines"><tr><td>

```cpp
9
10
11
12
13
14
15
16
17
```

</td><td>

```cpp
void UIManager::Update()
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
}

void UIManager::Render(RenderContext& rc)
{
	// 後に描いたものが上に重なる。
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11 | 経過時間。部品の `Update()` に渡す（[HUDの手順](HUDの手順.md) で使う） |
| 16 | あとに描いたものが上に重なる |

### 4-5. Game.h に UIManager があることを知らせる

[Game.h](../../../Game/Source/Core/Game.h) の **7行目** `class FireEffect;` の下に、1行足します（8〜8行目になります）。

<!-- add: Action/Game/Source/Core/Game.h 8 -->
<table class="code-lines"><tr><td>

```cpp
8
```

</td><td>

```cpp
class UIManager;
```

</td></tr></table>

### 4-6. UIManager を持つメンバーを足す

[Game.h](../../../Game/Source/Core/Game.h) の **30行目** `FireEffect* m_fireEffect = nullptr;` の下に、1行足します（31〜31行目になります）。

<!-- add: Action/Game/Source/Core/Game.h 31 -->
<table class="code-lines"><tr><td>

```cpp
31
```

</td><td>

```cpp
	UIManager* m_uiManager = nullptr;
```

</td></tr></table>

### 4-7. UIManager.h を読み込む

[Game.cpp](../../../Game/Source/Core/Game.cpp) の **10行目** `#include "BurningGround.h"` の下に、1行足します（11〜11行目になります）。

<!-- add: Action/Game/Source/Core/Game.cpp 11 -->
<table class="code-lines"><tr><td>

```cpp
11
```

</td><td>

```cpp
#include "UIManager.h"
```

</td></tr></table>

### 4-8. 終わるときに、UI を先に消す

[Game.cpp](../../../Game/Source/Core/Game.cpp) の **31行目** `{` の下、`DeleteGO(m_stage);` の上に、2行足します（32〜33行目になります）。

<!-- add: Action/Game/Source/Core/Game.cpp 32 -->
<table class="code-lines"><tr><td>

```cpp
32
33
```

</td><td>

```cpp
	// UI はキャラクターを読んでいるので、キャラクターより先に消す。
	DeleteGO(m_uiManager);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 32〜33 | UI はキャラクターを読んでいるので、キャラクターより先に消す |

### 4-9. 優先度のコメントを書き換える

[Game.cpp](../../../Game/Source/Core/Game.cpp) の **67行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/Game/Source/Core/Game.cpp 67 -->
<table class="code-lines"><tr><td>

```cpp
67
```

</td><td>

```cpp
	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ。
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/Game/Source/Core/Game.cpp 67 -->
<table class="code-lines"><tr><td>

```cpp
67
```

</td><td>

```cpp
	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ、優先度3=UI（キャラが動いたあとの値を出す）。
```

</td></tr></table>

### 4-10. Start() で UIManager を作る

[Game.cpp](../../../Game/Source/Core/Game.cpp) の **76行目** `m_fireEffect->SetScale(FIRE_SCALE);` の下に、1行足します（77〜77行目になります）。

<!-- add: Action/Game/Source/Core/Game.cpp 77 -->
<table class="code-lines"><tr><td>

```cpp
77
```

</td><td>

```cpp
	m_uiManager = NewGO<UIManager>(3, "uiManager");
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 77 | 優先度 3 で作る。キャラクターとカメラが動いたあとに更新する |

### 出来上がった UIManager.h

<!-- full: Action/Game/Source/UI/UIManager.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
```

</td><td>

```cpp
#pragma once

// 画面の UI をまとめて持ち、毎フレーム更新して描く。ゲームに1つだけ。
// UI はゲームを読むだけで、ゲーム側は UI のことを知らない。UIManager に話しかけるのは Game だけにする
// （シングルトンにしない。キャラクターが自分で UI を呼べると、ゲームが UI を知ることになるため）。
// キャラクターが動いたあとの値を出すため、NewGO の優先度はキャラクター（1）・カメラ（2）より後の 3 にする。
class UIManager : public IGameObject
{
public:
	bool Start() override;
	void Update() override;
	void Render(RenderContext& rc) override;
};
```

</td></tr></table>

### 出来上がった UIManager.cpp

<!-- full: Action/Game/Source/UI/UIManager.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
```

</td><td>

```cpp
#include "stdafx.h"
#include "UIManager.h"

bool UIManager::Start()
{
	return true;
}

void UIManager::Update()
{
	const float deltaTime = g_gameTime->GetFrameDeltaTime();
}

void UIManager::Render(RenderContext& rc)
{
	// 後に描いたものが上に重なる。
}
```

</td></tr></table>

**確かめ方（これで出来上がり）:** ソリューションのビルドが通り、起動して落ちない（まだ何も描かないので、画面は変わらない）。
