# FPSの表示の手順

作成日: 2026-11-08
設計: [FPSの表示の設計](FPSの表示の設計.md)
経緯: [UIの経緯](UIの経緯.md)
確かめた記録: [ゲームの手順書の経緯](../土台/ゲームの手順書の経緯.md)

画面の右上に、0.5秒ごとの平均の FPS を出すまでの手順です。上から順にやると、[FPSの表示の設計](FPSの表示の設計.md) の形になります。

**出来上がるもの:** Debug・Preview・Release のどのビルドでも、ゲームの画面の右上に `FPS 60.0` のように小さく出ます。エンジンが Debug ビルドだけ左上に出していた `FPS = xx.xx` は止めます。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [UIの構成の手順](UIの構成の手順.md) と [HUDの手順](HUDの手順.md) を済ませておく。この手順では、そこで作った `UIManager` と文字の部品 `Text`、エンジンの `FontEngine::MeasureString()` を使う。
- 作業を始める前に、main からブランチを切る。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | `DebugHud` を作り、右上に文字を置く | ビルドが通る |
| 2 | `UIManager` に持たせて描く | 右上に「FPS --」と出る |
| 3 | 0.5秒ごとに平均の FPS を数えて出す | 右上の数字が動く |
| 4 | エンジンが出していた FPS を止める | エンジンの FPS が消える |

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加 | 1 |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加 | 1 |
| [DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) | 新しく作る・追加 | 1・3 |
| [DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) | 新しく作る・追加 | 1・3 |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加・書き換え | 2・3 |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加 | 2 |
| [Font.cpp](../../../k2EngineLow/graphics/font/Font.cpp) | 追加 | 4 |
| [Font.h](../../../k2EngineLow/graphics/font/Font.h) | 追加 | 4 |
| [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) | コメントアウト・書き換え | 4 |

消すファイルはありません。手順4で、エンジンの [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の FPS を描く処理をコメントアウトします。

`Game.vcxproj` と `Game.vcxproj.filters` は、Visual Studio でファイルを足すと自動で書き換わります。手で書く必要はありませんが、何が変わるかがわかるように diff を載せています。

---

## 手順1: DebugHud を作り、右上に文字を置く

開発用の表示は、PlayerHud・BossHud のようなゲームの HUD とは分けて `DebugHud` にまとめます。まずは文字の部品 `Text` を1つ持ち、右上に置いて描くところまで作ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（2行。Visual Studio で足すと自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（6行。自動で入る） |
| [DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) | 新しく作る（置き場所・大きさ・色の定数と、`Init()`・`Draw()`） |
| [DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) | 新しく作る（`Init()`・`Draw()` と、文字の `m_fps`） |

### 1-1. ファイルを作る

ソリューションエクスプローラーの `Game` の `Source\UI\Hud` フィルターを右クリック →「追加 → 新しい項目」で、`DebugHud.h` と `DebugHud.cpp` を作ります。**場所は `Action/Game/Source/UI/Hud/` にする。** 保存は UTF-8（BOM 付き）。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の2行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClCompile Include="Source\UI\Hud\PlayerHud.cpp" />
     <ClCompile Include="Source\UI\Hud\BossHud.cpp" />
+    <ClCompile Include="Source\UI\Hud\DebugHud.cpp" />
     <ClCompile Include="Source\UI\Elements\Gauge.cpp" />
     <ClCompile Include="Source\UI\Elements\Icon.cpp" />
```

<!-- file: ../../../Game/Game.vcxproj -->

```diff
     <ClInclude Include="Source\UI\Hud\PlayerHud.h" />
     <ClInclude Include="Source\UI\Hud\BossHud.h" />
+    <ClInclude Include="Source\UI\Hud\DebugHud.h" />
     <ClInclude Include="Source\UI\Elements\UIElement.h" />
     <ClInclude Include="Source\UI\Elements\UISprite.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、次の6行が**追加**されます。

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI\Hud</Filter>
     </ClCompile>
+    <ClCompile Include="Source\UI\Hud\DebugHud.cpp">
+      <Filter>Source\UI\Hud</Filter>
+    </ClCompile>
     <ClCompile Include="Source\UI\Elements\Gauge.cpp">
       <Filter>Source\UI\Elements</Filter>
```

<!-- file: ../../../Game/Game.vcxproj.filters -->

```diff
       <Filter>Source\UI\Hud</Filter>
     </ClInclude>
+    <ClInclude Include="Source\UI\Hud\DebugHud.h">
+      <Filter>Source\UI\Hud</Filter>
+    </ClInclude>
     <ClInclude Include="Source\UI\Elements\UIElement.h">
       <Filter>Source\UI\Elements</Filter>
```

### 1-2. DebugHud.h を書く

`UIElement` は継承しません。HUD の部品ではなく、`UIManager` がメンバーとして持つだけのクラスです。

<!-- new: ../../../Game/Source/UI/Hud/DebugHud.h -->

```cpp
#pragma once

#include "Text.h"

// 開発用の表示（今は FPS だけ）。ゲームの HUD とは分けて持つ。UIElement は継承しない。UIManager が持つ。
// FPS は Debug・Preview・Release のどのビルドでも出す（出すかどうかは DebugHud.cpp の SHOW_FPS）。
// FPS は画面の右上に小さく出す。毎フレームの値は揺れて読みにくいので、一定時間ごとの平均を出す。
// FPS は実際の時間（steady_clock）で測る。ゲームの経過時間（GetFrameDeltaTime）は 1/30 秒で頭打ちになるため、
// それで測ると 30 FPS より下が出せない。
class DebugHud
{
public:
	void Init();
	void Draw(RenderContext& rc);

private:
	Text m_fps;
};
```

### 1-3. DebugHud.cpp を書く

<!-- new: ../../../Game/Source/UI/Hud/DebugHud.cpp -->

```cpp
#include "stdafx.h"
#include "DebugHud.h"

namespace
{
	// FPS の文字。右上に小さく出す。
	constexpr float FPS_MARGIN_RIGHT = 20.0f;  // 画面の右端から、いちばん長い表示（FPS_WIDEST_TEXT）の右端まで。
	constexpr float FPS_MARGIN_TOP = 20.0f;    // 画面の上端から、文字の中央まで。
	constexpr float FPS_SCALE = 0.4f;
	const Vector4 FPS_COLOR = { 1.0f, 1.0f, 1.0f, 0.9f };
	const Vector4 FPS_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.8f };
	constexpr float FPS_SHADOW_OFFSET = 1.5f;
	// 数字の幅で文字が左右に動かないよう、左端を固定する。左端は、この文字が右端に収まる位置。
	constexpr const wchar_t* FPS_WIDEST_TEXT = L"FPS 0000.0";

	// FPS を出すか。Release で速さを確かめるため、Debug・Preview・Release のどのビルドでも出す。
	// 出したくないビルドがあれば、ここを #ifdef K2_DEBUG などで切り替える。
	constexpr bool SHOW_FPS = true;
}

void DebugHud::Init()
{
	// 右寄せにすると、数字の幅が変わるたびに文字全体が左右に動く（ガクガクして見える）。
	// そのため左寄せにし、左端を「いちばん長い表示が右端に収まる位置」に固定する。
	const float widestWidth = g_graphicsEngine->GetFontEngine().MeasureString(FPS_WIDEST_TEXT).x * FPS_SCALE;
	m_fps.SetPosition({
		static_cast<float>(UI_SPACE_WIDTH) * 0.5f - FPS_MARGIN_RIGHT - widestWidth,
		static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - FPS_MARGIN_TOP
	});
	m_fps.SetAlign(Text::Align::Left);
	m_fps.SetScale(FPS_SCALE);
	m_fps.SetColor(FPS_COLOR);
	m_fps.SetShadow(FPS_SHADOW_OFFSET, FPS_SHADOW_COLOR);
	m_fps.SetVisible(SHOW_FPS);
	// 最初の平均が出るまでの表示。
	m_fps.SetText(L"FPS --");
}

void DebugHud::Draw(RenderContext& rc)
{
	m_fps.Draw(rc);
}
```

- **左寄せにして、左端を固定する。** 右寄せにすると、数字の幅が変わるたびに文字全体が左右に動いてガクガクして見えます。いちばん長い表示 `FPS 0000.0` の幅をエンジンの `MeasureString()` で測り、それが右端から 20 に収まる位置を左端にします。
- **出すかどうかは `SHOW_FPS` の1か所で決める。** false なら `SetVisible(false)` で描かれなくなります。

**確かめ方:** ソリューションのビルドが通る（まだどこからも使っていないので、画面は変わらない）。

---

## 手順2: UIManager に持たせて描く

`DebugHud` を `UIManager` のメンバーにして、起動時に `Init()`、毎フレーム `Draw()` を呼びます。開発用の表示がゲームの HUD に隠れないように、いちばん最後（いちばん上）に描きます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加・書き換え（`Start()` で `Init()`、`Render()` で `Draw()` を呼ぶ。`Render()` のコメントも書き換える） |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加（`#include` と、メンバー `m_debugHud`） |

[UIManager.h](../../../Game/Source/UI/UIManager.h) に `#include` とメンバーを**追加**します。

<!-- file: ../../../Game/Source/UI/UIManager.h -->

```diff
 #include "PlayerHud.h"
 #include "BossHud.h"
+#include "DebugHud.h"
 
 class Player;
```

<!-- file: ../../../Game/Source/UI/UIManager.h -->

```diff
 	PlayerHud m_playerHud;
 	BossHud m_bossHud;
+	DebugHud m_debugHud;  // 開発用の表示（FPS）。いちばん上に描く。
 };
```

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の `Start()` に `Init()` を、`Render()` の最後に `Draw()` を**追加**します。`Render()` の先頭のコメントは、開発用の表示がいちばん上だとわかるように**書き換え**ます。

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 	m_playerHud.Init();
 	m_bossHud.Init();
+	m_debugHud.Init();
 	return true;
 }
```

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 void UIManager::Render(RenderContext& rc)
 {
-	// 後に描いたものが上に重なる。
+	// 後に描いたものが上に重なる。開発用の表示はいちばん上。
 	m_playerHud.Draw(rc);
 	m_bossHud.Draw(rc);
+	m_debugHud.Draw(rc);
 }
```

**確かめ方:** ゲームを起動すると、右上に小さく「FPS --」と出る。

---

## 手順3: 0.5秒ごとに平均の FPS を数えて出す

毎フレームの FPS は揺れて読めないので、0.5秒の間に何フレーム回ったかを数え、「フレーム数 ÷ 実際にかかった秒数」を出します。時間はゲームの経過時間（`GetFrameDeltaTime()`）ではなく、`std::chrono::steady_clock` で測ります。ゲームの経過時間は 1/30 秒で頭打ちになるので、それで測ると 30 FPS より下が出せないためです。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) | 追加（平均を取る間隔の定数と、`Update()`） |
| [DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) | 追加（`<chrono>`、`Update()`、時間を測るメンバー3つ） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加（`Update()` で `m_debugHud.Update()` を呼ぶ） |

[DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) に、`<chrono>` の `#include`、`Update()` の宣言、時間を測るメンバーを**追加**します。

<!-- file: ../../../Game/Source/UI/Hud/DebugHud.h -->

```diff
 #pragma once
 
+#include <chrono>
 #include "Text.h"
 
```

<!-- file: ../../../Game/Source/UI/Hud/DebugHud.h -->

```diff
 public:
 	void Init();
+	void Update(float deltaTime);
 	void Draw(RenderContext& rc);
 
 private:
+	using Clock = std::chrono::steady_clock;
+
 	Text m_fps;
+	Clock::time_point m_averageStart;  // 平均を取り始めた時刻。
+	int m_frameCount = 0;              // 平均を取り始めてからのフレーム数。
+	bool m_isStarted = false;          // 最初の Update で m_averageStart を決めたか。
 };
```

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の無名の namespace に、平均を取る間隔の定数を**追加**し、`Init()` と `Draw()` の間に `Update()` を**追加**します。

<!-- file: ../../../Game/Source/UI/Hud/DebugHud.cpp -->

```diff
 	// 数字の幅で文字が左右に動かないよう、左端を固定する。左端は、この文字が右端に収まる位置。
 	constexpr const wchar_t* FPS_WIDEST_TEXT = L"FPS 0000.0";
+	// この秒数ごとに、その間の平均の FPS に書き換える。
+	constexpr float FPS_AVERAGE_SECONDS = 0.5f;
 
 	// FPS を出すか。Release で速さを確かめるため、Debug・Preview・Release のどのビルドでも出す。
```

<!-- file: ../../../Game/Source/UI/Hud/DebugHud.cpp -->

```diff
 }
 
+void DebugHud::Update(float deltaTime)
+{
+	if (!SHOW_FPS) {
+		return;
+	}
+	const Clock::time_point now = Clock::now();
+	if (!m_isStarted) {
+		m_averageStart = now;
+		m_isStarted = true;
+	}
+	m_frameCount++;
+	const float elapsed = std::chrono::duration<float>(now - m_averageStart).count();
+	if (elapsed >= FPS_AVERAGE_SECONDS) {
+		wchar_t text[64];
+		swprintf_s(text, L"FPS %.1f", m_frameCount / elapsed);
+		m_fps.SetText(text);
+		m_averageStart = now;
+		m_frameCount = 0;
+	}
+	m_fps.Update(deltaTime);
+}
+
 void DebugHud::Draw(RenderContext& rc)
 {
```

- 最初の `Update()` で、測り始める時刻 `m_averageStart` を決めます（`m_isStarted`）。
- 0.5秒たったら文字を書き換えて、数え直します。`Text::SetText()` は値が変わったときだけ作り直すので、毎フレームの負担も減ります。

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の `Update()` で、`m_debugHud.Update()` を**追加**します。

<!-- file: ../../../Game/Source/UI/UIManager.cpp -->

```diff
 	m_playerHud.Update(deltaTime);
 	m_bossHud.Update(deltaTime);
+	m_debugHud.Update(deltaTime);
 }
 
```

**確かめ方:** ゲームを起動すると、0.5秒ほどで右上が「FPS 123.4」のような数字に変わり、動き続ける。

---

## 手順4: エンジンが出していた FPS を止める

エンジンの `K2EngineLow::EndFrame()` も、Debug ビルドでは左上に `FPS = xx.xx` を描いています。2つ出ると紛らわしいので、エンジンの方を止めます。エンジンだけで動かすとき（ゲームの UI がないとき）にまた使えるように、消さずにコメントアウトします。コメントを外したときに左上の HUD と重ならないよう、コメントアウトする中身は「右上に小さく出す」形に書き換えておきます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Font.cpp](../../../k2EngineLow/graphics/font/Font.cpp) | 追加（`MeasureString()` の中身） |
| [Font.h](../../../k2EngineLow/graphics/font/Font.h) | 追加（`MeasureString()` の宣言） |
| [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) | コメントアウト・書き換え（`EndFrame()` の FPS を描く処理を、右上に出す形に書き換えてコメントアウトし、説明のコメント2行を足す） |

### 4-1. Font に文字の幅を測る関数を足す

コメントアウトする中身で、右端をそろえるために `Font::MeasureString()` を使います。中身は、[UIの構成の手順](UIの構成の手順.md) の手順2で `FontEngine` に足した `MeasureString()` を呼ぶだけです。

[Font.h](../../../k2EngineLow/graphics/font/Font.h) の `SetShadowParam()` の下（`private:` の上）に**追加**します。

<!-- file: ../../../k2EngineLow/graphics/font/Font.h -->

```diff
 			m_shadowColor = shadowColor;
 		}
+		/// <summary>
+		/// 文字列を大きさ 1 で描いたときの幅と高さ（ピクセル）を測る。右寄せ・中央寄せに使う。
+		/// Draw の pivot は割合ではなく、文字の左上からのピクセル数として扱われるので、寄せるときはこの値で位置をずらす。
+		/// </summary>
+		Vector2 MeasureString(const wchar_t* text) const;
 	private:
 		DirectX::SpriteBatch* m_spriteBatch = nullptr;		//　スプライトバッチ。
```

[Font.cpp](../../../k2EngineLow/graphics/font/Font.cpp) の `Font::End()` の下に**追加**します。

<!-- file: ../../../k2EngineLow/graphics/font/Font.cpp -->

```diff
 		fontEngine.EndDraw(rc);
 	}
+	Vector2 Font::MeasureString(const wchar_t* text) const
+	{
+		return g_graphicsEngine->GetFontEngine().MeasureString(text);
+	}
 	void Font::Draw(
 		wchar_t const* text,
```

### 4-2. エンジンの FPS の表示をコメントアウトする

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の `K2EngineLow::EndFrame()` の先頭、`#ifdef K2_DEBUG` から `#endif` までを**書き換え**て、**コメントアウト**します。

- 書き換え: 左上に大きさ 1 で描いていたのを、大きさ 0.5 で右上に描く形にする（`fpsScale`・`fpsShadowOffset`・`fpsWidth`・`fpsPosition` を使う）。
- コメントアウト: 行の頭に `//` を付ける（`#ifdef` / `#endif` の行も）。
- その上に、なぜ止めたかのコメント2行を**追加**する。

<!-- file: ../../../k2EngineLow/k2EngineLow.cpp -->

```diff
 	void K2EngineLow::EndFrame()
 	{
-#ifdef K2_DEBUG
-		m_fpsFont->Begin(g_graphicsEngine->GetRenderContext());
-		float time = g_gameTime->GetFrameDeltaTime();
-		wchar_t text[256];
-		swprintf(text, L"FPS = %0.2f", 1.0f / time);
-		m_fpsFontShadow->Draw(text, { UI_SPACE_WIDTH * -0.48f + 3.0f , UI_SPACE_HEIGHT * 0.48f - 3.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 1.0f, { 0.0f, 1.0f });
-		m_fpsFont->Draw(text, { UI_SPACE_WIDTH * -0.48f, UI_SPACE_HEIGHT * 0.48f }, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, 1.0f, { 0.0f, 1.0f });
-		m_fpsFont->End(g_graphicsEngine->GetRenderContext());
-#endif 
+		// FPS の表示は、ゲームの UI（Game/Source/UI/Hud/DebugHud）で出すので、ここでは描かない。
+		// エンジンだけで動かすとき（ゲームの UI がないとき）は、下のコメントを外せば右上に出る。
+//#ifdef K2_DEBUG
+//		m_fpsFont->Begin(g_graphicsEngine->GetRenderContext());
+//		float time = g_gameTime->GetFrameDeltaTime();
+//		wchar_t text[256];
+//		swprintf(text, L"FPS = %0.2f", 1.0f / time);
+//		const float fpsScale = 0.5f;
+//		const float fpsShadowOffset = 1.5f;
+//		// 右端をそろえるため、文字の幅だけ左から描き始める。位置は文字の左上。
+//		const float fpsWidth = m_fpsFont->MeasureString(text).x * fpsScale;
+//		const Vector2 fpsPosition = { UI_SPACE_WIDTH * 0.49f - fpsWidth, UI_SPACE_HEIGHT * 0.49f };
+//		m_fpsFontShadow->Draw(text, { fpsPosition.x + fpsShadowOffset, fpsPosition.y - fpsShadowOffset }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, fpsScale, { 0.0f, 1.0f });
+//		m_fpsFont->Draw(text, fpsPosition, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, fpsScale, { 0.0f, 1.0f });
+//		m_fpsFont->End(g_graphicsEngine->GetRenderContext());
+//#endif
 		m_graphicsEngine->EndRender();
 #ifdef USE_FPS_LIMITTER
```

`m_fpsFont`・`m_fpsFontShadow` を作る処理（`K2EngineLow::Init()`）は、もとからある（変えない）。使わなくなりますが、コメントを外したときにそのまま動くように残しています。

**確かめ方（これで出来上がり）:**

- k2EngineLow とゲームのビルドが通る。
- Debug ビルドで起動しても、エンジンの `FPS = xx.xx` が出ず、ゲームの `FPS xx.x` だけが右上に出る。
- Release ビルドでも、右上に FPS が出る。
