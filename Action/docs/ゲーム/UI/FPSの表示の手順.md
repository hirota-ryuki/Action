# FPSの表示の手順

設計: [FPSの表示の設計](FPSの表示の設計.md)
経緯: [UIの経緯](UIの経緯.md)

ゲームの画面の右上に、0.5秒ごとの平均の FPS を出すまでの手順です。

**出来上がるもの:** Debug・Preview・Release のどのビルドでも、ゲームの画面の右上に `FPS 60.0` のように小さく出ます。エンジンが Debug ビルドだけ左上に出していた `FPS = xx.xx` は止めます。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [UIの構成の手順](UIの構成の手順.md) と [HUDの手順](HUDの手順.md) を済ませておく。この手順では、そこで作った `UIManager` と文字の部品 `Text`、エンジンの `FontEngine::MeasureString()` を使う。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | `DebugHud` を作り、右上に文字を置く | ビルドが通る |
| 2 | `UIManager` に持たせて描く | 右上に「FPS --」と出る |
| 3 | 0.5秒ごとに平均の FPS を数えて出す | 右上の数字が動く |
| 4 | エンジンが出していた FPS を止める | エンジンの FPS が消える |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- 書き換えるところは、「前」と「後」の枠を並べています。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) | 新しく作る・追加 | 1・3 |
| [DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) | 新しく作る・追加 | 1・3 |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加 | 2 |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加・書き換え | 2・3 |
| [Font.h](../../../k2EngineLow/graphics/font/Font.h) | 追加 | 4 |
| [Font.cpp](../../../k2EngineLow/graphics/font/Font.cpp) | 追加 | 4 |
| [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) | 書き換え・コメントアウト | 4 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: DebugHud を作り、右上に文字を置く

開発用の表示（今は FPS だけ）を、ゲームの HUD とは分けて持ちます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) | 新しく作る（開発用の表示） |
| [DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) | 新しく作る（右上に文字を置く） |

`Source\UI\Hud` フィルターに `DebugHud.h` と `DebugHud.cpp` を作ります。

### 1-1. DebugHud クラスの形を書く

[DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) を新しく作り、次の10行を書きます（1〜10行目）。

<!-- new: Action/Game/Source/UI/Hud/DebugHud.h -->
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

#include "Text.h"

class DebugHud
{
public:
	void Init();
	void Draw(RenderContext& rc);
};
```

</td></tr></table>

### 1-2. FPS の文字を持つメンバーを足す

[DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) の **9行目** `void Draw(RenderContext& rc);` の下に、3行足します（10〜12行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.h 10 -->
<table class="code-lines"><tr><td>

```cpp
10
11
12
```

</td><td>

```cpp

private:
	Text m_fps;
```

</td></tr></table>

### 1-3. コメントを書く

[DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) の **4行目**（`#include "Text.h"` の下の空の行）の下、`class DebugHud` の上に、5行足します（5〜9行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.h 5 -->
<table class="code-lines"><tr><td>

```cpp
5
6
7
8
9
```

</td><td>

```cpp
// 開発用の表示（今は FPS だけ）。ゲームの HUD とは分けて持つ。UIElement は継承しない。UIManager が持つ。
// FPS は Debug・Preview・Release のどのビルドでも出す（出すかどうかは DebugHud.cpp の SHOW_FPS）。
// FPS は画面の右上に小さく出す。毎フレームの値は揺れて読みにくいので、一定時間ごとの平均を出す。
// FPS は実際の時間（steady_clock）で測る。ゲームの経過時間（GetFrameDeltaTime）は 1/30 秒で頭打ちになるため、
// それで測ると 30 FPS より下が出せない。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 8〜9 | FPS は実際の時間で測る。ゲームの経過時間は 1/30 秒より長くならないように止めてあるので、それで測ると 30 FPS より下が出せない |

### 1-4. DebugHud.cpp を作り、使うものを読み込む

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) を新しく作り、次の3行を書きます（1〜3行目）。

<!-- new: Action/Game/Source/UI/Hud/DebugHud.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
```

</td><td>

```cpp
#include "stdafx.h"
#include "DebugHud.h"

```

</td></tr></table>

### 1-5. 値を置く場所を作る

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **3行目**（`#include "DebugHud.h"` の下の空の行）の下（ファイルの最後）に、4行足します（4〜7行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 4 -->
<table class="code-lines"><tr><td>

```cpp
4
5
6
7
```

</td><td>

```cpp
namespace
{
}

```

</td></tr></table>

### 1-6. FPS の文字の位置と見た目を決める

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **5行目** `{` の下、`}` の上に、7行足します（6〜12行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 6 -->
<table class="code-lines"><tr><td>

```cpp
6
7
8
9
10
11
12
```

</td><td>

```cpp
	// FPS の文字。右上に小さく出す。
	constexpr float FPS_MARGIN_RIGHT = 20.0f;  // 画面の右端から、いちばん長い表示（FPS_WIDEST_TEXT）の右端まで。
	constexpr float FPS_MARGIN_TOP = 20.0f;    // 画面の上端から、文字の中央まで。
	constexpr float FPS_SCALE = 0.4f;
	const Vector4 FPS_COLOR = { 1.0f, 1.0f, 1.0f, 0.9f };
	const Vector4 FPS_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.8f };
	constexpr float FPS_SHADOW_OFFSET = 1.5f;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 7 | 画面の右端から、いちばん長い表示の右端までの距離 |
| 8 | 画面の上端から、文字の中央までの距離 |

### 1-7. いちばん長い表示を決める

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **12行目** `constexpr float FPS_SHADOW_OFFSET = 1.5f;` の下に、3行足します（13〜15行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 13 -->
<table class="code-lines"><tr><td>

```cpp
13
14
15
```

</td><td>

```cpp
	// 数字の幅で文字が左右に動かないよう、左端を固定する。左端は、この文字が右端に収まる位置。
	constexpr const wchar_t* FPS_WIDEST_TEXT = L"FPS 0000.0";

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 14 | 数字の幅で文字が左右に動かないよう、この文字が右端に収まる位置に左端を固定する |

### 1-8. FPS を出すかを決める

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **15行目**（`constexpr const wchar_t* FPS_WIDEST_TEXT = L"FPS 0000.0";` の下の空の行）の下、`}` の上に、3行足します（16〜18行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 16 -->
<table class="code-lines"><tr><td>

```cpp
16
17
18
```

</td><td>

```cpp
	// FPS を出すか。Release で速さを確かめるため、Debug・Preview・Release のどのビルドでも出す。
	// 出したくないビルドがあれば、ここを #ifdef K2_DEBUG などで切り替える。
	constexpr bool SHOW_FPS = true;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 18 | 出すかどうかは、この1か所で決める。Release で速さを確かめるため、どのビルドでも出す |

### 1-9. 準備の関数を書く

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **20行目**（`}` の下の空の行）の下（ファイルの最後）に、11行足します（21〜31行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 21 -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp
void DebugHud::Init()
{
	m_fps.SetAlign(Text::Align::Left);
	m_fps.SetScale(FPS_SCALE);
	m_fps.SetColor(FPS_COLOR);
	m_fps.SetShadow(FPS_SHADOW_OFFSET, FPS_SHADOW_COLOR);
	m_fps.SetVisible(SHOW_FPS);
	// 最初の平均が出るまでの表示。
	m_fps.SetText(L"FPS --");
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 23 | 左寄せにする |
| 27 | `SHOW_FPS` が `false` なら描かない |
| 29 | 最初の平均が出るまでの表示 |

### 1-10. 左端の位置を決める

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **22行目** `{` の下、`m_fps.SetAlign(Text::Align::Left);` の上に、7行足します（23〜29行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 23 -->
<table class="code-lines"><tr><td>

```cpp
23
24
25
26
27
28
29
```

</td><td>

```cpp
	// 右寄せにすると、数字の幅が変わるたびに文字全体が左右に動く（ガクガクして見える）。
	// そのため左寄せにし、左端を「いちばん長い表示が右端に収まる位置」に固定する。
	const float widestWidth = g_graphicsEngine->GetFontEngine().MeasureString(FPS_WIDEST_TEXT).x * FPS_SCALE;
	m_fps.SetPosition({
		static_cast<float>(UI_SPACE_WIDTH) * 0.5f - FPS_MARGIN_RIGHT - widestWidth,
		static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - FPS_MARGIN_TOP
	});
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 23〜24 | 右寄せにすると、数字の幅が変わるたびに文字全体が左右に動いてガクガクして見える。そのため左寄せにして、左端を固定する |
| 25 | いちばん長い表示の幅を、エンジンの `MeasureString()` で測る |
| 26〜29 | 右端から 20 に収まる位置を左端にする |

### 1-11. 描く関数を書く

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **38行目**（`}` の下の空の行）の下（ファイルの最後）に、4行足します（39〜42行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 39 -->
<table class="code-lines"><tr><td>

```cpp
39
40
41
42
```

</td><td>

```cpp
void DebugHud::Draw(RenderContext& rc)
{
	m_fps.Draw(rc);
}
```

</td></tr></table>

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: UIManager に持たせて描く

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加（`#include`、メンバー） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加（準備・描く）・書き換え（コメント） |

### 2-1. DebugHud.h を読み込む

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **4行目** `#include "BossHud.h"` の下に、1行足します（5〜5行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 5 -->
<table class="code-lines"><tr><td>

```cpp
5
```

</td><td>

```cpp
#include "DebugHud.h"
```

</td></tr></table>

### 2-2. DebugHud を持つメンバーを足す

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **37行目** `BossHud m_bossHud;` の下に、1行足します（38〜38行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 38 -->
<table class="code-lines"><tr><td>

```cpp
38
```

</td><td>

```cpp
	DebugHud m_debugHud;  // 開発用の表示（FPS）。いちばん上に描く。
```

</td></tr></table>

### 2-3. 最初に DebugHud を準備する

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **8行目** `m_bossHud.Init();` の下に、1行足します（9〜9行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 9 -->
<table class="code-lines"><tr><td>

```cpp
9
```

</td><td>

```cpp
	m_debugHud.Init();
```

</td></tr></table>

### 2-4. 描く順のコメントを書き換える

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **22行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/Game/Source/UI/UIManager.cpp 22 -->
<table class="code-lines"><tr><td>

```cpp
22
```

</td><td>

```cpp
	// 後に描いたものが上に重なる。
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/Game/Source/UI/UIManager.cpp 22 -->
<table class="code-lines"><tr><td>

```cpp
22
```

</td><td>

```cpp
	// 後に描いたものが上に重なる。開発用の表示はいちばん上。
```

</td></tr></table>

### 2-5. DebugHud をいちばん上に描く

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **24行目** `m_bossHud.Draw(rc);` の下に、1行足します（25〜25行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 25 -->
<table class="code-lines"><tr><td>

```cpp
25
```

</td><td>

```cpp
	m_debugHud.Draw(rc);
```

</td></tr></table>

**確かめ方:** 起動すると、右上に「FPS --」と出る。

---

## 手順3: 0.5秒ごとに平均の FPS を数えて出す

毎フレームの値は揺れて読みにくいので、0.5秒ごとにその間の平均を出します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) | 追加（`#include <chrono>`、`Update()`、時計とメンバー） |
| [DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) | 追加（平均を取る間隔、`Update()`） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加（`Update()` を呼ぶ） |

### 3-1. 時間を測る仕組みを読み込む

[DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) の **2行目**（`#pragma once` の下の空の行）の下、`#include "Text.h"` の上に、1行足します（3〜3行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.h 3 -->
<table class="code-lines"><tr><td>

```cpp
3
```

</td><td>

```cpp
#include <chrono>
```

</td></tr></table>

### 3-2. 毎フレームの関数を宣言する

[DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) の **14行目** `void Init();` の下に、1行足します（15〜15行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.h 15 -->
<table class="code-lines"><tr><td>

```cpp
15
```

</td><td>

```cpp
	void Update(float deltaTime);
```

</td></tr></table>

### 3-3. 使う時計を決める

[DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) の **18行目** `private:` の下、`Text m_fps;` の上に、2行足します（19〜20行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.h 19 -->
<table class="code-lines"><tr><td>

```cpp
19
20
```

</td><td>

```cpp
	using Clock = std::chrono::steady_clock;

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 19 | 実際の時間を測る時計。`steady_clock` は、PC の時刻を変えても戻らない |

### 3-4. 平均を取るためのメンバーを足す

[DebugHud.h](../../../Game/Source/UI/Hud/DebugHud.h) の **21行目** `Text m_fps;` の下に、3行足します（22〜24行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.h 22 -->
<table class="code-lines"><tr><td>

```cpp
22
23
24
```

</td><td>

```cpp
	Clock::time_point m_averageStart;  // 平均を取り始めた時刻。
	int m_frameCount = 0;              // 平均を取り始めてからのフレーム数。
	bool m_isStarted = false;          // 最初の Update で m_averageStart を決めたか。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 22 | 平均を取り始めた時刻 |
| 23 | 平均を取り始めてからのフレーム数 |
| 24 | 最初の `Update()` で、取り始める時刻を決めたか |

### 3-5. 平均を取る間隔を決める

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **14行目** `constexpr const wchar_t* FPS_WIDEST_TEXT = L"FPS 0000.0";` の下に、2行足します（15〜16行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 15 -->
<table class="code-lines"><tr><td>

```cpp
15
16
```

</td><td>

```cpp
	// この秒数ごとに、その間の平均の FPS に書き換える。
	constexpr float FPS_AVERAGE_SECONDS = 0.5f;
```

</td></tr></table>

### 3-6. 毎フレームの関数の形を書く

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **40行目**（`}` の下の空の行）の下、`void DebugHud::Draw(RenderContext& rc)` の上に、8行足します（41〜48行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 41 -->
<table class="code-lines"><tr><td>

```cpp
41
42
43
44
45
46
47
48
```

</td><td>

```cpp
void DebugHud::Update(float deltaTime)
{
	if (!SHOW_FPS) {
		return;
	}
	m_fps.Update(deltaTime);
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 43〜45 | 出さない設定なら何もしない |

### 3-7. 最初のフレームで、測り始める

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **45行目** `}` の下、`m_fps.Update(deltaTime);` の上に、5行足します（46〜50行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 46 -->
<table class="code-lines"><tr><td>

```cpp
46
47
48
49
50
```

</td><td>

```cpp
	const Clock::time_point now = Clock::now();
	if (!m_isStarted) {
		m_averageStart = now;
		m_isStarted = true;
	}
```

</td></tr></table>

### 3-8. 0.5秒たったら、平均を出して数え直す

[DebugHud.cpp](../../../Game/Source/UI/Hud/DebugHud.cpp) の **50行目** `}` の下、`m_fps.Update(deltaTime);` の上に、9行足します（51〜59行目になります）。

<!-- add: Action/Game/Source/UI/Hud/DebugHud.cpp 51 -->
<table class="code-lines"><tr><td>

```cpp
51
52
53
54
55
56
57
58
59
```

</td><td>

```cpp
	m_frameCount++;
	const float elapsed = std::chrono::duration<float>(now - m_averageStart).count();
	if (elapsed >= FPS_AVERAGE_SECONDS) {
		wchar_t text[64];
		swprintf_s(text, L"FPS %.1f", m_frameCount / elapsed);
		m_fps.SetText(text);
		m_averageStart = now;
		m_frameCount = 0;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 51 | フレームを数える |
| 52 | 取り始めてからの時間（秒） |
| 53〜59 | 0.5秒たったら、フレーム数 ÷ 時間を出して、数え直す。`SetText()` は値が変わったときだけ作り直すので、毎フレームの負担も減る |

### 3-9. UIManager から Update() を呼ぶ

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **17行目** `m_bossHud.Update(deltaTime);` の下に、1行足します（18〜18行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 18 -->
<table class="code-lines"><tr><td>

```cpp
18
```

</td><td>

```cpp
	m_debugHud.Update(deltaTime);
```

</td></tr></table>

### 出来上がった DebugHud.h

<!-- full: Action/Game/Source/UI/Hud/DebugHud.h -->
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
```

</td><td>

```cpp
#pragma once

#include <chrono>
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
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	using Clock = std::chrono::steady_clock;

	Text m_fps;
	Clock::time_point m_averageStart;  // 平均を取り始めた時刻。
	int m_frameCount = 0;              // 平均を取り始めてからのフレーム数。
	bool m_isStarted = false;          // 最初の Update で m_averageStart を決めたか。
};
```

</td></tr></table>

### 出来上がった DebugHud.cpp

<!-- full: Action/Game/Source/UI/Hud/DebugHud.cpp -->
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
64
65
66
```

</td><td>

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
	// この秒数ごとに、その間の平均の FPS に書き換える。
	constexpr float FPS_AVERAGE_SECONDS = 0.5f;

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

void DebugHud::Update(float deltaTime)
{
	if (!SHOW_FPS) {
		return;
	}
	const Clock::time_point now = Clock::now();
	if (!m_isStarted) {
		m_averageStart = now;
		m_isStarted = true;
	}
	m_frameCount++;
	const float elapsed = std::chrono::duration<float>(now - m_averageStart).count();
	if (elapsed >= FPS_AVERAGE_SECONDS) {
		wchar_t text[64];
		swprintf_s(text, L"FPS %.1f", m_frameCount / elapsed);
		m_fps.SetText(text);
		m_averageStart = now;
		m_frameCount = 0;
	}
	m_fps.Update(deltaTime);
}

void DebugHud::Draw(RenderContext& rc)
{
	m_fps.Draw(rc);
}
```

</td></tr></table>

**確かめ方:** 右上の数字が 0.5秒ごとに変わる。

---

## 手順4: エンジンが出していた FPS を止める

エンジンの `K2EngineLow::EndFrame()` も、Debug ビルドでは左上に `FPS = xx.xx` を描いています。2つ出ると紛らわしいので、エンジンの方を止めます。エンジンだけで動かすとき（ゲームの UI がないとき）にまた使えるように、消さずにコメントアウトします。コメントを外したときに左上と重ならないよう、コメントアウトする中身は「右上に小さく出す」形に書き換えておきます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Font.h](../../../k2EngineLow/graphics/font/Font.h) | 追加（`MeasureString()` の宣言） |
| [Font.cpp](../../../k2EngineLow/graphics/font/Font.cpp) | 追加（`MeasureString()` の中身） |
| [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) | 書き換え・コメントアウト（`EndFrame()` の FPS を描く処理） |

### 4-1. Font に、文字の幅を測る関数を宣言する

コメントアウトする中身で、右端をそろえるために使います。

[Font.h](../../../k2EngineLow/graphics/font/Font.h) の **55行目** `}` の下、`private:` の上に、5行足します（56〜60行目になります）。

<!-- add: Action/k2EngineLow/graphics/font/Font.h 56 -->
<table class="code-lines"><tr><td>

```cpp
56
57
58
59
60
```

</td><td>

```cpp
		/// <summary>
		/// 文字列を大きさ 1 で描いたときの幅と高さ（ピクセル）を測る。右寄せ・中央寄せに使う。
		/// Draw の pivot は割合ではなく、文字の左上からのピクセル数として扱われるので、寄せるときはこの値で位置をずらす。
		/// </summary>
		Vector2 MeasureString(const wchar_t* text) const;
```

</td></tr></table>

### 4-2. 文字の幅を測る関数を書く

[Font.cpp](../../../k2EngineLow/graphics/font/Font.cpp) の **18行目** `}` の下、`void Font::Draw(` の上に、4行足します（19〜22行目になります）。

<!-- add: Action/k2EngineLow/graphics/font/Font.cpp 19 -->
<table class="code-lines"><tr><td>

```cpp
19
20
21
22
```

</td><td>

```cpp
	Vector2 Font::MeasureString(const wchar_t* text) const
	{
		return g_graphicsEngine->GetFontEngine().MeasureString(text);
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 21 | [UIの構成の手順](UIの構成の手順.md) の 2-9 で `FontEngine` に足した `MeasureString()` を呼ぶだけ |

### 4-3. エンジンの FPS の表示を、右上に出す形にしてコメントアウトする

`#ifdef K2_DEBUG` から `#endif` までを、次のように書き換えます。

- 左上に大きさ 1 で描いていたのを、大きさ 0.5 で右上に描く形にする（`fpsScale`・`fpsShadowOffset`・`fpsWidth`・`fpsPosition` を使う）。
- 行の頭に `//` を付けてコメントアウトする（`#ifdef` / `#endif` の行も）。
- その上に、なぜ止めたかのコメント2行を書く。

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の **75〜83行目**を、次のように書き換えます（75〜90行目になります）。

**前**

<!-- rewrite-before: Action/k2EngineLow/k2EngineLow.cpp 75 -->
<table class="code-lines"><tr><td>

```cpp
75
76
77
78
79
80
81
82
83
```

</td><td>

```cpp
#ifdef K2_DEBUG
		m_fpsFont->Begin(g_graphicsEngine->GetRenderContext());
		float time = g_gameTime->GetFrameDeltaTime();
		wchar_t text[256];
		swprintf(text, L"FPS = %0.2f", 1.0f / time);
		m_fpsFontShadow->Draw(text, { UI_SPACE_WIDTH * -0.48f + 3.0f , UI_SPACE_HEIGHT * 0.48f - 3.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 1.0f, { 0.0f, 1.0f });
		m_fpsFont->Draw(text, { UI_SPACE_WIDTH * -0.48f, UI_SPACE_HEIGHT * 0.48f }, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, 1.0f, { 0.0f, 1.0f });
		m_fpsFont->End(g_graphicsEngine->GetRenderContext());
#endif 
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/k2EngineLow.cpp 75 -->
<table class="code-lines"><tr><td>

```cpp
75
76
77
78
79
80
81
82
83
84
85
86
87
88
89
90
```

</td><td>

```cpp
		// FPS の表示は、ゲームの UI（Game/Source/UI/Hud/DebugHud）で出すので、ここでは描かない。
		// エンジンだけで動かすとき（ゲームの UI がないとき）は、下のコメントを外せば右上に出る。
//#ifdef K2_DEBUG
//		m_fpsFont->Begin(g_graphicsEngine->GetRenderContext());
//		float time = g_gameTime->GetFrameDeltaTime();
//		wchar_t text[256];
//		swprintf(text, L"FPS = %0.2f", 1.0f / time);
//		const float fpsScale = 0.5f;
//		const float fpsShadowOffset = 1.5f;
//		// 右端をそろえるため、文字の幅だけ左から描き始める。位置は文字の左上。
//		const float fpsWidth = m_fpsFont->MeasureString(text).x * fpsScale;
//		const Vector2 fpsPosition = { UI_SPACE_WIDTH * 0.49f - fpsWidth, UI_SPACE_HEIGHT * 0.49f };
//		m_fpsFontShadow->Draw(text, { fpsPosition.x + fpsShadowOffset, fpsPosition.y - fpsShadowOffset }, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, fpsScale, { 0.0f, 1.0f });
//		m_fpsFont->Draw(text, fpsPosition, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, fpsScale, { 0.0f, 1.0f });
//		m_fpsFont->End(g_graphicsEngine->GetRenderContext());
//#endif
```

</td></tr></table>

`m_fpsFont`・`m_fpsFontShadow` を作る処理（`K2EngineLow::Init()`）は、もとからあるので変えません。コメントを外したときにそのまま動くように残しています。

**確かめ方（これで出来上がり）:**

- k2EngineLow とゲームのビルドが通る。
- Debug ビルドで起動しても、エンジンの `FPS = xx.xx` が出ず、ゲームの `FPS xx.x` だけが右上に出る。
- Release ビルドでも、右上に FPS が出る。
