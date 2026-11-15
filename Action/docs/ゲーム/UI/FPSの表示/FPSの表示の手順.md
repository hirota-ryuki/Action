# FPSの表示の手順

設計: [FPSの表示の設計](FPSの表示の設計.md)
経緯: [UIの経緯](../UIの経緯.md)

ゲームの画面の右上に、0.5秒ごとの平均の FPS を出すまでの手順です。

**出来上がるもの:** Debug・Preview・Release のどのビルドでも、ゲームの画面の右上に `FPS 60.0` のように小さく出ます。エンジンが Debug ビルドだけ左上に出していた `FPS = xx.xx` は止めます。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [UIの構成の手順](../UIの構成/UIの構成の手順.md) と [HUDの手順](../HUD/HUDの手順.md) を済ませておく。この手順では、そこで作った `UIManager` と文字の部品 `Text`、エンジンの `FontEngine::MeasureString()` を使う。

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
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる変数・引数・関数・型・マクロ・C++ の書き方を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、このゲームで作ったもの・エンジン（k2Engine）の機能・C++ の標準ライブラリ・C++ の書き方のどれかがわかります。
- [前提知識](../../../前提知識.md) で「よく知っている」にあるもの（クラス・ポインター・`constexpr`・`namespace`・`using`・`static_cast` など）は、表に載せません。「まあまあ」「怪しい」「微妙」にあるものは載せます。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) | 新しく作る・追加 | 1・3 |
| [DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) | 新しく作る・追加 | 1・3 |
| [UIManager.h](../../../../Game/Source/UI/UIManager.h) | 追加 | 2 |
| [UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) | 追加・書き換え | 2・3 |
| [Font.h](../../../../k2EngineLow/graphics/font/Font.h) | 追加 | 4 |
| [Font.cpp](../../../../k2EngineLow/graphics/font/Font.cpp) | 追加 | 4 |
| [k2EngineLow.cpp](../../../../k2EngineLow/k2EngineLow.cpp) | 書き換え・コメントアウト | 4 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: DebugHud を作り、右上に文字を置く

開発用の表示（今は FPS だけ）を、ゲームの HUD とは分けて持ちます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) | 新しく作る（開発用の表示） |
| [DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) | 新しく作る（右上に文字を置く） |

`Source\UI\Hud` フィルターに `DebugHud.h` と `DebugHud.cpp` を作ります。

### 1-1. DebugHud クラスの形を書く

[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) を新しく作り、次の10行を書きます（1〜10行目）。

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

- **なぜ:** FPS のような開発用の表示を、ゲームの HUD と同じクラスに混ぜない。別のクラスにしておけば、出す・消すを1か所で決められる。
- **効果:** `UIManager` が `DebugHud` を1つ持つだけで、開発用の表示を足していけるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `DebugHud` | 開発用の表示をまとめるため | 今は FPS の文字を持つクラス | この手順で作る |
| 型 | `Text` | 画面に文字を出すため | 位置・大きさ・色などを決めて文字を描く部品 | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 関数 | `Init` | 使う前の準備をするため | 文字の位置や見た目を決める。最初に1回だけ呼ぶ | この手順で作る |
| 関数 | `Draw` | 文字を画面に出すため | 持っている文字を描く。毎フレーム呼ぶ | この手順で作る |

### 1-2. FPS の文字を持つメンバーを足す

[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) の **9行目** `void Draw(RenderContext& rc);` の下に、3行足します（10〜12行目になります）。

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

- **なぜ:** FPS の文字は、`Init` で決めた見た目を `Draw` まで持っていないといけない。メンバー変数にすれば、関数をまたいで使える。
- **効果:** `Init` と `Draw` の両方から、同じ FPS の文字を触れるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_fps` | FPS の文字を持つため | `Text` 1つ。位置・見た目・文字の中身を入れる | この手順で作る |

### 1-3. コメントを書く

[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) の **4行目**（`#include "Text.h"` の下の空の行）の下、`class DebugHud` の上に、5行足します（5〜9行目になります）。

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

- **なぜ:** 「なぜ実際の時間で測るのか」「どのビルドで出すのか」は、コードだけを見ても思い出せない。理由をクラスの上に書いておく。
- **効果:** あとでこのクラスを読む人が、`GetFrameDeltaTime` を使ってはいけない理由で迷わなくなる。

### 1-4. DebugHud.cpp を作り、使うものを読み込む

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) を新しく作り、次の3行を書きます（1〜3行目）。

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

- **なぜ:** `.cpp` の1行目は `stdafx.h` と決まっている。次に、このクラスの `.h` を読み込む。
- **効果:** `DebugHud.cpp` で、`Text` やエンジンの関数が使えるようになる。

### 1-5. 値を置く場所を作る

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **3行目**（`#include "DebugHud.h"` の下の空の行）の下（ファイルの最後）に、4行足します（4〜7行目になります）。

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

- **なぜ:** この `.cpp` の中だけで使う値は、無名の `namespace` に入れる。ほかのファイルから見えず、名前がぶつからない。
- **効果:** 見た目の数字を1か所にまとめて置けるようになる。

### 1-6. FPS の文字の位置と見た目を決める

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **5行目** `{` の下、`}` の上に、7行足します（6〜12行目になります）。

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

- **なぜ:** 位置・大きさ・色を、コードの中に直接書かず、名前を付けて置く。あとで見た目を変えるときに、ここだけ直せばよい。
- **効果:** FPS の文字の位置と見た目を、この7行だけで調整できるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `FPS_MARGIN_RIGHT` | 右端からの余白を決めるため | 画面の右端から、いちばん長い表示の右端までの距離（20） | この手順で作る |
| 変数 | `FPS_MARGIN_TOP` | 上端からの余白を決めるため | 画面の上端から、文字の中央までの距離（20） | この手順で作る |
| 変数 | `FPS_SCALE` | 文字を小さく出すため | 文字の大きさの倍率（0.4 倍） | この手順で作る |
| 変数 | `FPS_COLOR` | 文字の色を決めるため | 白（少し透ける） | この手順で作る |
| 変数 | `FPS_SHADOW_COLOR` | 影の色を決めるため | 黒（少し透ける） | この手順で作る |
| 変数 | `FPS_SHADOW_OFFSET` | 影をずらす量を決めるため | 影をずらすピクセル数（1.5）。背景が明るくても読めるようにする | この手順で作る |

### 1-7. いちばん長い表示を決める

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **12行目** `constexpr float FPS_SHADOW_OFFSET = 1.5f;` の下に、3行足します（13〜15行目になります）。

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

- **なぜ:** 数字の幅は、`1` と `8` で違う。文字の右端をそろえると、数字が変わるたびに左右に動いて見づらい。そこで、いちばん長くなる文字を決めて、左端を固定する。
- **効果:** FPS の数字が変わっても、文字が左右に動かなくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `FPS_WIDEST_TEXT` | 左端の位置を決めるため | いちばん長くなる表示（`FPS 0000.0`）。この幅を測って、左端を決める | この手順で作る |
| 型 | `wchar_t` | 日本語を含む文字（ワイド文字）を1つ持つため | `const wchar_t*` は、ワイド文字の文字列を指すポインター | C++ の書き方 |
| C++ の書き方 | `L"…"` | ワイド文字の文字列を書くため | 文字列の前に `L` を付けると、`wchar_t` の文字列になる | C++ の書き方 |

### 1-8. FPS を出すかを決める

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **15行目**（`constexpr const wchar_t* FPS_WIDEST_TEXT = L"FPS 0000.0";` の下の空の行）の下、`}` の上に、3行足します（16〜18行目になります）。

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

- **なぜ:** 出すかどうかを、ほかのコードに散らさず、この1か所の `true` / `false` で決める。ビルドごとに切り替えたいときも、ここだけ直せばよい。
- **効果:** `SHOW_FPS` を `false` にするだけで、FPS の表示を全部止められる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `SHOW_FPS` | FPS を出すかを決めるため | `true` で出す、`false` で出さない | この手順で作る |

### 1-9. 準備の関数を書く

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **20行目**（`}` の下の空の行）の下（ファイルの最後）に、11行足します（21〜31行目になります）。

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

- **なぜ:** 見た目の設定は、1回決めれば変わらない。`Init` で1回だけ入れておけば、毎フレームは描くだけでよい。
- **効果:** 起動すると、FPS の文字が決めた見た目で、最初は「FPS --」と出るようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetAlign` | 文字を寄せる向きを決めるため | 左寄せ・中央寄せ・右寄せのどれかを入れる | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 値 | `Text::Align::Left` | 左寄せを表すため | `Text` の中の `enum class Align` の値（`Left`・`Center`・`Right`） | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 関数 | `SetScale` | 文字の大きさを決めるため | 大きさの倍率を入れる | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 関数 | `SetColor` | 文字の色を決めるため | 色（`Vector4`）を入れる | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 関数 | `SetShadow` | 文字の周りに影を付けるため | 影をずらすピクセル数と、影の色を入れる | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 関数 | `SetVisible` | 見えるかを決めるため | `false` の間は、`Draw` を呼んでも何も描かない | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)。`Text` が継承している） |
| 関数 | `SetText` | 出す文字を決めるため | 文字列を入れる。前と同じ文字なら何もしない | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |

### 1-10. 左端の位置を決める

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **22行目** `{` の下、`m_fps.SetAlign(Text::Align::Left);` の上に、7行足します（23〜29行目になります）。

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

- **なぜ:** 右寄せにすると、数字の幅が変わるたびに文字全体が動く。左寄せにして、左端を「いちばん長い表示が右端に収まる位置」に固定すれば、動かない。
- **効果:** FPS の文字が、右上にぴったり収まり、数字が変わってもガクガクしなくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `g_graphicsEngine` | 画面に描くエンジンの機能を、どこからでも使うため | 描画エンジン `GraphicsEngine` を指すグローバルなポインター | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |
| 関数 | `GetFontEngine` | 文字を扱うエンジンを取り出すため | 文字を描く `FontEngine` を返す | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |
| 関数 | `MeasureString` | 文字の幅を知るため | 文字列を大きさ 1 で描いたときの幅と高さ（ピクセル）を返す。`FPS 0000.0` なら、幅は `.x` | [UIの構成の手順](../UIの構成/UIの構成の手順.md) の 2-9 で足した（k2EngineLow（エンジン）の [FontEngine.h](../../../../k2EngineLow/graphics/font/FontEngine.h)） |
| 変数 | `widestWidth` | 画面に出る文字の幅を持つため | `FPS 0000.0` の幅に `FPS_SCALE`（0.4）を掛けた、実際の幅 | この手順で作る |
| 型 | `Vector2` | 横（x）と縦（y）の2つの値をまとめて持つため | 2次元の位置を表す。`{ x, y }` と書いて作る | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h) |
| 関数 | `SetPosition` | 文字を置く位置を決めるため | 位置（`Vector2`）を入れる。原点は画面の中心で、右が +x、上が +y | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)） |
| 値 | `UI_SPACE_WIDTH`・`UI_SPACE_HEIGHT` | 画面の大きさを知るため | UI を置く画面の幅（1920）と高さ（1080） | k2EngineLow（エンジン）の [k2EngineLowPreCompile.h](../../../../k2EngineLow/k2EngineLowPreCompile.h) |

### 1-11. 描く関数を書く

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **38行目**（`}` の下の空の行）の下（ファイルの最後）に、4行足します（39〜42行目になります）。

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

- **なぜ:** 文字を画面に出すには、毎フレーム `Draw` を呼ぶ必要がある。`DebugHud` は文字を持つだけなので、描くのは中の `Text` に任せる。
- **効果:** `UIManager` が `DebugHud::Draw` を呼べば、FPS の文字が画面に出るようになる。

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: UIManager に持たせて描く

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [UIManager.h](../../../../Game/Source/UI/UIManager.h) | 追加（`#include`、メンバー） |
| [UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) | 追加（準備・描く）・書き換え（コメント） |

### 2-1. DebugHud.h を読み込む

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **4行目** `#include "BossHud.h"` の下に、1行足します（5〜5行目になります）。

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

- **なぜ:** `UIManager.h` で `DebugHud` を使うために、先にその定義を読み込んでおく。
- **効果:** `UIManager` のメンバーとして `DebugHud` を持てるようになる。

### 2-2. DebugHud を持つメンバーを足す

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **37行目** `BossHud m_bossHud;` の下に、1行足します（38〜38行目になります）。

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

- **なぜ:** `PlayerHud`・`BossHud` と同じく、`UIManager` が持つ。ゲームの HUD と分けて、1つ別のメンバーにしておく。
- **効果:** `UIManager` が動くとき、開発用の表示も一緒に準備・更新・描画できるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_debugHud` | 開発用の表示を持つため | `DebugHud` 1つ。`UIManager` の準備・更新・描画から使う | この手順で作る |

### 2-3. 最初に DebugHud を準備する

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **8行目** `m_bossHud.Init();` の下に、1行足します（9〜9行目になります）。

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

- **なぜ:** `DebugHud` の見た目の設定は `Init` に書いた。`UIManager` が始まるときに、1回呼ぶ。
- **効果:** 最初に FPS の文字の位置と見た目が決まる。

### 2-4. 描く順のコメントを書き換える

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **22行目**を、次のように書き換えます。

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

- **なぜ:** いちばん上に描く理由を、描く順を書いたコメントに残しておく。
- **効果:** あとで描く順を変える人が、`DebugHud` を最後にしておく理由がわかる。

### 2-5. DebugHud をいちばん上に描く

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **24行目** `m_bossHud.Draw(rc);` の下に、1行足します（25〜25行目になります）。

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

- **なぜ:** 後に描いたものが上に重なる。いちばん最後に描けば、HUD の上でも FPS が隠れない。
- **効果:** FPS の文字が、ほかの HUD に隠されずに画面に出る。

**確かめ方:** 起動すると、右上に「FPS --」と出る。

---

## 手順3: 0.5秒ごとに平均の FPS を数えて出す

毎フレームの値は揺れて読みにくいので、0.5秒ごとにその間の平均を出します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) | 追加（`#include <chrono>`、`Update()`、時計とメンバー） |
| [DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) | 追加（平均を取る間隔、`Update()`） |
| [UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) | 追加（`Update()` を呼ぶ） |

### 3-1. 時間を測る仕組みを読み込む

[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) の **2行目**（`#pragma once` の下の空の行）の下、`#include "Text.h"` の上に、1行足します（3〜3行目になります）。

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

- **なぜ:** 実際の時間を測る道具（時計）は、標準ライブラリの `<chrono>` にある。使う前に読み込む。
- **効果:** `DebugHud.h` で、時計と時刻の型が使えるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `<chrono>` | 時間を測るため | 時計・時刻・時間の長さの型が入っているヘッダー | C++ の標準ライブラリ（`#include <chrono>` で読み込む） |

### 3-2. 毎フレームの関数を宣言する

[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) の **14行目** `void Init();` の下に、1行足します（15〜15行目になります）。

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

- **なぜ:** 平均を出すには、毎フレーム数える必要がある。毎フレーム呼ばれる関数を足す。
- **効果:** `UIManager` から、毎フレーム `DebugHud` を更新できるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Update` | 毎フレーム数えるため | フレームを数え、0.5秒ごとに FPS の文字を書き換える | この手順で作る |
| 引数 | `deltaTime` | 前のフレームからの時間を受け取るため | 文字の `Update` に渡す（FPS の計算には使わない） | `UIManager::Update` が渡す（`g_gameTime->GetFrameDeltaTime()`） |

### 3-3. 使う時計を決める

[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) の **18行目** `private:` の下、`Text m_fps;` の上に、2行足します（19〜20行目になります）。

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

- **なぜ:** 時計の名前を長く何度も書きたくない。別名を付けて、あとで時計を替えるときも1行で済むようにする。ゲームの経過時間は 1/30 秒で頭打ちになるので、PC の実際の時間を測る時計を使う。
- **効果:** `Clock` と書くだけで、時刻が戻らない時計が使えるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Clock` | 時計の名前を短くするため | `std::chrono::steady_clock` の別名 | この手順で作る |
| 型 | `std::chrono::steady_clock` | 実際の時間を測るため | PC の時刻を変えても戻らない時計 | C++ の標準ライブラリ（`<chrono>`。3-1 で読み込んだ） |

### 3-4. 平均を取るためのメンバーを足す

[DebugHud.h](../../../../Game/Source/UI/Hud/DebugHud.h) の **21行目** `Text m_fps;` の下に、3行足します（22〜24行目になります）。

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

- **なぜ:** 平均は「フレーム数 ÷ 時間」で出す。そのために、取り始めた時刻と、フレーム数を覚えておく。
- **効果:** 0.5秒ごとに、その間の平均の FPS を出せるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Clock::time_point` | ある時刻を持つため | 時計の「今の時刻」を入れる型 | C++ の標準ライブラリ（`<chrono>`） |
| メンバー変数 | `m_averageStart` | 時間を計る始まりを覚えるため | 平均を取り始めた時刻 | この手順で作る |
| メンバー変数 | `m_frameCount` | フレーム数を数えるため | 取り始めてから、`Update` が呼ばれた回数 | この手順で作る |
| メンバー変数 | `m_isStarted` | 最初の1回だけ時刻を決めるため | 最初の `Update` で取り始める時刻を決めたか | この手順で作る |

### 3-5. 平均を取る間隔を決める

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **14行目** `constexpr const wchar_t* FPS_WIDEST_TEXT = L"FPS 0000.0";` の下に、2行足します（15〜16行目になります）。

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

- **なぜ:** 毎フレームの値は揺れて読みにくい。一定の秒数ごとの平均なら、落ち着いて読める。秒数は名前を付けて置く。
- **効果:** 平均を取る間隔を、この1か所で変えられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `FPS_AVERAGE_SECONDS` | 平均を取る間隔を決めるため | この秒数（0.5）たつごとに、文字を書き換える | この手順で作る |

### 3-6. 毎フレームの関数の形を書く

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **40行目**（`}` の下の空の行）の下、`void DebugHud::Draw(RenderContext& rc)` の上に、8行足します（41〜48行目になります）。

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

- **なぜ:** `SHOW_FPS` が `false` のときは、時計を見る必要もない。最初に帰る。`m_fps.Update` は、文字の見た目を描く用に反映するために呼ぶ。
- **効果:** FPS を出さない設定のときは、何も負担がかからない。

### 3-7. 最初のフレームで、測り始める

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **45行目** `}` の下、`m_fps.Update(deltaTime);` の上に、5行足します（46〜50行目になります）。

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

- **なぜ:** `Init` の時点では、まだゲームが動いていない。最初の `Update` が呼ばれた時刻から数え始める。
- **効果:** 起動の準備にかかった時間が、最初の平均に入らなくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Clock::now` | 今の時刻を知るため | 今の時刻（`time_point`）を返す | C++ の標準ライブラリ（`<chrono>`） |
| 変数 | `now` | 今の時刻を持つため | この `Update` が呼ばれた時刻。1回の `Update` の中で、何度も同じ値を使う | この手順で作る |

### 3-8. 0.5秒たったら、平均を出して数え直す

[DebugHud.cpp](../../../../Game/Source/UI/Hud/DebugHud.cpp) の **50行目** `}` の下、`m_fps.Update(deltaTime);` の上に、9行足します（51〜59行目になります）。

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

- **なぜ:** 毎フレーム文字を書き換えると、揺れて読めない。0.5秒たったときだけ、その間のフレーム数を時間で割って書き換える。
- **効果:** FPS の数字が、0.5秒ごとに落ち着いて変わるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `elapsed` | 経った時間を持つため | 取り始めてからの時間（秒）。`now - m_averageStart` を秒の `float` にしたもの | この手順で作る |
| 型 | `std::chrono::duration<float>` | 時間の長さを秒の `float` で持つため | 時刻の差を、秒の `float` にする。`.count()` で数字を取り出す | C++ の標準ライブラリ（`<chrono>`） |
| 変数 | `text` | 出す文字を作るため | `FPS 60.0` のような文字を入れる、64 文字分の入れ物（`wchar_t` の配列） | この手順で作る |
| 関数 | `swprintf_s` | 数字から文字を作るため | 書式（`%.1f` は小数点以下1桁）に数字を入れて、`text` に書く。`60.0` のようになる | C の標準ライブラリの Visual Studio 版（`<wchar.h>`） |

### 3-9. UIManager から Update() を呼ぶ

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **17行目** `m_bossHud.Update(deltaTime);` の下に、1行足します（18〜18行目になります）。

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

- **なぜ:** `DebugHud::Update` は、呼ばれて初めて動く。ほかの HUD と同じように、`UIManager` の `Update` から毎フレーム呼ぶ。
- **効果:** FPS の数字が、0.5秒ごとに動くようになる。

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
| [Font.h](../../../../k2EngineLow/graphics/font/Font.h) | 追加（`MeasureString()` の宣言） |
| [Font.cpp](../../../../k2EngineLow/graphics/font/Font.cpp) | 追加（`MeasureString()` の中身） |
| [k2EngineLow.cpp](../../../../k2EngineLow/k2EngineLow.cpp) | 書き換え・コメントアウト（`EndFrame()` の FPS を描く処理） |

### 4-1. Font に、文字の幅を測る関数を宣言する

コメントアウトする中身で、右端をそろえるために使います。

[Font.h](../../../../k2EngineLow/graphics/font/Font.h) の **55行目** `}` の下、`private:` の上に、5行足します（56〜60行目になります）。

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

- **なぜ:** 文字の幅を測れるのは `FontEngine` だけで、`Font` からは呼べない。コメントアウトする中身（`m_fpsFont`）が使えるよう、`Font` から呼べる形にする。
- **効果:** `Font` の使い手が、文字の幅を測って、右寄せ・中央寄せができるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Font` | 文字を描くため | エンジンの文字描画のクラス（`Begin`・`Draw`・`End` の順に使う） | k2EngineLow（エンジン）の [Font.h](../../../../k2EngineLow/graphics/font/Font.h) |
| 関数 | `Font::MeasureString` | `Font` から文字の幅を知るため | 文字列を大きさ 1 で描いたときの幅と高さを返す。1-10 の `FontEngine` の `MeasureString` を呼ぶ | この手順で作る |

### 4-2. 文字の幅を測る関数を書く

[Font.cpp](../../../../k2EngineLow/graphics/font/Font.cpp) の **18行目** `}` の下、`void Font::Draw(` の上に、4行足します（19〜22行目になります）。

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
| 21 | [UIの構成の手順](../UIの構成/UIの構成の手順.md) の 2-9 で `FontEngine` に足した `MeasureString()` を呼ぶだけ |

- **なぜ:** 中身は、すでに `FontEngine` にある関数を呼ぶだけ。同じ処理を2か所に書かない。
- **効果:** `Font::MeasureString` が、`FontEngine` と同じ幅を返すようになる。

### 4-3. エンジンの FPS の表示を、右上に出す形にしてコメントアウトする

`#ifdef K2_DEBUG` から `#endif` までを、次のように書き換えます。

- 左上に大きさ 1 で描いていたのを、大きさ 0.5 で右上に描く形にする（`fpsScale`・`fpsShadowOffset`・`fpsWidth`・`fpsPosition` を使う）。
- 行の頭に `//` を付けてコメントアウトする（`#ifdef` / `#endif` の行も）。
- その上に、なぜ止めたかのコメント2行を書く。

[k2EngineLow.cpp](../../../../k2EngineLow/k2EngineLow.cpp) の **75〜83行目**を、次のように書き換えます（75〜90行目になります）。

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

- **なぜ:** ゲームの UI が FPS を出すので、エンジンの表示は止める。2つ出ると紛らわしい。消さずにコメントアウトしておけば、エンジンだけで動かすときに、コメントを外せばまた使える。
- **効果:** Debug ビルドでも、FPS が右上の1つだけになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| マクロ | `K2_DEBUG` | Debug のときだけ動かすため | `#ifdef K2_DEBUG` で囲んだところは、Debug・Preview ビルドだけに入る | `Game.vcxproj` のビルド設定（Debug・Preview で定義） |
| メンバー変数 | `m_fpsFont`・`m_fpsFontShadow` | FPS の文字と影を描くため | エンジンが持つ `Font` 2つ（`std::unique_ptr<Font>`） | k2EngineLow（エンジン）の [k2EngineLow.h](../../../../k2EngineLow/k2EngineLow.h)（もとからある） |
| 関数 | `Begin`・`End` | 文字の描画をまとめて始める・終えるため | `Font` で文字を描く前に `Begin`、描いたあとに `End` を呼ぶ | k2EngineLow（エンジン）の [Font.h](../../../../k2EngineLow/graphics/font/Font.h) |
| 関数 | `GetRenderContext` | 描くための入れ物を取り出すため | `Begin`・`End` に渡す `RenderContext` を返す | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |
| 変数 | `g_gameTime` | ゲームの時間を、どこからでも使うため | `GameTime` を指すグローバルなポインター | k2EngineLow（エンジン）の [k2EngineLow.h](../../../../k2EngineLow/k2EngineLow.h) |
| 関数 | `GetFrameDeltaTime` | 1フレームにかかった時間を知るため | 前のフレームからの時間（秒）を返す。1/30 秒より長くはならない | k2EngineLow（エンジン）の [GameTime.h](../../../../k2EngineLow/time/GameTime.h) |
| 関数 | `swprintf` | 数字から文字を作るため | `swprintf_s` と同じ。文字数の上限を渡さない古い形 | C の標準ライブラリ（`<wchar.h>`） |
| 変数 | `time` | 1フレームの時間を持つため | `GetFrameDeltaTime` の値。`1.0f / time` が FPS になる | この手順で作る（コメントの中。もとからあるコードの変数） |
| 変数 | `fpsScale`・`fpsShadowOffset` | 文字の大きさと影のずれを決めるため | 大きさ 0.5、影のずれ 1.5 ピクセル | この手順で作る（コメントの中） |
| 変数 | `fpsWidth` | 文字の幅を持つため | `MeasureString` の幅に `fpsScale` を掛けた、実際の幅 | この手順で作る（コメントの中） |
| 変数 | `fpsPosition` | 文字を描く位置を持つため | 文字の左上の位置。右端から `fpsWidth` だけ左にずらして、右端をそろえる | この手順で作る（コメントの中） |
| 関数 | `Font::Draw` | 文字を描くため | 引数は、文字・位置・色・回転・大きさ・基準点（`pivot`）の順 | k2EngineLow（エンジン）の [Font.h](../../../../k2EngineLow/graphics/font/Font.h) |

**確かめ方（これで出来上がり）:**

- k2EngineLow とゲームのビルドが通る。
- Debug ビルドで起動しても、エンジンの `FPS = xx.xx` が出ず、ゲームの `FPS xx.x` だけが右上に出る。
- Release ビルドでも、右上に FPS が出る。
