# UIの構成の手順

作成日: 2026-11-08
設計: [UIの構成の設計](UIの構成の設計.md)
経緯: [UIの経緯](UIの経緯.md)
確かめた記録: [ゲームの手順書の経緯](../土台/ゲームの手順書の経緯.md)

画面に出す UI の部品（`UIElement` と、文字の `Text`・画像の `Icon`）と、UI をまとめて持つ `UIManager` を作るまでの手順です。上から順にやると、[UIの構成の設計](UIの構成の設計.md) の形のうち、HUD（[HUDの手順](HUDの手順.md)）と FPS の表示（[FPSの表示の手順](FPSの表示の手順.md)）を除いた部分ができます。

**出来上がるもの:** `Game` が `UIManager` を作り、毎フレーム更新して描く流れができます。まだ何も描かないので、画面は変わりません。HUD の手順で、ここで作った部品を使って HP ゲージなどを出します。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- ゲームの手順書は [README の「ゲームを作る順番」](../../README.md#ゲームを作る順番) の順に進める。この手順の前に、[燃える床の手順](../魔法/燃える床の手順.md) まで済ませておく。
- 作業を始める前に、main からブランチを切る。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | 部品の基底クラス `UIElement` | ビルドが通る |
| 2 | 文字の部品 `Text`（エンジンに文字の幅を測る関数を足す） | ビルドが通る |
| 3 | 画像の部品 `Icon` と、画像のパス `UISprite` | ビルドが通る |
| 4 | `UIManager` を作り、`Game` から作る | 起動して落ちない |

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加 | 1・2・3・4 |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加 | 1・2・3・4 |
| [UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) | 新しく作る | 1 |
| [Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) | 新しく作る | 2 |
| [Text.h](../../../Game/Source/UI/Elements/Text.h) | 新しく作る | 2 |
| [FontEngine.h](../../../k2EngineLow/graphics/font/FontEngine.h) | 追加 | 2 |
| [Icon.cpp](../../../Game/Source/UI/Elements/Icon.cpp) | 新しく作る | 3 |
| [Icon.h](../../../Game/Source/UI/Elements/Icon.h) | 新しく作る | 3 |
| [UISprite.h](../../../Game/Source/UI/Elements/UISprite.h) | 新しく作る | 3 |
| [Game.cpp](../../../Game/Source/Core/Game.cpp) | 追加・書き換え | 4 |
| [Game.h](../../../Game/Source/Core/Game.h) | 追加 | 4 |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 新しく作る | 4 |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 新しく作る | 4 |

消すファイル・コメントアウトする行はありません。

`Game.vcxproj` と `Game.vcxproj.filters` は、Visual Studio でファイルやフィルターを足すと自動で書き換わります。手で書く必要はありませんが、何が変わるかがわかるように diff を載せています。

新しく作る `.h` / `.cpp` は、すべて UTF-8（BOM 付き）で保存します。

---

## 手順1: 部品の基底クラス UIElement を作る

画面に出る部品（ゲージ・アイコン・文字）に共通の「位置」「表示する／しない」「透明度」を、基底クラス `UIElement` にまとめます。部品は言われた値を描くだけで、キャラクターのことは知りません。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（1行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（フィルター2つと、ファイル1つ。自動で入る） |
| [UIElement.h](../../../Game/Source/UI/Elements/UIElement.h) | 新しく作る（位置・表示する／しない・透明度と、`Update()`・`Draw()`） |

### 1-1. フィルターとファイルを作る

ソリューションエクスプローラーの `Game` の `Source` フィルターを右クリック →「追加 → 新しいフィルター」で `UI` を作り、その下に `Elements` を作ります。`Elements` に `UIElement.h` を作ります（「追加 → 新しい項目」→「ヘッダー ファイル」）。**場所は `Action/Game/Source/UI/Elements/` にする。**

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の1行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\Player\PlayerAnimator.h" />
     <ClInclude Include="Source\Enemy\Enemy.h" />
+    <ClInclude Include="Source\UI\Elements\UIElement.h" />
     <ClInclude Include="Source\Objects\FireEffect.h" />
     <ClInclude Include="Source\Character\Input\PlayerInputSource.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、フィルター2つとファイル1つが**追加**されます（`UniqueIdentifier` の値は、作るたびに変わります）。

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <UniqueIdentifier>{22dd8095-74f9-46df-81a1-36675a040b1f}</UniqueIdentifier>
     </Filter>
+    <Filter Include="Source\UI">
+      <UniqueIdentifier>{6c2e8f14-3a7b-4d59-9e01-8b4f2c7d5a36}</UniqueIdentifier>
+    </Filter>
+    <Filter Include="Source\UI\Elements">
+      <UniqueIdentifier>{b83a5e60-2d1f-4c97-8a4e-5f07d9c2e1b3}</UniqueIdentifier>
+    </Filter>
   </ItemGroup>
   <ItemGroup>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\Enemy</Filter>
     </ClInclude>
+    <ClInclude Include="Source\UI\Elements\UIElement.h">
+      <Filter>Source\UI\Elements</Filter>
+    </ClInclude>
     <ClInclude Include="Source\Objects\FireEffect.h">
       <Filter>Source\Objects</Filter>
```

### 1-2. UIElement.h を書く

<!-- new: ../../../Game/Source/UI/Elements/UIElement.h -->

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

- **`Draw()` は派生クラスで変えない。** 見えないとき（`SetVisible(false)` か透明度 0）はここで止め、見えるときだけ派生クラスの `OnDraw()` を呼びます。
- **透明度は `ApplyAlpha()` で色に掛ける。** 部品ごとにフェードの処理を書かずに済みます。
- 座標は画面の中心が原点で、右が +x、上が +y です（画面は `UI_SPACE_WIDTH` × `UI_SPACE_HEIGHT` = 1920 × 1080）。

**確かめ方:** ソリューションのビルドが通る（まだどこからも使っていない）。

---

## 手順2: 文字の部品 Text を作る

文字を出す部品です。位置の横を左端・中央・右端のどれに合わせるか（`Align`）を選べるように、描く前に文字の幅を測ります。幅を測る関数がエンジンにないので、先にエンジンの `FontEngine` に足します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（2行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（6行。自動で入る） |
| [Text.cpp](../../../Game/Source/UI/Elements/Text.cpp) | 新しく作る（`SetText()`・`Update()`・`OnDraw()`） |
| [Text.h](../../../Game/Source/UI/Elements/Text.h) | 新しく作る（文字・色・大きさ・寄せ方・影） |
| [FontEngine.h](../../../k2EngineLow/graphics/font/FontEngine.h) | 追加（文字の幅と高さを測る `MeasureString()`） |

### 2-1. エンジンに文字の幅を測る関数を足す

[FontEngine.h](../../../k2EngineLow/graphics/font/FontEngine.h) の `EndDraw()` の宣言の下（`private:` の上）に**追加**します。DirectXTK の `SpriteFont::MeasureString()` が、大きさ 1 で描いたときの幅と高さ（ピクセル）を返します。

<!-- file: ../../../k2EngineLow/graphics/font/FontEngine.h -->

```diff
 		/// </summary>
 		void EndDraw(RenderContext& rc);
+		/// <summary>
+		/// 文字列を大きさ 1 で描いたときの幅と高さ（ピクセル）を測る。右寄せ・中央寄せに使う。
+		/// </summary>
+		Vector2 MeasureString(const wchar_t* text) const
+		{
+			DirectX::XMFLOAT2 size;
+			DirectX::XMStoreFloat2(&size, m_spriteFont->MeasureString(text));
+			return Vector2(size.x, size.y);
+		}
 	private:
 		std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;	//スプライトバッチ。
```

### 2-2. ファイルを作る

`Source\UI\Elements` フィルターに `Text.h` と `Text.cpp` を作ります。場所は `Action/Game/Source/UI/Elements/`。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の2行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClCompile Include="Source\Player\PlayerAnimator.cpp" />
     <ClCompile Include="Source\Enemy\Enemy.cpp" />
+    <ClCompile Include="Source\UI\Elements\Text.cpp" />
     <ClCompile Include="Source\Objects\FireEffect.cpp" />
     <ClCompile Include="Source\Character\Input\PlayerInputSource.cpp" />
```

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\Enemy\Enemy.h" />
     <ClInclude Include="Source\UI\Elements\UIElement.h" />
+    <ClInclude Include="Source\UI\Elements\Text.h" />
     <ClInclude Include="Source\Objects\FireEffect.h" />
     <ClInclude Include="Source\Character\Input\PlayerInputSource.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、次の6行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\Enemy</Filter>
     </ClCompile>
+    <ClCompile Include="Source\UI\Elements\Text.cpp">
+      <Filter>Source\UI\Elements</Filter>
+    </ClCompile>
     <ClCompile Include="Source\Objects\FireEffect.cpp">
       <Filter>Source\Objects</Filter>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI\Elements</Filter>
     </ClInclude>
+    <ClInclude Include="Source\UI\Elements\Text.h">
+      <Filter>Source\UI\Elements</Filter>
+    </ClInclude>
     <ClInclude Include="Source\Objects\FireEffect.h">
       <Filter>Source\Objects</Filter>
```

### 2-3. Text.h を書く

<!-- new: ../../../Game/Source/UI/Elements/Text.h -->

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

### 2-4. Text.cpp を書く

<!-- new: ../../../Game/Source/UI/Elements/Text.cpp -->

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

- **文字が変わったときだけ書き換えて測る。** `SetText()` は毎フレーム呼んでよく、同じ文字なら何もしません。
- **エンジンの `Font` の pivot は、割合ではなく「文字の左上からのピクセル数」。** そのため pivot は `{ 0, 1 }`（左上）に固定し、測った幅で位置をずらして寄せます。

**確かめ方:** ソリューションのビルドが通る（k2EngineLow もビルドし直される）。

---

## 手順3: 画像の部品 Icon と、画像のパス UISprite を作る

画像を1枚出す部品です。状態異常のマークなどに使います。画像がまだないものは、白い 4×4 の画像に色を掛けて四角で出すので、その画像のパスを `UISprite` にまとめておきます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（3行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（9行。自動で入る） |
| [Icon.cpp](../../../Game/Source/UI/Elements/Icon.cpp) | 新しく作る（`Init()`・`Update()`・`OnDraw()`） |
| [Icon.h](../../../Game/Source/UI/Elements/Icon.h) | 新しく作る（画像・色） |
| [UISprite.h](../../../Game/Source/UI/Elements/UISprite.h) | 新しく作る（白い画像とゲージの画像のパス） |

### 3-1. ファイルを作る

`Source\UI\Elements` フィルターに `Icon.h`・`Icon.cpp`・`UISprite.h` を作ります。場所は `Action/Game/Source/UI/Elements/`。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の3行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClCompile Include="Source\Player\PlayerAnimator.cpp" />
     <ClCompile Include="Source\Enemy\Enemy.cpp" />
+    <ClCompile Include="Source\UI\Elements\Icon.cpp" />
     <ClCompile Include="Source\UI\Elements\Text.cpp" />
     <ClCompile Include="Source\Objects\FireEffect.cpp" />
```

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\Enemy\Enemy.h" />
     <ClInclude Include="Source\UI\Elements\UIElement.h" />
+    <ClInclude Include="Source\UI\Elements\UISprite.h" />
+    <ClInclude Include="Source\UI\Elements\Icon.h" />
     <ClInclude Include="Source\UI\Elements\Text.h" />
     <ClInclude Include="Source\Objects\FireEffect.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、次の9行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\Enemy</Filter>
     </ClCompile>
+    <ClCompile Include="Source\UI\Elements\Icon.cpp">
+      <Filter>Source\UI\Elements</Filter>
+    </ClCompile>
     <ClCompile Include="Source\UI\Elements\Text.cpp">
       <Filter>Source\UI\Elements</Filter>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI\Elements</Filter>
     </ClInclude>
+    <ClInclude Include="Source\UI\Elements\UISprite.h">
+      <Filter>Source\UI\Elements</Filter>
+    </ClInclude>
+    <ClInclude Include="Source\UI\Elements\Icon.h">
+      <Filter>Source\UI\Elements</Filter>
+    </ClInclude>
     <ClInclude Include="Source\UI\Elements\Text.h">
       <Filter>Source\UI\Elements</Filter>
```

### 3-2. UISprite.h を書く

`Assets/sprite/white.dds`（4×4 の白）と `Assets/sprite/gauge.dds`（4×16 の、上が白く下へ少し暗くなる画像）は、リポジトリにもとからある（変えない）。

<!-- new: ../../../Game/Source/UI/Elements/UISprite.h -->

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

### 3-3. Icon.h を書く

<!-- new: ../../../Game/Source/UI/Elements/Icon.h -->

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

### 3-4. Icon.cpp を書く

<!-- new: ../../../Game/Source/UI/Elements/Icon.cpp -->

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

**確かめ方:** ソリューションのビルドが通る。

---

## 手順4: UIManager を作り、Game から作る

画面の UI をまとめて持ち、毎フレーム更新して描く `UIManager` を作ります。UI に話しかけるのは `Game` だけにするので、シングルトンにはせず、`Game` が作って持ちます。キャラクターが動いたあとの値を出すため、`NewGO` の優先度はキャラクター（1）・カメラ（2）より後の 3 にします。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（2行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（6行。自動で入る） |
| [Game.cpp](../../../Game/Source/Core/Game.cpp) | 追加・書き換え（`#include`、`Start()` で作る、デストラクターで消す。優先度のコメントも書き換える） |
| [Game.h](../../../Game/Source/Core/Game.h) | 追加（前方宣言と、メンバー `m_uiManager`） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 新しく作る（中身はまだ空） |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 新しく作る（`Start()`・`Update()`・`Render()` だけのクラス） |

### 4-1. ファイルを作る

`Source\UI` フィルターに `UIManager.h` と `UIManager.cpp` を作ります。場所は `Action/Game/Source/UI/`。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の2行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClCompile Include="Source\Player\PlayerAnimator.cpp" />
     <ClCompile Include="Source\Enemy\Enemy.cpp" />
+    <ClCompile Include="Source\UI\UIManager.cpp" />
     <ClCompile Include="Source\UI\Elements\Icon.cpp" />
     <ClCompile Include="Source\UI\Elements\Text.cpp" />
```

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\Player\PlayerAnimator.h" />
     <ClInclude Include="Source\Enemy\Enemy.h" />
+    <ClInclude Include="Source\UI\UIManager.h" />
     <ClInclude Include="Source\UI\Elements\UIElement.h" />
     <ClInclude Include="Source\UI\Elements\UISprite.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、次の6行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\Enemy</Filter>
     </ClCompile>
+    <ClCompile Include="Source\UI\UIManager.cpp">
+      <Filter>Source\UI</Filter>
+    </ClCompile>
     <ClCompile Include="Source\UI\Elements\Icon.cpp">
       <Filter>Source\UI\Elements</Filter>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\Enemy</Filter>
     </ClInclude>
+    <ClInclude Include="Source\UI\UIManager.h">
+      <Filter>Source\UI</Filter>
+    </ClInclude>
     <ClInclude Include="Source\UI\Elements\UIElement.h">
       <Filter>Source\UI\Elements</Filter>
```

### 4-2. UIManager.h を書く

まだ何も持たず、`IGameObject` の3つの関数だけを書きます。HUD の手順で、ここにメンバーを足していきます。

<!-- new: ../../../Game/Source/UI/UIManager.h -->

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

### 4-3. UIManager.cpp を書く

<!-- new: ../../../Game/Source/UI/UIManager.cpp -->

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

`Update()` の `deltaTime` は、HUD の手順で部品の `Update()` に渡します。

### 4-4. Game から作って消す

[Game.h](../../../Game/Source/Core/Game.h) に、前方宣言とメンバーを**追加**します。

<!-- file: ../../../Game/Source/Core/Game.h -->

```diff
 class Stage;
 class FireEffect;
+class UIManager;
 
 namespace nsK2Engine {
```

<!-- file: ../../../Game/Source/Core/Game.h -->

```diff
 	GameCamera* m_gameCamera = nullptr;
 	FireEffect* m_fireEffect = nullptr;
+	UIManager* m_uiManager = nullptr;
 	nsK2Engine::SkyCube* m_skyCube = nullptr;  // IBLの環境マップ生成も兼ねる。
 };
```

[Game.cpp](../../../Game/Source/Core/Game.cpp) に、`#include`、デストラクターで消す処理、`Start()` で作る処理を**追加**します。UI はキャラクターの値を読むので、キャラクターより先に消します。`Start()` の優先度のコメントにも、UI（3）を書き足します（書き換え）。

<!-- file: ../../../Game/Source/Core/Game.cpp -->

```diff
 #include "BulletBase.h"
 #include "BurningGround.h"
+#include "UIManager.h"
 
 namespace
```

<!-- file: ../../../Game/Source/Core/Game.cpp -->

```diff
 Game::~Game()
 {
+	// UI はキャラクターを読んでいるので、キャラクターより先に消す。
+	DeleteGO(m_uiManager);
 	DeleteGO(m_stage);
 	DeleteGO(m_player);
```

<!-- file: ../../../Game/Source/Core/Game.cpp -->

```diff
 	g_camera3D->SetFar(40000.0f);
 
-	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ。
+	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ、優先度3=UI（キャラが動いたあとの値を出す）。
 	m_stage = NewGO<Stage>(0, "stage");
 	m_player = NewGO<Player>(1, "player");
```

<!-- file: ../../../Game/Source/Core/Game.cpp -->

```diff
 	m_fireEffect->SetPosition(FIRE_POSITION);
 	m_fireEffect->SetScale(FIRE_SCALE);
+	m_uiManager = NewGO<UIManager>(3, "uiManager");
 
 	// SkyCubeのテクスチャをIBLの環境マップとしても使い回す。
```

**確かめ方（これで出来上がり）:**

- ソリューションのビルドが通る。
- ゲームを起動して、いつもどおり動く。ウィンドウを閉じても落ちない（まだ UI は何も出ない）。
