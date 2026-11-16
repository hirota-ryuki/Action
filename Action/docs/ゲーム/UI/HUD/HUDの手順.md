# HUDの手順

設計: [HUDの設計](HUDの設計.md)
経緯: [UIの経緯](../UIの経緯.md)

画面に Player の HP ゲージ・状態異常のアイコンと、ボスの HP ゲージを出すまでの手順です。

**出来上がるもの:** 画面下の中央に、Player の細い緑の HP ゲージと「今のHP / 最大HP」の数字が出ます。凍結中・燃焼中は、ゲージの右に水色・橙色の四角が出ます。ボスの HP ゲージ（画面上の中央）は、`UIManager::ShowBoss()` を呼んだときだけ出ます。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [UIの構成の手順](../UIの構成/UIの構成の手順.md) を済ませておく（ゲームの手順書の順番は [README の「ゲームを作る順番」](../../../README.md#ゲームを作る順番)）。この手順では、そこで作った `UIManager`・`Text`・`Icon`・`UISprite` を使う。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | 横向きのバーの部品 `Gauge` | ビルドが通る |
| 2 | `PlayerHud` で、Player の HP ゲージと数字を出す | 画面下に HP が出る |
| 3 | `PlayerHud` に、状態異常のアイコンを足す | 凍結・燃焼で四角が出る |
| 4 | `BossHud` で、ボスの名前と HP ゲージを出す | `ShowBoss()` で上に出る |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる変数・引数・関数・型・マクロ・C++ の書き方を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、このゲームで作ったもの・エンジン（k2Engine）の機能・C++ の標準ライブラリ・C++ の書き方のどれかがわかります。
- [前提知識](../../../前提知識.md) で「よく知っている」にあるもの（クラス・継承・ポインター・`override`・`constexpr`・`namespace`・`static_cast`・`nullptr` など）は、表に載せません。「まあまあ」「怪しい」「微妙」にあるものは載せます。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) | 新しく作る | 1 |
| [Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) | 新しく作る | 1 |
| [PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) | 新しく作る・追加 | 2・3 |
| [PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) | 新しく作る・追加 | 2・3 |
| [UIManager.h](../../../../Game/Source/UI/UIManager.h) | 追加 | 2・4 |
| [UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) | 追加 | 2・4 |
| [Game.cpp](../../../../Game/Source/Core/Game.cpp) | 追加 | 2 |
| [BossHud.h](../../../../Game/Source/UI/Hud/BossHud.h) | 新しく作る | 4 |
| [BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) | 新しく作る | 4 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: 横向きのバーの部品 Gauge を作る

HP・クールダウンなどに使う、横向きのバーです。位置は「バーの左端の中央」で、中身は左端から右へ伸び縮みします。減った分が少し残ってから追いかけて減る「遅れて減るバー」も出せます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) | 新しく作る（ゲージの設定、クラスの宣言） |
| [Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) | 新しく作る（割合を決める、遅れて減るバー、描く） |

`Source\UI\Elements` フィルターに `Gauge.h` と `Gauge.cpp` を作ります。

### 1-1. Gauge クラスの形を書く

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) を新しく作り、次の9行を書きます（1〜9行目）。

<!-- new: Action/Game/Source/UI/Elements/Gauge.h -->
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
#include "UISprite.h"

class Gauge : public UIElement
{
public:
};
```

</td></tr></table>

- **なぜ:** HP・クールダウンなど、横に伸び縮みするバーは何度も使う。1つの部品にしておけば、画面ごとに同じコードを書かずに済む。
- **効果:** `Gauge` を持って、割合を入れるだけで、バーが出せるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Gauge` | 横向きのバーを出すため | 割合（0〜1）に合わせて、中身が伸び縮みするバー | この手順で作る |
| 型 | `UIElement` | UI の部品に、位置・見えるか・透明度を持たせるため | 部品の基底クラス。`Gauge` はこれを継承する | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)） |

### 1-2. ゲージの設定を書く（大きさと色）

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) の **8行目** `public:` の下、`};` の上に、10行足します（9〜18行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.h 9 -->
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
```

</td><td>

```cpp
	struct Settings
	{
		float width = 400.0f;
		float height = 20.0f;
		float borderSize = 0.0f;                            // 枠の太さ。0 なら枠を描かない。
		Vector4 borderColor = { 0.0f, 0.0f, 0.0f, 0.8f };
		Vector4 backColor = { 0.0f, 0.0f, 0.0f, 0.6f };     // 空きの部分。
		Vector4 barColor = { 0.85f, 0.15f, 0.15f, 1.0f };   // 中身。
	};

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11〜12 | 幅と高さ |
| 13 | 枠の太さ。0 なら枠を描かない |
| 15 | 空いている部分の色 |
| 16 | 中身の色 |

- **なぜ:** 大きさや色は、使う場所ごとに違う。1つの `Settings` にまとめて渡せば、引数を何個も並べずに済み、決めない項目は初期値のままにできる。
- **効果:** `Settings` の使いたい項目だけを書き換えて、好きな大きさ・色のゲージが作れるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Settings` | ゲージの設定をまとめて渡すため | 大きさ・色などを持つ構造体。`Gauge::Settings` と書く | この手順で作る |
| メンバー変数 | `width`・`height` | ゲージの大きさを決めるため | 幅と高さ（ピクセル） | この手順で作る |
| メンバー変数 | `borderSize` | 枠の太さを決めるため | 枠の太さ（ピクセル）。0 なら枠を描かない | この手順で作る |
| メンバー変数 | `borderColor` | 枠の色を決めるため | 枠の色（`Vector4`） | この手順で作る |
| メンバー変数 | `backColor` | 空いている部分の色を決めるため | HP が減って空になった部分の色 | この手順で作る |
| メンバー変数 | `barColor` | 中身の色を決めるため | 中身（HP が残っている部分）の色 | この手順で作る |

### 1-3. 画像と、遅れて減るバーの設定を足す

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) の **16行目** `Vector4 barColor = { 0.85f, 0.15f, 0.15f, 1.0f };   // 中身。` の下に、5行足します（17〜21行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.h 17 -->
<table class="code-lines"><tr><td>

```cpp
17
18
19
20
21
```

</td><td>

```cpp
		const char* barImagePath = UISprite::WHITE_PATH;     // 中身と遅れて減るバーの画像。色はこれに掛ける。
		bool useTrail = false;                               // 遅れて減るバーを使うか。
		Vector4 trailColor = { 1.0f, 1.0f, 1.0f, 0.9f };
		float trailDelay = 0.5f;                             // 減ってから、遅れて減るバーが縮み始めるまでの秒数。
		float trailSpeed = 0.5f;                             // 遅れて減るバーが縮む速さ（1秒にゲージ全体の何割か）。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 17 | 中身と遅れて減るバーの画像。色はこれに掛ける |
| 18 | 遅れて減るバーを使うか |
| 20 | 減ってから、遅れて減るバーが縮み始めるまでの時間 |
| 21 | 縮む速さ（1秒にゲージ全体の何割か） |

- **なぜ:** 中身の画像を替えたい（光の当たった立体的な画像を使うなど）、遅れて減るバーを使いたい、というゲージもある。使わないゲージは何も書かなくてよいように、初期値を決めておく。
- **効果:** `useTrail` を `true` にするだけで、減った分が残ってから追いかけて縮むバーが使えるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `barImagePath` | 中身の画像を決めるため | 画像ファイルのパス。初期値は白い画像 | この手順で作る |
| 値 | `UISprite::WHITE_PATH` | 白い画像のパスを使うため | 4×4 の白い画像。色を掛けて、ゲージや四角に使う | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UISprite.h](../../../../Game/Source/UI/Elements/UISprite.h)） |
| メンバー変数 | `useTrail` | 遅れて減るバーを使うか決めるため | `true` で使う | この手順で作る |
| メンバー変数 | `trailColor` | 遅れて減るバーの色を決めるため | 遅れて減るバーの色 | この手順で作る |
| メンバー変数 | `trailDelay` | 縮み始めるまでの時間を決めるため | 減ってから縮み始めるまでの秒数 | この手順で作る |
| メンバー変数 | `trailSpeed` | 縮む速さを決めるため | 1秒にゲージ全体の何割縮むか（0.5 なら 50%） | この手順で作る |

### 1-4. 準備と、割合を決める関数を宣言する

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) の **23行目**（`};` の下の空の行）の下、`};` の上に、6行足します（24〜29行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.h 24 -->
<table class="code-lines"><tr><td>

```cpp
24
25
26
27
28
29
```

</td><td>

```cpp
	void Init(const Settings& settings);
	// 中身の割合。0〜1 に収めてから使う。
	// 増えたときは遅れて減るバーもすぐそろえる（残らないように）。
	void SetRate(float rate);
	// 遅れて減るバーを今の割合にそろえる。表示し始めるときに呼ぶ（減っていく演出にならないように）。
	void ResetTrail();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 27 | 中身の割合（0〜1）を決める |
| 29 | 遅れて減るバーを今の割合にそろえる。表示し始めるときに呼ぶ |

- **なぜ:** 設定は、`Init` で1回だけ渡す。割合は、HP が変わるたびに入れる。入れ方を関数にしておけば、入れたときに 0〜1 に収めるなどの決まりを守らせられる。
- **効果:** 使う側は、`Init` と `SetRate` を呼ぶだけで、ゲージを動かせるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Init` | 使う前の準備をするため | 設定を受け取り、4枚の画像を読む | この手順で作る |
| 引数 | `settings` | 設定を受け取るため | `Settings` を `const` の参照で受け取る（コピーしない） | 使う側が決める |
| 関数 | `SetRate` | 中身の長さを決めるため | 割合（0〜1）を入れる。ゲージの長さが変わる | この手順で作る |
| 引数 | `rate` | 割合を受け取るため | 1 なら満タン、0 なら空 | 使う側が決める |
| 関数 | `ResetTrail` | 遅れて減るバーを今の割合にそろえるため | 表示し始めるときに呼ぶ。バーが減っていく演出にならない | この手順で作る |

### 1-5. 割合と設定を読む関数を足す

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) の **29行目** `void ResetTrail();` の下に、9行足します（30〜38行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.h 30 -->
<table class="code-lines"><tr><td>

```cpp
30
31
32
33
34
35
36
37
38
```

</td><td>

```cpp
	float GetRate() const
	{
		return m_rate;
	}
	const Settings& GetSettings() const
	{
		return m_settings;
	}

```

</td></tr></table>

- **なぜ:** 割合や設定は、ゲージの外から読みたいことがある。値をそのまま公開せず、読むだけの関数（`const`）にして、外から書き換えられないようにする。
- **効果:** `Gauge` の今の割合と設定を、安全に読めるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `GetRate` | 今の割合を読むため | `m_rate` を返す | この手順で作る |
| 関数 | `GetSettings` | 今の設定を読むため | `m_settings` を返す。`const &` なので読むだけ | この手順で作る |
| メンバー変数 | `m_rate` | 中身の割合を持つため | 今の割合（0〜1）。1-8 で宣言する | この手順で作る |
| メンバー変数 | `m_settings` | 設定を持つため | `Init` で受け取った設定。1-8 で宣言する | この手順で作る |

### 1-6. 更新と、描く関数を宣言する

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) の **38行目**（`}` の下の空の行）の下、`};` の上に、5行足します（39〜43行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.h 39 -->
<table class="code-lines"><tr><td>

```cpp
39
40
41
42
43
```

</td><td>

```cpp
	void Update(float deltaTime) override;

protected:
	void OnDraw(RenderContext& rc) override;

```

</td></tr></table>

- **なぜ:** 毎フレームの動きと、描く動きは、`UIElement` で決まった形がある。それに合わせて `override` する。
- **効果:** `Gauge` を、ほかの部品と同じ呼び方で動かせるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Update` | 毎フレーム、画像を置き直すため | 割合に合わせて、4枚の画像の位置・大きさ・色を決める | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)）（`Gauge` が `override` する） |
| 引数 | `deltaTime` | 前のフレームからの時間を受け取るため | 遅れて減るバーを、時間に合わせて縮めるために使う（秒） | `UIManager` が渡す（`g_gameTime->GetFrameDeltaTime()`） |
| 関数 | `OnDraw` | 画像を実際に描くため | `UIElement::Draw` が、見えるときだけ呼ぶ。中身は派生クラスが書く | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)）（`Gauge` が `override` する） |

### 1-7. バーを置く関数を宣言する

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) の **43行目**（`void OnDraw(RenderContext& rc) override;` の下の空の行）の下、`};` の上に、4行足します（44〜47行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.h 44 -->
<table class="code-lines"><tr><td>

```cpp
44
45
46
47
```

</td><td>

```cpp
private:
	// 左端の中央を x の位置に合わせ、横の長さを rate 倍にして置く。
	void UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color);

```

</td></tr></table>

- **なぜ:** 枠・空き・遅れて減るバー・中身の4枚は、置き方がほとんど同じ。1つの関数にまとめれば、同じコードを4回書かずに済む。
- **効果:** 4枚を同じ手順で置けるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `UpdateBar` | 1枚の画像を置くため | 画像の位置と大きさ（長さ）と色を決める | この手順で作る |
| 引数 | `sprite` | 置く画像を受け取るため | `SpriteRender` を参照で受け取る | 使う側が決める（`Gauge` の4枚の画像） |
| 引数 | `x` | 左端の位置を受け取るため | バーの左端の x 座標 | 使う側が決める |
| 引数 | `color` | 色を受け取るため | 画像に掛ける色 | 使う側が決める |

### 1-8. 持つ値を足す

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) の **47行目**（`void UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color);` の下の空の行）の下、`};` の上に、8行足します（48〜55行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.h 48 -->
<table class="code-lines"><tr><td>

```cpp
48
49
50
51
52
53
54
55
```

</td><td>

```cpp
	Settings m_settings;
	SpriteRender m_border;
	SpriteRender m_back;
	SpriteRender m_trail;
	SpriteRender m_bar;
	float m_rate = 1.0f;
	float m_trailRate = 1.0f;
	float m_trailWait = 0.0f;  // 遅れて減るバーが縮み始めるまでの残り秒数。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 49〜52 | 下から 枠 → 空き → 遅れて減るバー → 中身 の4枚の画像 |
| 53 | 中身の割合 |
| 54 | 遅れて減るバーの割合 |
| 55 | 遅れて減るバーが縮み始めるまでの残り時間 |

- **なぜ:** 4枚の画像を別々に持てば、割合・色を1枚ずつ決められる。割合などの値は、毎フレーム使うのでメンバー変数に持つ。
- **効果:** `Update` と `OnDraw` が、4枚の画像と割合を使えるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_border` | 枠の画像を持つため | 枠の `SpriteRender` | この手順で作る |
| メンバー変数 | `m_back` | 空き部分の画像を持つため | 空いている部分の `SpriteRender` | この手順で作る |
| メンバー変数 | `m_trail` | 遅れて減るバーの画像を持つため | 遅れて減るバーの `SpriteRender` | この手順で作る |
| メンバー変数 | `m_bar` | 中身の画像を持つため | 中身の `SpriteRender` | この手順で作る |
| メンバー変数 | `m_trailRate` | 遅れて減るバーの長さを持つため | 遅れて減るバーの今の割合 | この手順で作る |
| メンバー変数 | `m_trailWait` | 縮み始めるまで待つため | 縮み始めるまでの残りの秒数 | この手順で作る |

### 1-9. コメントを書く

[Gauge.h](../../../../Game/Source/UI/Elements/Gauge.h) の **5行目**（`#include "UISprite.h"` の下の空の行）の下、`class Gauge : public UIElement` の上に、4行足します（6〜9行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.h 6 -->
<table class="code-lines"><tr><td>

```cpp
6
7
8
9
```

</td><td>

```cpp
// 横向きのバー。SetRate(0〜1) で中身の長さを変える（HP・クールダウンなど）。
// 位置は「バーの左端の中央」（枠は含まない）。中身は左端から右へ伸びる。
// 下から 枠 → 空き → 遅れて減るバー → 中身 の順に重ねて描く。
// 遅れて減るバー（useTrail）をオンにすると、減った分が残り、少し待ってから追いかけて縮む。
```

</td></tr></table>

- **なぜ:** 画像を4枚重ねる順や、位置の決め方は、コードだけを見ても気づきにくい。クラスの上に書いておく。
- **効果:** あとでこのクラスを読む人が、使い方と重なる順を、すぐにわかる。

### 1-10. Gauge.cpp を作り、使うものを読み込む

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) を新しく作り、次の5行を書きます（1〜5行目）。

<!-- new: Action/Game/Source/UI/Elements/Gauge.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
```

</td><td>

```cpp
#include "stdafx.h"
#include "Gauge.h"

#include "UISprite.h"

```

</td></tr></table>

- **なぜ:** `.cpp` の1行目は `stdafx.h` と決まっている。次に、このクラスの `.h` と、使う `UISprite.h` を読み込む。
- **効果:** `Gauge.cpp` で、`Gauge` と白い画像のパスが使えるようになる。

### 1-11. 画像の基準を決める

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **5行目**（`#include "UISprite.h"` の下の空の行）の下（ファイルの最後）に、6行足します（6〜11行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 6 -->
<table class="code-lines"><tr><td>

```cpp
6
7
8
9
10
11
```

</td><td>

```cpp
namespace
{
	// 左端の中央を基準にする（ピボットの x は 0 が左端、0.5 が中央）。
	const Vector2 LEFT_CENTER_PIVOT = { 0.0f, 0.5f };
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 9 | 画像の基準を左端の中央にする。横の大きさを割合にすると、左端から右へ伸び縮みする |

- **なぜ:** 画像の「基準の点」を左端の中央にしておくと、大きさを割合にしたとき、左端を動かさずに右へ伸び縮みする。中央が基準だと、両側へ伸び縮みしてしまう。
- **効果:** ゲージの中身が、左端から右へ伸び縮みするようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `LEFT_CENTER_PIVOT` | 画像の基準を決めるため | 基準の点（ピボット）。x は 0 が左端、0.5 が中央。y は 0.5 が上下の中央。`{ 0.0f, 0.5f }` で左端の中央 | この手順で作る |
| 型 | `Vector2` | 横と縦の2つの値をまとめて持つため | 基準の点や、画面上の位置を表す。`{ x, y }` と書いて作る | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h) |

### 1-12. 準備の関数を書き、4枚の画像を読む

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **11行目**（`}` の下の空の行）の下（ファイルの最後）に、10行足します（12〜21行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 12 -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp
void Gauge::Init(const Settings& settings)
{
	m_settings = settings;
	const float border = m_settings.borderSize;
	m_border.Init(UISprite::WHITE_PATH, m_settings.width + border * 2.0f, m_settings.height + border * 2.0f);
	m_back.Init(UISprite::WHITE_PATH, m_settings.width, m_settings.height);
	m_trail.Init(m_settings.barImagePath, m_settings.width, m_settings.height);
	m_bar.Init(m_settings.barImagePath, m_settings.width, m_settings.height);
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 16 | 枠は、上下左右に枠の太さだけ大きくする |
| 17 | 空きは白い画像 |
| 18〜19 | 遅れて減るバーと中身は、設定の画像 |

- **なぜ:** 4枚の画像は、ゲージの設定（幅・高さ・枠の太さ）で大きさが決まる。`Init` で、設定を受け取ってから画像を読む。
- **効果:** 設定どおりの大きさの、4枚の画像ができる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `border` | 枠の太さを短く書くため | `m_settings.borderSize` のコピー。枠は上下左右に `border` ずつ大きくするので、幅・高さに `border * 2` を足す | この手順で作る |

### 1-13. 画像の基準を左端の中央にする

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **19行目** `m_bar.Init(m_settings.barImagePath, m_settings.width, m_settings.height);` の下に、4行足します（20〜23行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 20 -->
<table class="code-lines"><tr><td>

```cpp
20
21
22
23
```

</td><td>

```cpp
	m_border.SetPivot(LEFT_CENTER_PIVOT);
	m_back.SetPivot(LEFT_CENTER_PIVOT);
	m_trail.SetPivot(LEFT_CENTER_PIVOT);
	m_bar.SetPivot(LEFT_CENTER_PIVOT);
```

</td></tr></table>

- **なぜ:** 4枚の画像の基準を、1-11 で決めた左端の中央にそろえる。
- **効果:** 4枚が、左端をそろえて重なり、右へ伸び縮みするようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetPivot` | 画像の基準の点を決めるため | 画像の位置・拡大・回転の基準になる点を入れる | k2Engine（エンジン）の [SpriteRender.h](../../../../k2Engine/graphics/SpriteRender.h) |

### 1-14. 割合を決める関数を書く

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **25行目**（`}` の下の空の行）の下（ファイルの最後）に、9行足します（26〜34行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 26 -->
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
```

</td><td>

```cpp
void Gauge::SetRate(float rate)
{
	rate = min(1.0f, max(0.0f, rate));
	m_rate = rate;
	if (m_trailRate < m_rate) {
		m_trailRate = m_rate;
	}
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 28 | 0〜1 の外に出ないようにする |
| 30〜32 | 増えたときは、遅れて減るバーもすぐそろえる（残らないように） |

- **なぜ:** 範囲の外の割合が入っても、バーがはみ出したりへこんだりしないよう、0〜1 に収める。増えたときは、遅れて減るバーが中身より短くならないよう、すぐそろえる。
- **効果:** どんな値を入れても、ゲージが正しく出る。回復したときに、白いバーが残らない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `min`・`max` | 値を範囲に収めるため | 2つの値の小さい方・大きい方を返す。`min(1.0f, max(0.0f, rate))` で、`rate` が 0〜1 に収まる（`std::min`・`std::max` と同じ使い方） | Windows の API（`<windows.h>` のマクロ） |

### 1-15. 減ったときは、遅れて減るバーを待たせる

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **28行目** `rate = min(1.0f, max(0.0f, rate));` の下に、4行足します（29〜32行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 29 -->
<table class="code-lines"><tr><td>

```cpp
29
30
31
32
```

</td><td>

```cpp
	if (rate < m_rate) {
		// 減った。白いバーは今の位置に残し、少し待ってから縮める。
		m_trailWait = m_settings.trailDelay;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 29〜32 | 減ったら、遅れて減るバーは今の位置に残し、少し待ってから縮める |

- **なぜ:** HP が減ったとき、白いバーをすぐ縮めると、いくつ減ったかが見えない。減った位置に残して、少し待ってから縮める。
- **効果:** 減った量が、白いバーの長さで見えるようになる。

### 1-16. 遅れて減るバーをそろえる関数を書く

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **38行目**（`}` の下の空の行）の下（ファイルの最後）に、6行足します（39〜44行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 39 -->
<table class="code-lines"><tr><td>

```cpp
39
40
41
42
43
44
```

</td><td>

```cpp
void Gauge::ResetTrail()
{
	m_trailRate = m_rate;
	m_trailWait = 0.0f;
}

```

</td></tr></table>

- **なぜ:** ボスが出てくるときなど、満タンから始めたいときがある。白いバーを今の割合にそろえる関数があれば、減っていく演出にならない。
- **効果:** 表示し始めに、白いバーが縮む動きが出なくなる。

### 1-17. 毎フレームの関数を書き、4枚を置く

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **44行目**（`}` の下の空の行）の下（ファイルの最後）に、10行足します（45〜54行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 45 -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp
void Gauge::Update(float deltaTime)
{

	// 枠は、中身より borderSize だけ左から始まり、上下左右に borderSize ずつはみ出す。
	UpdateBar(m_border, m_position.x - m_settings.borderSize, 1.0f, m_settings.borderColor);
	UpdateBar(m_back, m_position.x, 1.0f, m_settings.backColor);
	UpdateBar(m_trail, m_position.x, m_trailRate, m_settings.trailColor);
	UpdateBar(m_bar, m_position.x, m_rate, m_settings.barColor);
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 49 | 枠は、中身より枠の太さだけ左から始まる |
| 50〜52 | 空き（いつも 1）、遅れて減るバー、中身 |

- **なぜ:** 枠は、中身より枠の太さだけ左から始まる。4枚を同じ関数で置けば、位置の決め方を1か所に書ける。
- **効果:** 毎フレーム、割合に合わせて4枚が置き直される。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_position` | 部品の位置を持つため | `SetPosition` で入れた位置。`Gauge` は左端の中央の位置として使う | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)）（`protected`） |

### 1-18. 遅れて減るバーを縮める

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **46行目** `{` の下に、11行足します（47〜57行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 47 -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp
	if (m_settings.useTrail && m_trailRate > m_rate) {
		if (m_trailWait > 0.0f) {
			m_trailWait -= deltaTime;
		}
		else {
			m_trailRate = max(m_rate, m_trailRate - m_settings.trailSpeed * deltaTime);
		}
	}
	else {
		m_trailRate = m_rate;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 47〜54 | 遅れて減るバーが中身より長ければ、待ち時間を減らし、待ち終わったら縮める。中身より短くはしない |
| 55〜57 | 使わないか、追いついていれば、中身と同じにする |

- **なぜ:** 白いバーは、待ち時間が終わってから、一定の速さで縮める。中身より短くすると見た目がおかしいので、中身の割合を下限にする。
- **効果:** 減ったあと、白いバーが少し待ってから、中身に追いつくように縮む。

### 1-19. バーを置く関数を書く

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **65行目**（`}` の下の空の行）の下（ファイルの最後）に、8行足します（66〜73行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 66 -->
<table class="code-lines"><tr><td>

```cpp
66
67
68
69
70
71
72
73
```

</td><td>

```cpp
void Gauge::UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color)
{
	sprite.SetPosition(Vector3(x, m_position.y, 0.0f));
	sprite.SetScale(Vector3(rate, 1.0f, 1.0f));
	sprite.SetMulColor(ApplyAlpha(color));
	sprite.Update();
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 69 | 横の大きさを割合にする。基準が左端なので、左端から伸び縮みする |

- **なぜ:** 4枚とも、画像の置き方は同じ（位置・長さ・色）。1つの関数にまとめる。透明度は、部品全体で決めた値を色のアルファに掛ける。
- **効果:** 透明度を変えると、4枚が同じ透明度で見えるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetMulColor` | 画像に色を掛けるため | 画像の色に掛ける色を入れる。白い画像に色を付けるのに使う | k2Engine（エンジン）の [SpriteRender.h](../../../../k2Engine/graphics/SpriteRender.h) |
| 関数 | `ApplyAlpha` | 部品の透明度を色に反映するため | 色のアルファに、部品の透明度を掛けた色を返す | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)）（`protected`） |

### 1-20. 描く関数を書く

[Gauge.cpp](../../../../Game/Source/UI/Elements/Gauge.cpp) の **73行目**（`}` の下の空の行）の下（ファイルの最後）に、12行足します（74〜85行目になります）。

<!-- add: Action/Game/Source/UI/Elements/Gauge.cpp 74 -->
<table class="code-lines"><tr><td>

```cpp
74
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
```

</td><td>

```cpp
void Gauge::OnDraw(RenderContext& rc)
{
	// 後に描いたものが上に重なる。枠 → 空き → 遅れて減るバー → 中身の順。
	if (m_settings.borderSize > 0.0f) {
		m_border.Draw(rc);
	}
	m_back.Draw(rc);
	if (m_settings.useTrail) {
		m_trail.Draw(rc);
	}
	m_bar.Draw(rc);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 76〜84 | あとに描いたものが上に重なる。枠 → 空き → 遅れて減るバー → 中身の順 |

- **なぜ:** 後に描いたものが上に重なる。枠 → 空き → 遅れて減るバー → 中身の順に描くと、中身がいちばん上に見える。使わない画像は描かない。
- **効果:** 4枚が正しい順に重なって見える。

### 出来上がった Gauge.h

<!-- full: Action/Game/Source/UI/Elements/Gauge.h -->
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
```

</td><td>

```cpp
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
		const char* barImagePath = UISprite::WHITE_PATH;     // 中身と遅れて減るバーの画像。色はこれに掛ける。
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
```

</td></tr></table>

### 出来上がった Gauge.cpp

<!-- full: Action/Game/Source/UI/Elements/Gauge.cpp -->
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
67
68
69
70
71
72
73
74
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
```

</td><td>

```cpp
#include "stdafx.h"
#include "Gauge.h"

#include "UISprite.h"

namespace
{
	// 左端の中央を基準にする（ピボットの x は 0 が左端、0.5 が中央）。
	const Vector2 LEFT_CENTER_PIVOT = { 0.0f, 0.5f };
}

void Gauge::Init(const Settings& settings)
{
	m_settings = settings;
	const float border = m_settings.borderSize;
	m_border.Init(UISprite::WHITE_PATH, m_settings.width + border * 2.0f, m_settings.height + border * 2.0f);
	m_back.Init(UISprite::WHITE_PATH, m_settings.width, m_settings.height);
	m_trail.Init(m_settings.barImagePath, m_settings.width, m_settings.height);
	m_bar.Init(m_settings.barImagePath, m_settings.width, m_settings.height);
	m_border.SetPivot(LEFT_CENTER_PIVOT);
	m_back.SetPivot(LEFT_CENTER_PIVOT);
	m_trail.SetPivot(LEFT_CENTER_PIVOT);
	m_bar.SetPivot(LEFT_CENTER_PIVOT);
}

void Gauge::SetRate(float rate)
{
	rate = min(1.0f, max(0.0f, rate));
	if (rate < m_rate) {
		// 減った。白いバーは今の位置に残し、少し待ってから縮める。
		m_trailWait = m_settings.trailDelay;
	}
	m_rate = rate;
	if (m_trailRate < m_rate) {
		m_trailRate = m_rate;
	}
}

void Gauge::ResetTrail()
{
	m_trailRate = m_rate;
	m_trailWait = 0.0f;
}

void Gauge::Update(float deltaTime)
{
	if (m_settings.useTrail && m_trailRate > m_rate) {
		if (m_trailWait > 0.0f) {
			m_trailWait -= deltaTime;
		}
		else {
			m_trailRate = max(m_rate, m_trailRate - m_settings.trailSpeed * deltaTime);
		}
	}
	else {
		m_trailRate = m_rate;
	}

	// 枠は、中身より borderSize だけ左から始まり、上下左右に borderSize ずつはみ出す。
	UpdateBar(m_border, m_position.x - m_settings.borderSize, 1.0f, m_settings.borderColor);
	UpdateBar(m_back, m_position.x, 1.0f, m_settings.backColor);
	UpdateBar(m_trail, m_position.x, m_trailRate, m_settings.trailColor);
	UpdateBar(m_bar, m_position.x, m_rate, m_settings.barColor);
}

void Gauge::UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color)
{
	sprite.SetPosition(Vector3(x, m_position.y, 0.0f));
	sprite.SetScale(Vector3(rate, 1.0f, 1.0f));
	sprite.SetMulColor(ApplyAlpha(color));
	sprite.Update();
}

void Gauge::OnDraw(RenderContext& rc)
{
	// 後に描いたものが上に重なる。枠 → 空き → 遅れて減るバー → 中身の順。
	if (m_settings.borderSize > 0.0f) {
		m_border.Draw(rc);
	}
	m_back.Draw(rc);
	if (m_settings.useTrail) {
		m_trail.Draw(rc);
	}
	m_bar.Draw(rc);
}
```

</td></tr></table>

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: PlayerHud で、Player の HP ゲージと数字を出す

Player の値を読んで、自分の部品に入れます。`UIElement` は継承しません（自分では描かず、部品を持つだけ）。`UIManager` が持ちます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) | 新しく作る（Player の HUD） |
| [PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) | 新しく作る（HP ゲージと数字） |
| [UIManager.h](../../../../Game/Source/UI/UIManager.h) | 追加（`#include`、Player を渡す関数、メンバー） |
| [UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) | 追加（準備・更新・描く） |
| [Game.cpp](../../../../Game/Source/Core/Game.cpp) | 追加（Player を渡す） |

`Source\UI` フィルターの下に `Hud` フィルターを作り、`PlayerHud.h` と `PlayerHud.cpp` を作ります。

### 2-1. PlayerHud クラスの形を書く

[PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) を新しく作り、次の12行を書きます（1〜12行目）。

<!-- new: Action/Game/Source/UI/Hud/PlayerHud.h -->
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
```

</td><td>

```cpp
#pragma once

#include "Gauge.h"
#include "Text.h"

class Player;

class PlayerHud
{
public:
	void Init();
};
```

</td></tr></table>

- **なぜ:** Player の HP の表示を、`PlayerHud` というクラスにまとめる。ゲージや文字の部品を持つだけにして、Player のことは Player に聞く。
- **効果:** `UIManager` が `PlayerHud` を1つ持てば、Player の HUD を出せるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `PlayerHud` | Player の HUD をまとめるため | HP ゲージ・数字・状態異常のアイコンを持つクラス | この手順で作る |
| 型 | `Text` | 文字を出すため | 位置・大きさ・色を決めて文字を描く部品 | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 型 | `Player` | HP などを読むため | プレイヤーのクラス。ここでは前方宣言だけして、`.cpp` で読み込む | [キャラクターの構成の手順](../../キャラクター/キャラクターの構成/キャラクターの構成の手順.md) で作った（[Player.h](../../../../Game/Source/Player/Player.h)） |

### 2-2. Player を渡す関数と、更新・描く関数を足す

[PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) の **11行目** `void Init();` の下に、7行足します（12〜18行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.h 12 -->
<table class="code-lines"><tr><td>

```cpp
12
13
14
15
16
17
18
```

</td><td>

```cpp
	// 表示する Player。読むだけ（const）。nullptr なら何も描かない。
	void SetTarget(const Player* player)
	{
		m_player = player;
	}
	void Update(float deltaTime);
	void Draw(RenderContext& rc);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13〜16 | 表示する Player。読むだけ（`const`）。`nullptr` なら何も描かない |

- **なぜ:** Player は読むだけなので `const` のポインターにして、HUD が書き換えられないようにする。`nullptr` の間は、何も描かない。
- **効果:** Player が作られる前でも、`PlayerHud` が壊れずに動く。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetTarget` | 表示する Player を決めるため | Player のポインターを覚える | この手順で作る |
| 引数 | `player` | 表示する Player を受け取るため | `const Player*`（読むだけ） | 使う側が決める（`UIManager::SetPlayer` から渡される） |
| メンバー変数 | `m_player` | 表示する Player を持つため | `SetTarget` で入れた Player。入れるまでは `nullptr` | この手順で作る |

### 2-3. 持つ値を足す

[PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) の **18行目** `void Draw(RenderContext& rc);` の下に、5行足します（19〜23行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.h 19 -->
<table class="code-lines"><tr><td>

```cpp
19
20
21
22
23
```

</td><td>

```cpp

private:
	const Player* m_player = nullptr;
	Gauge m_hp;
	Text m_hpNumber;  // 「今のHP / 最大HP」。
```

</td></tr></table>

- **なぜ:** HP ゲージと数字は、`Init` で見た目を決め、`Update`・`Draw` で使う。メンバー変数にすれば、関数をまたいで使える。
- **効果:** `PlayerHud` が、HP ゲージと数字を持てるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_hp` | HP ゲージを持つため | `Gauge` 1つ | この手順で作る |
| メンバー変数 | `m_hpNumber` | HP の数字を持つため | 「今のHP / 最大HP」を出す `Text` | この手順で作る |

### 2-4. コメントを書く

[PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) の **7行目**（`class Player;` の下の空の行）の下、`class PlayerHud` の上に、3行足します（8〜10行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.h 8 -->
<table class="code-lines"><tr><td>

```cpp
8
9
10
```

</td><td>

```cpp
// Player 用の HUD。Player の値を読んで、自分の部品に入れる。
// 画面下の中央に、細い緑の HPゲージと、その下に「今のHP / 最大HP」の数字を出す。状態異常のアイコンはゲージの右。
// UIElement は継承しない（自分では描かず、部品を持つだけ）。UIManager が持つ。
```

</td></tr></table>

- **なぜ:** どこに何を出すクラスなのかを、クラスの上に書いておく。
- **効果:** あとでこのクラスを読む人が、画面のどこに出るかをすぐにわかる。

### 2-5. PlayerHud.cpp を作り、使うものを読み込む

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) を新しく作り、次の6行を書きます（1〜6行目）。

<!-- new: Action/Game/Source/UI/Hud/PlayerHud.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
```

</td><td>

```cpp
#include "stdafx.h"
#include "PlayerHud.h"

#include "Player.h"
#include "UISprite.h"

```

</td></tr></table>

- **なぜ:** `.cpp` の1行目は `stdafx.h`。`Player` の HP を読むので `Player.h` を、白い画像のパスを使うので `UISprite.h` を読み込む。
- **効果:** `PlayerHud.cpp` で、`Player` と `UISprite` が使えるようになる。

### 2-6. 値を置く場所を作る

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **6行目**（`#include "UISprite.h"` の下の空の行）の下（ファイルの最後）に、4行足します（7〜10行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 7 -->
<table class="code-lines"><tr><td>

```cpp
7
8
9
10
```

</td><td>

```cpp
namespace
{
}

```

</td></tr></table>

- **なぜ:** この `.cpp` だけで使う値は、無名の `namespace` に置く。ほかのファイルから見えず、名前もぶつからない。
- **効果:** 見た目の数字を、1か所にまとめて置ける。

### 2-7. HP ゲージの大きさと置く高さを決める

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **8行目** `{` の下、`}` の上に、5行足します（9〜13行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 9 -->
<table class="code-lines"><tr><td>

```cpp
9
10
11
12
13
```

</td><td>

```cpp
	// HPゲージ。画面下の中央に、細く置く。
	constexpr float HP_WIDTH = 360.0f;
	constexpr float HP_HEIGHT = 10.0f;
	constexpr float HP_BORDER = 2.0f;
	constexpr float HP_BOTTOM = 80.0f;  // 画面の下端から、ゲージの中央までの高さ。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13 | 画面の下端から、ゲージの中央までの高さ |

- **なぜ:** 大きさや高さを、コードに直接書かず、名前を付けて置く。あとで見た目を変えるときに、ここだけ直せばよい。
- **効果:** HP ゲージの大きさと高さを、この5行で調整できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `HP_WIDTH`・`HP_HEIGHT` | HP ゲージの大きさを決めるため | 幅 360、高さ 10（ピクセル） | この手順で作る |
| 変数 | `HP_BORDER` | 枠の太さを決めるため | 枠の太さ 2 | この手順で作る |
| 変数 | `HP_BOTTOM` | 置く高さを決めるため | 画面の下端から、ゲージの中央までの高さ（80） | この手順で作る |

### 2-8. HP ゲージの位置と色を決める

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **13行目** `constexpr float HP_BOTTOM = 80.0f;  // 画面の下端から、ゲージの中央までの高さ。` の下に、7行足します（14〜20行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 14 -->
<table class="code-lines"><tr><td>

```cpp
14
15
16
17
18
19
20
```

</td><td>

```cpp
	// HPゲージの左端の中央。
	const Vector2 HP_POSITION = { -HP_WIDTH * 0.5f, -static_cast<float>(UI_SPACE_HEIGHT) * 0.5f + HP_BOTTOM };
	const Vector4 HP_COLOR = { 0.55f, 0.88f, 0.35f, 1.0f };      // 明るい緑。
	const Vector4 HP_BACK_COLOR = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 HP_BORDER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 HP_TRAIL_COLOR = { 1.0f, 1.0f, 1.0f, 0.75f };

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 15 | ゲージの左端の中央。幅の半分だけ左にして、画面の中央にそろえる |

- **なぜ:** 座標は画面の中心が原点。ゲージは左端の中央が位置なので、幅の半分だけ左にずらせば、画面の中央にそろう。
- **効果:** HP ゲージが、画面下の中央に細く出るようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `HP_POSITION` | ゲージを置く位置を決めるため | ゲージの左端の中央の位置（`Vector2`） | この手順で作る |
| 値 | `UI_SPACE_HEIGHT` | 画面の高さを知るため | UI を置く画面の高さ（1080）。`static_cast<float>` で `float` にして使う | k2EngineLow（エンジン）の [k2EngineLowPreCompile.h](../../../../k2EngineLow/k2EngineLowPreCompile.h) |
| 変数 | `HP_COLOR`・`HP_BACK_COLOR`・`HP_BORDER_COLOR`・`HP_TRAIL_COLOR` | ゲージの4つの色を決めるため | 中身（明るい緑）・空き・枠・遅れて減るバーの色 | この手順で作る |

### 2-9. HP の数字の見た目を決める

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **20行目**（`const Vector4 HP_TRAIL_COLOR = { 1.0f, 1.0f, 1.0f, 0.75f };` の下の空の行）の下、`}` の上に、6行足します（21〜26行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 21 -->
<table class="code-lines"><tr><td>

```cpp
21
22
23
24
25
26
```

</td><td>

```cpp
	// HP の数字。ゲージの下に小さく。
	constexpr float NUMBER_SCALE = 0.42f;
	constexpr float NUMBER_GAP = 18.0f;  // ゲージの中央から、数字の中央までの距離。
	const Vector4 NUMBER_COLOR = { 1.0f, 1.0f, 1.0f, 0.95f };
	const Vector4 NUMBER_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NUMBER_SHADOW_OFFSET = 1.5f;
```

</td></tr></table>

- **なぜ:** 数字の見た目も、名前を付けて置く。ゲージの色と分けておけば、別々に調整できる。
- **効果:** HP の数字の大きさ・位置・色・影を、この6行で調整できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `NUMBER_SCALE` | 数字の大きさを決めるため | 文字の大きさの倍率（0.42） | この手順で作る |
| 変数 | `NUMBER_GAP` | 数字を置く高さを決めるため | ゲージの中央から、数字の中央までの距離（18） | この手順で作る |
| 変数 | `NUMBER_COLOR`・`NUMBER_SHADOW_COLOR` | 数字と影の色を決めるため | 白と黒（どちらも少し透ける） | この手順で作る |
| 変数 | `NUMBER_SHADOW_OFFSET` | 影のずれを決めるため | 影をずらすピクセル数（1.5） | この手順で作る |

### 2-10. 準備の関数を書く

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **28行目**（`}` の下の空の行）の下（ファイルの最後）に、8行足します（29〜36行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 29 -->
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
```

</td><td>

```cpp
void PlayerHud::Init()
{
	Gauge::Settings hp;
	m_hp.Init(hp);
	m_hp.SetPosition(HP_POSITION);

}

```

</td></tr></table>

- **なぜ:** 部品の見た目は、1回決めれば変わらない。`Init` で1回だけ入れておけば、毎フレームは値を入れて描くだけでよい。
- **効果:** `PlayerHud` が、決めた位置に HP ゲージを出せるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `hp` | ゲージの設定を作るため | `Gauge::Settings` 1つ。決めた項目を入れて、`m_hp.Init(hp)` に渡す | この手順で作る |
| 関数 | `SetPosition` | 部品を置く位置を決めるため | 位置（`Vector2`）を入れる。原点は画面の中心で、右が +x、上が +y | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)） |

### 2-11. HP ゲージの設定を入れる

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **31行目** `Gauge::Settings hp;` の下に、9行足します（32〜40行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 32 -->
<table class="code-lines"><tr><td>

```cpp
32
33
34
35
36
37
38
39
40
```

</td><td>

```cpp
	hp.width = HP_WIDTH;
	hp.height = HP_HEIGHT;
	hp.borderSize = HP_BORDER;
	hp.borderColor = HP_BORDER_COLOR;
	hp.backColor = HP_BACK_COLOR;
	hp.barColor = HP_COLOR;
	hp.barImagePath = UISprite::GAUGE_PATH;
	hp.useTrail = true;
	hp.trailColor = HP_TRAIL_COLOR;
```

</td></tr></table>

- **なぜ:** 初期値と違う項目だけを書く。使わない項目は、初期値のままにしておく。
- **効果:** Player の HP ゲージが、細い緑で、遅れて減るバーが付いた見た目になる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `UISprite::GAUGE_PATH` | ゲージの中身の画像を使うため | 上が白く下へ少し暗くなる画像。上に光が当たったような立体感が出る | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UISprite.h](../../../../Game/Source/UI/Elements/UISprite.h)） |

### 2-12. HP の数字を準備する

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **43行目**（`m_hp.SetPosition(HP_POSITION);` の下の空の行）の下、`}` の上に、4行足します（44〜47行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 44 -->
<table class="code-lines"><tr><td>

```cpp
44
45
46
47
```

</td><td>

```cpp
	m_hpNumber.SetPosition({ 0.0f, HP_POSITION.y - NUMBER_GAP });
	m_hpNumber.SetScale(NUMBER_SCALE);
	m_hpNumber.SetColor(NUMBER_COLOR);
	m_hpNumber.SetShadow(NUMBER_SHADOW_OFFSET, NUMBER_SHADOW_COLOR);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 44 | ゲージの下に置く |

- **なぜ:** HP の数字も、ゲージと同じく、`Init` で見た目を決める。
- **効果:** 数字が、ゲージの下に、小さく影付きで出るようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetScale` | 文字の大きさを決めるため | 大きさの倍率を入れる | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 関数 | `SetColor` | 文字の色を決めるため | 色（`Vector4`）を入れる | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |
| 関数 | `SetShadow` | 文字の周りに影を付けるため | 影をずらすピクセル数と、影の色を入れる | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |

### 2-13. 毎フレーム、HP の割合を入れる

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **49行目**（`}` の下の空の行）の下（ファイルの最後）に、12行足します（50〜61行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 50 -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp
void PlayerHud::Update(float deltaTime)
{
	if (m_player == nullptr) {
		return;
	}
	const PlayerStatus& status = m_player->GetStatus();
	m_hp.SetRate(status.GetHpRate());

	m_hp.Update(deltaTime);
	m_hpNumber.Update(deltaTime);
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 55〜56 | HP の割合（0〜1）をそのまま入れる。ゲージは割合だけを知っていればよい |

- **なぜ:** ゲージは割合だけ知っていればよい。HP と最大 HP を知るのは Player（の `Status`）なので、`PlayerHud` が割合を聞いてゲージに入れる。
- **効果:** Player の HP が変わると、ゲージが動くようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `PlayerStatus` | Player の HP などを読むため | Player の変わる値（HP など）を持つクラス | [キャラクターの構成の手順](../../キャラクター/キャラクターの構成/キャラクターの構成の手順.md) で作った（[PlayerStatus.h](../../../../Game/Source/Player/PlayerStatus.h)） |
| 変数 | `status` | Player の値を短く書くため | `GetStatus()` で返ってきた `PlayerStatus` の参照 | この手順で作る |
| 関数 | `GetStatus` | キャラクターの変わる値を取り出すため | HP などを持つ `Status` を返す | [キャラクターの構成の手順](../../キャラクター/キャラクターの構成/キャラクターの構成の手順.md) で作った（[Player.h](../../../../Game/Source/Player/Player.h)） |
| 関数 | `GetHpRate` | HP の割合を知るため | 最大 HP に対する今の HP の割合（0〜1） | [キャラクターの構成の手順](../../キャラクター/キャラクターの構成/キャラクターの構成の手順.md) で作った（[Status.h](../../../../Game/Source/Character/Status.h)） |

### 2-14. HP の数字を作る

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **57行目**（`m_hp.SetRate(status.GetHpRate());` の下の空の行）の下、`m_hp.Update(deltaTime);` の上に、7行足します（58〜64行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 58 -->
<table class="code-lines"><tr><td>

```cpp
58
59
60
61
62
63
64
```

</td><td>

```cpp
	// 小数は切り上げる（0.3 残っているのに 0 と出さないように）。
	wchar_t number[64];
	swprintf_s(number, L"%d / %d",
		static_cast<int>(ceilf(status.GetHp())),
		static_cast<int>(ceilf(status.GetMaxHp())));
	m_hpNumber.SetText(number);

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 60〜62 | 小数は切り上げる。0.3 残っているのに 0 と出さないように |

- **なぜ:** HP は小数になることがある。切り上げないと、0.3 残っているのに 0 と出て、まだ生きているのに死んだように見える。
- **効果:** 数字が「今のHP / 最大HP」で出て、生きている間は 0 と出なくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `wchar_t` | 日本語を含む文字（ワイド文字）を持つため | `wchar_t number[64]` は、64 文字分の入れ物 | C++ の書き方 |
| 変数 | `number` | 出す文字を作るため | 「100 / 100」のような文字を入れる入れ物 | この手順で作る |
| 関数 | `swprintf_s` | 数字から文字を作るため | 書式（`%d` は整数）に数字を入れて、`number` に書く | C の標準ライブラリの Visual Studio 版（`<wchar.h>`） |
| 関数 | `ceilf` | 小数を切り上げるため | `0.3f` なら `1.0f` を返す | C の標準ライブラリ（`<math.h>`） |
| 関数 | `GetHp`・`GetMaxHp` | 今の HP と最大 HP を知るため | 今の HP・最大 HP を `float` で返す | [キャラクターの構成の手順](../../キャラクター/キャラクターの構成/キャラクターの構成の手順.md) で作った（[Status.h](../../../../Game/Source/Character/Status.h)） |
| 関数 | `SetText` | 出す文字を決めるため | 文字列を入れる。前と同じ文字なら何もしない | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Text.h](../../../../Game/Source/UI/Elements/Text.h)） |

### 2-15. 描く関数を書く

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **68行目**（`}` の下の空の行）の下（ファイルの最後）に、8行足します（69〜76行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 69 -->
<table class="code-lines"><tr><td>

```cpp
69
70
71
72
73
74
75
76
```

</td><td>

```cpp
void PlayerHud::Draw(RenderContext& rc)
{
	if (m_player == nullptr) {
		return;
	}
	m_hp.Draw(rc);
	m_hpNumber.Draw(rc);
}
```

</td></tr></table>

- **なぜ:** Player がまだ渡されていないときは、何も描かない。そのうえで、ゲージと数字を描く。
- **効果:** Player が作られる前でも、画面が壊れない。

### 2-16. UIManager.h で PlayerHud.h を読み込む

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **1行目** `#pragma once` の下に、4行足します（2〜5行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 2 -->
<table class="code-lines"><tr><td>

```cpp
2
3
4
5
```

</td><td>

```cpp

#include "PlayerHud.h"

class Player;
```

</td></tr></table>

- **なぜ:** `UIManager` が `PlayerHud` を持つので、`.h` を読み込む。`Player` は、ポインターで受け取るだけなので、前方宣言で足りる。
- **効果:** `UIManager.h` で、`PlayerHud` と `Player` が使えるようになる。

### 2-17. Player を渡す関数と、PlayerHud を持つメンバーを足す

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **16行目** `void Render(RenderContext& rc) override;` の下に、8行足します（17〜24行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 17 -->
<table class="code-lines"><tr><td>

```cpp
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

	void SetPlayer(const Player* player)
	{
		m_playerHud.SetTarget(player);
	}

private:
	PlayerHud m_playerHud;
```

</td></tr></table>

- **なぜ:** ゲームの側は `PlayerHud` を知らなくてよいよう、`UIManager` に渡す窓口を作る。HUD は `UIManager` が持つ。
- **効果:** `Game` が `UIManager` に Player を渡すだけで、HP が出せるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetPlayer` | 表示する Player を渡すため | `PlayerHud` に Player を渡す | この手順で作る |
| メンバー変数 | `m_playerHud` | Player の HUD を持つため | `PlayerHud` 1つ | この手順で作る |

### 2-18. 最初に PlayerHud を準備する

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **5行目** `{` の下、`return true;` の上に、2行足します（6〜7行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 6 -->
<table class="code-lines"><tr><td>

```cpp
6
7
```

</td><td>

```cpp
	// Init は部品の画像を読むだけで、SetPlayer / ShowBoss で入れた値は消さない。
	m_playerHud.Init();
```

</td></tr></table>

- **なぜ:** 部品の画像は、`Start` で1回だけ読む。`SetPlayer` で入れた値は消さない。
- **効果:** `UIManager` が始まるときに、HP ゲージの準備ができる。

### 2-19. 毎フレーム PlayerHud を更新する

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **13行目** `const float deltaTime = g_gameTime->GetFrameDeltaTime();` の下に、1行足します（14〜14行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 14 -->
<table class="code-lines"><tr><td>

```cpp
14
```

</td><td>

```cpp
	m_playerHud.Update(deltaTime);
```

</td></tr></table>

- **なぜ:** Player の HP は毎フレーム変わる。毎フレーム `Update` を呼んで、ゲージに入れ直す。
- **効果:** HP が減る・増えると、ゲージが動く。

### 2-20. PlayerHud を描く

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **19行目** `// 後に描いたものが上に重なる。` の下に、1行足します（20〜20行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 20 -->
<table class="code-lines"><tr><td>

```cpp
20
```

</td><td>

```cpp
	m_playerHud.Draw(rc);
```

</td></tr></table>

- **なぜ:** 後に描いたものが上に重なる。`PlayerHud` は先に描く。
- **効果:** HP ゲージが画面に出るようになる。

### 2-21. Game から Player を渡す

[Game.cpp](../../../../Game/Source/Core/Game.cpp) の **78行目** `m_uiManager = NewGO<UIManager>(3, "uiManager");` の下に、1行足します（79〜79行目になります）。

<!-- add: Action/Game/Source/Core/Game.cpp 79 -->
<table class="code-lines"><tr><td>

```cpp
79
```

</td><td>

```cpp
	m_uiManager->SetPlayer(m_player);
```

</td></tr></table>

- **なぜ:** Player を作ったのは `Game`。`UIManager` を作ったら、すぐに Player を渡す。
- **効果:** `PlayerHud` が Player を読めるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_uiManager` | UI を動かすため | `Game` が持つ `UIManager` のポインター。`NewGO` で作る | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（Game.h） |

**確かめ方:** 起動すると、画面下の中央に緑の HP ゲージと「100 / 100」が出る。

---

## 手順3: PlayerHud に、状態異常のアイコンを足す

画像ができるまでは、色付きの四角で出します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) | 追加（`#include`、アイコン2つ） |
| [PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) | 追加（大きさと色、置く、見えるかを決める、更新・描く） |

### 3-1. Icon.h を読み込む

[PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) の **3行目** `#include "Gauge.h"` の下に、1行足します（4〜4行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.h 4 -->
<table class="code-lines"><tr><td>

```cpp
4
```

</td><td>

```cpp
#include "Icon.h"
```

</td></tr></table>

- **なぜ:** `PlayerHud` が `Icon` を持つので、`Icon.h` を読み込む。
- **効果:** `PlayerHud.h` で、`Icon` が使えるようになる。

### 3-2. アイコンを持つメンバーを足す

[PlayerHud.h](../../../../Game/Source/UI/Hud/PlayerHud.h) の **27行目** `Text m_hpNumber;  // 「今のHP / 最大HP」。` の下に、2行足します（28〜29行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.h 28 -->
<table class="code-lines"><tr><td>

```cpp
28
29
```

</td><td>

```cpp
	Icon m_frozen;    // 凍結中だけ出す。
	Icon m_burning;   // 燃焼中だけ出す。
```

</td></tr></table>

- **なぜ:** 凍結と燃焼は別々に出し入れしたい。アイコンを1つずつ持つ。
- **効果:** 凍結・燃焼のアイコンを、別々に出したり消したりできる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Icon` | 画像を1枚出すため | 位置は画像の中央。画像がないうちは、白い画像に色を付けて四角で出す | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[Icon.h](../../../../Game/Source/UI/Elements/Icon.h)） |
| メンバー変数 | `m_frozen` | 凍結中のアイコンを持つため | `Icon` 1つ | この手順で作る |
| メンバー変数 | `m_burning` | 燃焼中のアイコンを持つため | `Icon` 1つ | この手順で作る |

### 3-3. アイコンの大きさと色を決める

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **26行目** `constexpr float NUMBER_SHADOW_OFFSET = 1.5f;` の下に、6行足します（27〜32行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 27 -->
<table class="code-lines"><tr><td>

```cpp
27
28
29
30
31
32
```

</td><td>

```cpp

	// 状態異常のアイコン（画像ができるまでは色付きの四角）。ゲージの右に並べる。
	constexpr float ICON_SIZE = 18.0f;
	constexpr float ICON_GAP = 10.0f;
	const Vector4 FROZEN_COLOR = { 0.4f, 0.85f, 1.0f, 1.0f };
	const Vector4 BURNING_COLOR = { 1.0f, 0.45f, 0.1f, 1.0f };
```

</td></tr></table>

- **なぜ:** 大きさ・間隔・色を、名前を付けて置く。画像ができたら、色の代わりに画像へ替える。
- **効果:** アイコンの大きさ・間隔・色を、この数行で調整できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `ICON_SIZE` | アイコンの大きさを決めるため | 一辺の長さ（18） | この手順で作る |
| 変数 | `ICON_GAP` | アイコンの間隔を決めるため | ゲージとアイコン、アイコン同士の間（10） | この手順で作る |
| 変数 | `FROZEN_COLOR`・`BURNING_COLOR` | アイコンの色を決めるため | 水色（凍結）・橙色（燃焼） | この手順で作る |

### 3-4. アイコンを置く位置を求める

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **53行目** `m_hpNumber.SetShadow(NUMBER_SHADOW_OFFSET, NUMBER_SHADOW_COLOR);` の下に、5行足します（54〜58行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 54 -->
<table class="code-lines"><tr><td>

```cpp
54
55
56
57
58
```

</td><td>

```cpp

	const float iconY = HP_POSITION.y;
	const float frozenX = HP_POSITION.x + HP_WIDTH + HP_BORDER + ICON_GAP + ICON_SIZE * 0.5f;
	const float burningX = frozenX + ICON_SIZE + ICON_GAP;

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 56 | ゲージの右端から少し離したところ |
| 57 | その右隣 |

- **なぜ:** アイコンはゲージの右に並べる。位置は、ゲージの位置と大きさから計算すれば、ゲージの大きさを変えても追いかけてくる。
- **効果:** ゲージの右に、凍結・燃焼のアイコンが並ぶ位置が決まる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `iconY` | アイコンの高さを決めるため | ゲージの高さと同じ | この手順で作る |
| 変数 | `frozenX` | 凍結のアイコンの位置を決めるため | ゲージの右端から少し離れた、アイコンの中央の x 座標 | この手順で作る |
| 変数 | `burningX` | 燃焼のアイコンの位置を決めるため | 凍結のアイコンの右隣の x 座標 | この手順で作る |

### 3-5. アイコンを準備する

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **58行目**（`const float burningX = frozenX + ICON_SIZE + ICON_GAP;` の下の空の行）の下、`}` の上に、7行足します（59〜65行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 59 -->
<table class="code-lines"><tr><td>

```cpp
59
60
61
62
63
64
65
```

</td><td>

```cpp
	m_frozen.Init(UISprite::WHITE_PATH, ICON_SIZE, ICON_SIZE);
	m_frozen.SetColor(FROZEN_COLOR);
	m_frozen.SetPosition({ frozenX, iconY });

	m_burning.Init(UISprite::WHITE_PATH, ICON_SIZE, ICON_SIZE);
	m_burning.SetColor(BURNING_COLOR);
	m_burning.SetPosition({ burningX, iconY });
```

</td></tr></table>

- **なぜ:** 画像ができるまでは、白い画像に色を付けた四角で出す。大きさ・色・位置は、`Init` で1回決める。
- **効果:** 状態異常のアイコンを、決めた位置と色で出せるようになる。

### 3-6. 状態異常のときだけ見えるようにする

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **82行目**（`m_hpNumber.SetText(number);` の下の空の行）の下、`m_hp.Update(deltaTime);` の上に、4行足します（83〜86行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 83 -->
<table class="code-lines"><tr><td>

```cpp
83
84
85
86
```

</td><td>

```cpp
	const StatusEffects& effects = m_player->GetStatusEffects();
	m_frozen.SetVisible(effects.IsFrozen());
	m_burning.SetVisible(effects.IsBurning());

```

</td></tr></table>

- **なぜ:** 状態異常は、毎フレーム変わる。そのたびに見えるかを入れ直せば、アイコンを出し入れするコードを別に書かずに済む。
- **効果:** 凍結中・燃焼中だけ、アイコンが見えるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `StatusEffects` | 凍結・燃焼などの状態を読むため | 状態異常を持つクラス | [状態異常の手順](../../キャラクター/状態異常/状態異常の手順.md) で作った（[StatusEffects.h](../../../../Game/Source/Character/Components/StatusEffects.h)） |
| 変数 | `effects` | 状態異常を短く書くため | `GetStatusEffects()` で返ってきた `StatusEffects` の参照 | この手順で作る |
| 関数 | `GetStatusEffects` | キャラクターの状態異常を取り出すため | 読むだけの `StatusEffects` を返す（`const` の Player から呼べる方） | [状態異常の手順](../../キャラクター/状態異常/状態異常の手順.md) で作った（[Character.h](../../../../Game/Source/Character/Character.h)） |
| 関数 | `IsFrozen`・`IsBurning` | 今の状態を知るため | 凍結中・燃焼中なら `true` | [状態異常の手順](../../キャラクター/状態異常/状態異常の手順.md) で作った（[StatusEffects.h](../../../../Game/Source/Character/Components/StatusEffects.h)） |
| 関数 | `SetVisible` | 見えるかを決めるため | `false` の間は、`Draw` を呼んでも何も描かない | [UIの構成の手順](../UIの構成/UIの構成の手順.md) で作った（[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h)） |

### 3-7. アイコンを更新する

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **88行目** `m_hpNumber.Update(deltaTime);` の下に、2行足します（89〜90行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 89 -->
<table class="code-lines"><tr><td>

```cpp
89
90
```

</td><td>

```cpp
	m_frozen.Update(deltaTime);
	m_burning.Update(deltaTime);
```

</td></tr></table>

- **なぜ:** アイコンも、毎フレーム `Update` を呼んで、置き直す。
- **効果:** 見えるかの変化が、画面に反映される。

### 3-8. アイコンを描く

[PlayerHud.cpp](../../../../Game/Source/UI/Hud/PlayerHud.cpp) の **99行目** `m_hpNumber.Draw(rc);` の下に、2行足します（100〜101行目になります）。

<!-- add: Action/Game/Source/UI/Hud/PlayerHud.cpp 100 -->
<table class="code-lines"><tr><td>

```cpp
100
101
```

</td><td>

```cpp
	m_frozen.Draw(rc);
	m_burning.Draw(rc);
```

</td></tr></table>

- **なぜ:** 描かないと、画面に出ない。`SetVisible(false)` のときは、`Draw` が何もしない。
- **効果:** 凍結中・燃焼中のアイコンが画面に出る。

### 出来上がった PlayerHud.h

<!-- full: Action/Game/Source/UI/Hud/PlayerHud.h -->
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
```

</td><td>

```cpp
#pragma once

#include "Gauge.h"
#include "Icon.h"
#include "Text.h"

class Player;

// Player 用の HUD。Player の値を読んで、自分の部品に入れる。
// 画面下の中央に、細い緑の HPゲージと、その下に「今のHP / 最大HP」の数字を出す。状態異常のアイコンはゲージの右。
// UIElement は継承しない（自分では描かず、部品を持つだけ）。UIManager が持つ。
class PlayerHud
{
public:
	void Init();
	// 表示する Player。読むだけ（const）。nullptr なら何も描かない。
	void SetTarget(const Player* player)
	{
		m_player = player;
	}
	void Update(float deltaTime);
	void Draw(RenderContext& rc);

private:
	const Player* m_player = nullptr;
	Gauge m_hp;
	Text m_hpNumber;  // 「今のHP / 最大HP」。
	Icon m_frozen;    // 凍結中だけ出す。
	Icon m_burning;   // 燃焼中だけ出す。
};
```

</td></tr></table>

### 出来上がった PlayerHud.cpp

<!-- full: Action/Game/Source/UI/Hud/PlayerHud.cpp -->
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
67
68
69
70
71
72
73
74
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
91
92
93
94
95
96
97
98
99
100
101
102
```

</td><td>

```cpp
#include "stdafx.h"
#include "PlayerHud.h"

#include "Player.h"
#include "UISprite.h"

namespace
{
	// HPゲージ。画面下の中央に、細く置く。
	constexpr float HP_WIDTH = 360.0f;
	constexpr float HP_HEIGHT = 10.0f;
	constexpr float HP_BORDER = 2.0f;
	constexpr float HP_BOTTOM = 80.0f;  // 画面の下端から、ゲージの中央までの高さ。
	// HPゲージの左端の中央。
	const Vector2 HP_POSITION = { -HP_WIDTH * 0.5f, -static_cast<float>(UI_SPACE_HEIGHT) * 0.5f + HP_BOTTOM };
	const Vector4 HP_COLOR = { 0.55f, 0.88f, 0.35f, 1.0f };      // 明るい緑。
	const Vector4 HP_BACK_COLOR = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 HP_BORDER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 HP_TRAIL_COLOR = { 1.0f, 1.0f, 1.0f, 0.75f };

	// HP の数字。ゲージの下に小さく。
	constexpr float NUMBER_SCALE = 0.42f;
	constexpr float NUMBER_GAP = 18.0f;  // ゲージの中央から、数字の中央までの距離。
	const Vector4 NUMBER_COLOR = { 1.0f, 1.0f, 1.0f, 0.95f };
	const Vector4 NUMBER_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NUMBER_SHADOW_OFFSET = 1.5f;

	// 状態異常のアイコン（画像ができるまでは色付きの四角）。ゲージの右に並べる。
	constexpr float ICON_SIZE = 18.0f;
	constexpr float ICON_GAP = 10.0f;
	const Vector4 FROZEN_COLOR = { 0.4f, 0.85f, 1.0f, 1.0f };
	const Vector4 BURNING_COLOR = { 1.0f, 0.45f, 0.1f, 1.0f };
}

void PlayerHud::Init()
{
	Gauge::Settings hp;
	hp.width = HP_WIDTH;
	hp.height = HP_HEIGHT;
	hp.borderSize = HP_BORDER;
	hp.borderColor = HP_BORDER_COLOR;
	hp.backColor = HP_BACK_COLOR;
	hp.barColor = HP_COLOR;
	hp.barImagePath = UISprite::GAUGE_PATH;
	hp.useTrail = true;
	hp.trailColor = HP_TRAIL_COLOR;
	m_hp.Init(hp);
	m_hp.SetPosition(HP_POSITION);

	m_hpNumber.SetPosition({ 0.0f, HP_POSITION.y - NUMBER_GAP });
	m_hpNumber.SetScale(NUMBER_SCALE);
	m_hpNumber.SetColor(NUMBER_COLOR);
	m_hpNumber.SetShadow(NUMBER_SHADOW_OFFSET, NUMBER_SHADOW_COLOR);

	const float iconY = HP_POSITION.y;
	const float frozenX = HP_POSITION.x + HP_WIDTH + HP_BORDER + ICON_GAP + ICON_SIZE * 0.5f;
	const float burningX = frozenX + ICON_SIZE + ICON_GAP;

	m_frozen.Init(UISprite::WHITE_PATH, ICON_SIZE, ICON_SIZE);
	m_frozen.SetColor(FROZEN_COLOR);
	m_frozen.SetPosition({ frozenX, iconY });

	m_burning.Init(UISprite::WHITE_PATH, ICON_SIZE, ICON_SIZE);
	m_burning.SetColor(BURNING_COLOR);
	m_burning.SetPosition({ burningX, iconY });
}

void PlayerHud::Update(float deltaTime)
{
	if (m_player == nullptr) {
		return;
	}
	const PlayerStatus& status = m_player->GetStatus();
	m_hp.SetRate(status.GetHpRate());

	// 小数は切り上げる（0.3 残っているのに 0 と出さないように）。
	wchar_t number[64];
	swprintf_s(number, L"%d / %d",
		static_cast<int>(ceilf(status.GetHp())),
		static_cast<int>(ceilf(status.GetMaxHp())));
	m_hpNumber.SetText(number);

	const StatusEffects& effects = m_player->GetStatusEffects();
	m_frozen.SetVisible(effects.IsFrozen());
	m_burning.SetVisible(effects.IsBurning());

	m_hp.Update(deltaTime);
	m_hpNumber.Update(deltaTime);
	m_frozen.Update(deltaTime);
	m_burning.Update(deltaTime);
}

void PlayerHud::Draw(RenderContext& rc)
{
	if (m_player == nullptr) {
		return;
	}
	m_hp.Draw(rc);
	m_hpNumber.Draw(rc);
	m_frozen.Draw(rc);
	m_burning.Draw(rc);
}
```

</td></tr></table>

**確かめ方:** 凍結中はゲージの右に水色の四角が、燃焼中は橙色の四角が出る。

---

## 手順4: BossHud で、ボスの名前と HP ゲージを出す

画面上の中央に、名前と HP ゲージ（遅れて減るバー付き）を出します。`Show()` でフェードインし、`Hide()` でフェードアウトします。Boss のクラスはまだないので、今は `Character` と名前を受け取ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BossHud.h](../../../../Game/Source/UI/Hud/BossHud.h) | 新しく作る（ボスの HUD） |
| [BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) | 新しく作る（名前とゲージ、フェード） |
| [UIManager.h](../../../../Game/Source/UI/UIManager.h) | 追加（`#include`、出す・消す関数、メンバー） |
| [UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) | 追加（準備・更新・描く） |

`Source\UI\Hud` フィルターに `BossHud.h` と `BossHud.cpp` を作ります。

### 4-1. BossHud クラスの形を書く

[BossHud.h](../../../../Game/Source/UI/Hud/BossHud.h) を新しく作り、次の12行を書きます（1〜12行目）。

<!-- new: Action/Game/Source/UI/Hud/BossHud.h -->
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
```

</td><td>

```cpp
#pragma once

#include "Gauge.h"
#include "Text.h"

class Character;

class BossHud
{
public:
	void Init();
};
```

</td></tr></table>

- **なぜ:** ボスの HP の表示は、Player の HUD と出し方が違う（上に出る、フェードする）。別のクラスにする。
- **効果:** `UIManager` が `BossHud` を1つ持てば、ボスの HUD を出せるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `BossHud` | ボスの HUD をまとめるため | 名前と HP ゲージを持つクラス | この手順で作る |
| 型 | `Character` | ボスの HP を読むため | Player・Enemy の基底クラス。Boss のクラスができるまでは、これで受け取る | [キャラクターの構成の手順](../../キャラクター/キャラクターの構成/キャラクターの構成の手順.md) で作った（[Character.h](../../../../Game/Source/Character/Character.h)） |

### 4-2. 出す・消す関数を宣言する

[BossHud.h](../../../../Game/Source/UI/Hud/BossHud.h) の **11行目** `void Init();` の下に、5行足します（12〜16行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.h 12 -->
<table class="code-lines"><tr><td>

```cpp
12
13
14
15
16
```

</td><td>

```cpp
	// 表示を始める。ゲージの白いバーを今のHPにそろえる。
	void Show(const Character* boss, const wchar_t* name);
	// 表示をやめる。ボスへのポインタはすぐ捨てる（このあと DeleteGO してよい）。
	// フェードアウトが終わるまでは、最後の値のまま描く。
	void Hide();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13 | 表示を始める。遅れて減るバーを今の HP にそろえる |
| 16 | 表示をやめる。ボスへのポインターはすぐ捨てるので、このあとボスを消してよい。フェードアウトが終わるまでは、最後の値のまま描く |

- **なぜ:** ボスが出てくるときだけ HUD を出す。出す・消すを関数にして、`UIManager` から呼べるようにする。消すときはポインターをすぐ捨てる。
- **効果:** ボスを `DeleteGO` する前に `Hide` を呼べば、消えたボスを読んで壊れることがなくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Show` | ボスの HUD を出すため | ボスと名前を覚え、フェードインを始める | この手順で作る |
| 引数 | `boss` | HP を読むボスを受け取るため | `const Character*`（読むだけ） | 使う側が決める |
| 引数 | `name` | 出す名前を受け取るため | ワイド文字の文字列 | 使う側が決める |
| 関数 | `Hide` | ボスの HUD を消すため | ボスへのポインターを捨て、フェードアウトを始める | この手順で作る |

### 4-3. 出しているか、更新・描く関数を足す

[BossHud.h](../../../../Game/Source/UI/Hud/BossHud.h) の **16行目** `void Hide();` の下に、6行足します（17〜22行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.h 17 -->
<table class="code-lines"><tr><td>

```cpp
17
18
19
20
21
22
```

</td><td>

```cpp
	bool IsShown() const
	{
		return m_boss != nullptr;
	}
	void Update(float deltaTime);
	void Draw(RenderContext& rc);
```

</td></tr></table>

- **なぜ:** ボスがいるかどうかは、`m_boss` が `nullptr` かどうかで決まる。ほかのクラスが聞けるよう、読む関数にしておく。
- **効果:** 今、ボスの HUD が出ているかを調べられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `IsShown` | 出ているかを知るため | ボスが入っていれば `true` | この手順で作る |
| メンバー変数 | `m_boss` | HP を読むボスを持つため | `Show` で入れたボス。`Hide` か、入れるまでは `nullptr` | この手順で作る |

### 4-4. 持つ値を足す

[BossHud.h](../../../../Game/Source/UI/Hud/BossHud.h) の **22行目** `void Draw(RenderContext& rc);` の下に、9行足します（23〜31行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.h 23 -->
<table class="code-lines"><tr><td>

```cpp
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

private:
	void SetAlpha(float alpha);

	const Character* m_boss = nullptr;
	Text m_name;
	Gauge m_hp;
	float m_alpha = 0.0f;        // 今の透明度。
	float m_targetAlpha = 0.0f;  // Show で 1、Hide で 0。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 30 | 今の透明度 |
| 31 | 目指す透明度。出すと 1、消すと 0 |

- **なぜ:** 名前とゲージを同じ透明度にしたい。透明度を入れる関数を1つにして、フェードのたびに2つへ入れる。
- **効果:** 名前とゲージを、いっしょにフェードイン・アウトできる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetAlpha` | 名前とゲージの透明度をまとめて決めるため | 同じ透明度を2つの部品に入れる（部品の `SetAlpha`（`UIElement.h`）とは別の関数） | この手順で作る |
| 引数 | `alpha` | 透明度を受け取るため | 0 で見えない、1 で不透明 | 使う側が決める |
| メンバー変数 | `m_name` | ボスの名前を持つため | 名前を出す `Text` | この手順で作る |
| メンバー変数 | `m_alpha` | 今の透明度を持つため | 0〜1。フェードの途中の値 | この手順で作る |
| メンバー変数 | `m_targetAlpha` | 目指す透明度を持つため | `Show` で 1、`Hide` で 0 | この手順で作る |

### 4-5. コメントを書く

[BossHud.h](../../../../Game/Source/UI/Hud/BossHud.h) の **7行目**（`class Character;` の下の空の行）の下、`class BossHud` の上に、3行足します（8〜10行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.h 8 -->
<table class="code-lines"><tr><td>

```cpp
8
9
10
```

</td><td>

```cpp
// ボス用の HUD。画面上の中央に、名前とHPゲージ（遅れて減るバー付き）を出す。
// Show() でフェードインし、Hide() でフェードアウトする。UIElement は継承しない。UIManager が持つ。
// Boss クラスはまだないので、今は Character と名前を受け取る。Boss ができたら const Boss* に変える。
```

</td></tr></table>

- **なぜ:** Boss クラスがまだないので `Character` で受け取っている。今は仮であることを、コメントに残しておく。
- **効果:** Boss ができたときに、どこを直せばよいかがわかる。

### 4-6. BossHud.cpp を作り、使うものを読み込む

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) を新しく作り、次の6行を書きます（1〜6行目）。

<!-- new: Action/Game/Source/UI/Hud/BossHud.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
```

</td><td>

```cpp
#include "stdafx.h"
#include "BossHud.h"

#include "Character.h"
#include "UISprite.h"

```

</td></tr></table>

- **なぜ:** `.cpp` の1行目は `stdafx.h`。ボスの HP を読むので `Character.h` を、画像のパスを使うので `UISprite.h` を読み込む。
- **効果:** `BossHud.cpp` で、`Character` と `UISprite` が使えるようになる。

### 4-7. 値を置く場所を作る

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **6行目**（`#include "UISprite.h"` の下の空の行）の下（ファイルの最後）に、4行足します（7〜10行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 7 -->
<table class="code-lines"><tr><td>

```cpp
7
8
9
10
```

</td><td>

```cpp
namespace
{
}

```

</td></tr></table>

- **なぜ:** この `.cpp` だけで使う値は、無名の `namespace` に置く。
- **効果:** 見た目の数字を、1か所にまとめて置ける。

### 4-8. HP ゲージの大きさと置く高さを決める

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **8行目** `{` の下、`}` の上に、5行足します（9〜13行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 9 -->
<table class="code-lines"><tr><td>

```cpp
9
10
11
12
13
```

</td><td>

```cpp
	// HPゲージ。画面上の中央に、横長に置く（Player のゲージは画面下なので重ならない）。
	constexpr float HP_WIDTH = 720.0f;
	constexpr float HP_HEIGHT = 12.0f;
	constexpr float HP_BORDER = 2.0f;
	constexpr float HP_TOP = 100.0f;  // 画面の上端から、ゲージの中央までの距離。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13 | 画面の上端から、ゲージの中央までの距離 |

- **なぜ:** Player のゲージは画面下、ボスのゲージは画面上。同じ名前でも、この `.cpp` の中だけの値なので、ぶつからない。ボスのゲージは、横長に大きく出す。
- **効果:** ボスのゲージの大きさと高さを、この5行で調整できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `HP_TOP` | 置く高さを決めるため | 画面の上端から、ゲージの中央までの距離（100） | この手順で作る |

### 4-9. HP ゲージの位置と色を決める

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **13行目** `constexpr float HP_TOP = 100.0f;  // 画面の上端から、ゲージの中央までの距離。` の下に、7行足します（14〜20行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 14 -->
<table class="code-lines"><tr><td>

```cpp
14
15
16
17
18
19
20
```

</td><td>

```cpp
	// HPゲージの左端の中央。
	const Vector2 HP_POSITION = { -HP_WIDTH * 0.5f, static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - HP_TOP };
	const Vector4 HP_COLOR = { 0.9f, 0.28f, 0.22f, 1.0f };      // 赤。
	const Vector4 HP_BACK_COLOR = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 HP_BORDER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 HP_TRAIL_COLOR = { 1.0f, 0.92f, 0.8f, 0.8f };  // 少し暖かい白。

```

</td></tr></table>

- **なぜ:** 手順2と同じく、左端の中央の位置と色を決める。座標は、画面の中心が原点で上が +y なので、上に置くときは画面の高さの半分から引く。
- **効果:** ボスの HP ゲージが、画面の上の中央に、赤く出るようになる。

### 4-10. 名前の見た目を決める

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **20行目**（`const Vector4 HP_TRAIL_COLOR = { 1.0f, 0.92f, 0.8f, 0.8f };  // 少し暖かい白。` の下の空の行）の下、`}` の上に、7行足します（21〜27行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 21 -->
<table class="code-lines"><tr><td>

```cpp
21
22
23
24
25
26
27
```

</td><td>

```cpp
	// 名前は HPゲージの上に小さく。
	constexpr float NAME_SCALE = 0.55f;
	constexpr float NAME_GAP = 24.0f;  // ゲージの中央から、名前の中央までの距離。
	const Vector2 NAME_POSITION = { 0.0f, HP_POSITION.y + NAME_GAP };
	const Vector4 NAME_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NAME_SHADOW_OFFSET = 1.5f;

```

</td></tr></table>

- **なぜ:** 名前はゲージの上に小さく出す。位置はゲージの位置から決めれば、ゲージを動かしても追いかけてくる。
- **効果:** ボスの名前が、ゲージの上に出るようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `NAME_SCALE` | 名前の大きさを決めるため | 文字の大きさの倍率（0.55） | この手順で作る |
| 変数 | `NAME_GAP` | 名前を置く高さを決めるため | ゲージの中央から、名前の中央までの距離（24） | この手順で作る |
| 変数 | `NAME_POSITION` | 名前を置く位置を決めるため | 画面の中央の x と、ゲージより少し上の y | この手順で作る |
| 変数 | `NAME_SHADOW_COLOR`・`NAME_SHADOW_OFFSET` | 名前の影を決めるため | 黒い影の色と、ずらすピクセル数（1.5） | この手順で作る |

### 4-11. 遅れて減るバーとフェードの時間を決める

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **27行目**（`constexpr float NAME_SHADOW_OFFSET = 1.5f;` の下の空の行）の下、`}` の上に、3行足します（28〜30行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 28 -->
<table class="code-lines"><tr><td>

```cpp
28
29
30
```

</td><td>

```cpp
	constexpr float TRAIL_DELAY = 0.5f;  // 減ってから、遅れて減るバーが縮み始めるまでの秒数。
	constexpr float TRAIL_SPEED = 0.5f;  // 1秒にゲージ全体の 50% 縮む。
	constexpr float FADE_SECONDS = 0.3f; // フェードイン・アウトにかける秒数。
```

</td></tr></table>

- **なぜ:** 時間は、コードに直接書かず、名前を付けて置く。
- **効果:** ボスのゲージが縮む待ち時間・速さと、フェードの長さを、ここで調整できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `TRAIL_DELAY`・`TRAIL_SPEED` | 遅れて減るバーの動きを決めるため | 縮み始めるまでの秒数（0.5）と、1秒に縮む割合（0.5） | この手順で作る |
| 変数 | `FADE_SECONDS` | フェードの長さを決めるため | 見えない状態から見えるまでの秒数（0.3） | この手順で作る |

### 4-12. 準備の関数を書く

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **32行目**（`}` の下の空の行）の下（ファイルの最後）に、9行足します（33〜41行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 33 -->
<table class="code-lines"><tr><td>

```cpp
33
34
35
36
37
38
39
40
41
```

</td><td>

```cpp
void BossHud::Init()
{
	Gauge::Settings hp;
	m_hp.Init(hp);
	m_hp.SetPosition(HP_POSITION);

	SetAlpha(0.0f);
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 39 | 最初は見えない |

- **なぜ:** 準備は `Init` に書く。最初は見えないので、透明度 0 にしておく。
- **効果:** `UIManager` が始まったとき、ボスの HUD は見えない。

### 4-13. HP ゲージの設定を入れる

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **35行目** `Gauge::Settings hp;` の下に、11行足します（36〜46行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 36 -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp
	hp.width = HP_WIDTH;
	hp.height = HP_HEIGHT;
	hp.borderSize = HP_BORDER;
	hp.borderColor = HP_BORDER_COLOR;
	hp.backColor = HP_BACK_COLOR;
	hp.barColor = HP_COLOR;
	hp.barImagePath = UISprite::GAUGE_PATH;
	hp.useTrail = true;
	hp.trailColor = HP_TRAIL_COLOR;
	hp.trailDelay = TRAIL_DELAY;
	hp.trailSpeed = TRAIL_SPEED;
```

</td></tr></table>

- **なぜ:** 初期値と違う項目だけを書く。Player のゲージと違って、縮み始める時間と速さも決める。
- **効果:** ボスのゲージが、赤く、遅れて減るバーが付いた見た目になる。

### 4-14. 名前を準備する

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **49行目**（`m_hp.SetPosition(HP_POSITION);` の下の空の行）の下、`SetAlpha(0.0f);` の上に、4行足します（50〜53行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 50 -->
<table class="code-lines"><tr><td>

```cpp
50
51
52
53
```

</td><td>

```cpp
	m_name.SetPosition(NAME_POSITION);
	m_name.SetScale(NAME_SCALE);
	m_name.SetShadow(NAME_SHADOW_OFFSET, NAME_SHADOW_COLOR);

```

</td></tr></table>

- **なぜ:** 名前の見た目も、`Init` で1回決める。
- **効果:** 名前が、決めた位置・大きさ・影で出る。

### 4-15. 出す関数を書く

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **56行目**（`}` の下の空の行）の下（ファイルの最後）に、12行足します（57〜68行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 57 -->
<table class="code-lines"><tr><td>

```cpp
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
67
68
```

</td><td>

```cpp
void BossHud::Show(const Character* boss, const wchar_t* name)
{
	m_boss = boss;
	m_name.SetText(name);
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}
	// 戦い始めに白いバーが減っていく演出にならないよう、今のHPにそろえる。
	m_hp.ResetTrail();
	m_targetAlpha = 1.0f;
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 61〜63 | 今の HP の割合を入れる |
| 65 | 遅れて減るバーを今の HP にそろえる。戦い始めに、白いバーが減っていく演出にならないように |
| 66 | フェードインを始める |

- **なぜ:** 出すときに今の HP を入れて、白いバーも今の HP にそろえる。そろえないと、戦い始めに白いバーが満タンから減っていく動きが出てしまう。
- **効果:** ボスの HUD が、今の HP のゲージで、フェードインして出る。

### 4-16. 消す関数を書く

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **68行目**（`}` の下の空の行）の下（ファイルの最後）に、6行足します（69〜74行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 69 -->
<table class="code-lines"><tr><td>

```cpp
69
70
71
72
73
74
```

</td><td>

```cpp
void BossHud::Hide()
{
	m_boss = nullptr;
	m_targetAlpha = 0.0f;
}

```

</td></tr></table>

- **なぜ:** ボスを `DeleteGO` したあとに、消えたボスを読むと壊れる。消す前に、ポインターを捨てておく。
- **効果:** ボスを消しても、HUD が壊れない。フェードアウトは、最後の値のまま続く。

### 4-17. 毎フレーム、HP の割合を入れる

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **74行目**（`}` の下の空の行）の下（ファイルの最後）に、10行足します（75〜84行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 75 -->
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
```

</td><td>

```cpp
void BossHud::Update(float deltaTime)
{
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}

	m_name.Update(deltaTime);
	m_hp.Update(deltaTime);
}

```

</td></tr></table>

- **なぜ:** ボスの HP は毎フレーム変わる。ボスがいる間は、毎フレーム割合を入れ直す。
- **効果:** ボスの HP が減ると、ゲージが動く。

### 4-18. 目指す透明度へ近づける

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **80行目**（`}` の下の空の行）の下、`m_name.Update(deltaTime);` の上に、9行足します（81〜89行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 81 -->
<table class="code-lines"><tr><td>

```cpp
81
82
83
84
85
86
87
88
89
```

</td><td>

```cpp
	// 目標の透明度へ、一定の速さで近づける。
	const float step = deltaTime / FADE_SECONDS;
	if (m_alpha < m_targetAlpha) {
		SetAlpha(min(m_targetAlpha, m_alpha + step));
	}
	else if (m_alpha > m_targetAlpha) {
		SetAlpha(max(m_targetAlpha, m_alpha - step));
	}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 82〜88 | 0.3 秒で 0 から 1 になる速さで、目指す透明度へ近づける |

- **なぜ:** 透明度を一気に変えず、決めた時間で少しずつ近づければ、なめらかにフェードする。
- **効果:** 出すときはフェードインし、消すときはフェードアウトする。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `step` | 1フレームで変える量を持つため | `deltaTime / FADE_SECONDS`。0.3 秒で 0 から 1 になる速さ | この手順で作る |

### 4-19. 描く関数を書く

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **93行目**（`}` の下の空の行）の下（ファイルの最後）に、6行足します（94〜99行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 94 -->
<table class="code-lines"><tr><td>

```cpp
94
95
96
97
98
99
```

</td><td>

```cpp
void BossHud::Draw(RenderContext& rc)
{
	m_name.Draw(rc);
	m_hp.Draw(rc);
}

```

</td></tr></table>

- **なぜ:** 名前とゲージを描く。ボスがいなくても、フェードアウト中は描く必要がある。
- **効果:** ボスの名前と HP ゲージが、画面に出る。

### 4-20. 透明度を入れる関数を書く

[BossHud.cpp](../../../../Game/Source/UI/Hud/BossHud.cpp) の **99行目**（`}` の下の空の行）の下（ファイルの最後）に、7行足します（100〜106行目になります）。

<!-- add: Action/Game/Source/UI/Hud/BossHud.cpp 100 -->
<table class="code-lines"><tr><td>

```cpp
100
101
102
103
104
105
106
```

</td><td>

```cpp
void BossHud::SetAlpha(float alpha)
{
	m_alpha = alpha;
	// 名前とゲージに同じ透明度を入れる。0 のときは部品の側で描かない。
	m_name.SetAlpha(alpha);
	m_hp.SetAlpha(alpha);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 104〜105 | 名前とゲージに同じ透明度を入れる。0 のときは部品の側で描かない |

- **なぜ:** 名前とゲージには同じ透明度を入れたい。1か所にまとめれば、入れ忘れない。
- **効果:** 名前とゲージが、同じ透明度でフェードする。

### 4-21. UIManager.h で BossHud.h を読み込む

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **3行目** `#include "PlayerHud.h"` の下に、1行足します（4〜4行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 4 -->
<table class="code-lines"><tr><td>

```cpp
4
```

</td><td>

```cpp
#include "BossHud.h"
```

</td></tr></table>

- **なぜ:** `UIManager` が `BossHud` を持つので、`.h` を読み込む。
- **効果:** `UIManager.h` で、`BossHud` が使えるようになる。

### 4-22. Character があることを知らせる

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **6行目** `class Player;` の下に、1行足します（7〜7行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 7 -->
<table class="code-lines"><tr><td>

```cpp
7
```

</td><td>

```cpp
class Character;
```

</td></tr></table>

- **なぜ:** `ShowBoss` が `Character` のポインターを受け取るだけなので、前方宣言で足りる。`.h` を読み込まずに済み、コンパイルが速くなる。
- **効果:** `UIManager.h` で、`Character*` が使えるようになる。

### 4-23. ボスの HP を出す・消す関数を足す

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **23行目** `}` の下に、9行足します（24〜32行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 24 -->
<table class="code-lines"><tr><td>

```cpp
24
25
26
27
28
29
30
31
32
```

</td><td>

```cpp
	// ボスの HPバーを出す。ボスを DeleteGO する前に、必ず HideBoss() を呼ぶ。
	void ShowBoss(const Character* boss, const wchar_t* name)
	{
		m_bossHud.Show(boss, name);
	}
	void HideBoss()
	{
		m_bossHud.Hide();
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 24 | ボスを消す前に、必ず `HideBoss()` を呼ぶ |

- **なぜ:** ゲームの側は `BossHud` を知らなくてよいよう、`UIManager` に窓口を作る。
- **効果:** ボスの HP バーを、`UIManager` を通して出したり消したりできるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `ShowBoss` | ボスの HP バーを出すため | `BossHud::Show` を呼ぶ | この手順で作る |
| 関数 | `HideBoss` | ボスの HP バーを消すため | `BossHud::Hide` を呼ぶ。ボスを `DeleteGO` する前に必ず呼ぶ | この手順で作る |

### 4-24. BossHud を持つメンバーを足す

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **35行目** `PlayerHud m_playerHud;` の下に、1行足します（36〜36行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.h 36 -->
<table class="code-lines"><tr><td>

```cpp
36
```

</td><td>

```cpp
	BossHud m_bossHud;
```

</td></tr></table>

- **なぜ:** `PlayerHud` と同じく、`UIManager` が持つ。
- **効果:** `UIManager` が、ボスの HUD も準備・更新・描画できるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_bossHud` | ボスの HUD を持つため | `BossHud` 1つ | この手順で作る |

### 4-25. 最初に BossHud を準備する

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **7行目** `m_playerHud.Init();` の下に、1行足します（8〜8行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 8 -->
<table class="code-lines"><tr><td>

```cpp
8
```

</td><td>

```cpp
	m_bossHud.Init();
```

</td></tr></table>

- **なぜ:** `BossHud` も、`Start` で1回だけ準備する。
- **効果:** ボスの HUD の部品が、始まるときに準備される。

### 4-26. 毎フレーム BossHud を更新する

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **15行目** `m_playerHud.Update(deltaTime);` の下に、1行足します（16〜16行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 16 -->
<table class="code-lines"><tr><td>

```cpp
16
```

</td><td>

```cpp
	m_bossHud.Update(deltaTime);
```

</td></tr></table>

- **なぜ:** ボスの HP は毎フレーム変わる。`PlayerHud` と同じく、毎フレーム `Update` を呼ぶ。
- **効果:** ボスの HUD が、毎フレーム更新される。

### 4-27. BossHud を描く

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **22行目** `m_playerHud.Draw(rc);` の下に、1行足します（23〜23行目になります）。

<!-- add: Action/Game/Source/UI/UIManager.cpp 23 -->
<table class="code-lines"><tr><td>

```cpp
23
```

</td><td>

```cpp
	m_bossHud.Draw(rc);
```

</td></tr></table>

- **なぜ:** 後に描いたものが上に重なる。`PlayerHud` のあとに描く。
- **効果:** ボスの HUD が、画面に出る。

### 出来上がった BossHud.h

<!-- full: Action/Game/Source/UI/Hud/BossHud.h -->
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
```

</td><td>

```cpp
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
```

</td></tr></table>

### 出来上がった BossHud.cpp

<!-- full: Action/Game/Source/UI/Hud/BossHud.cpp -->
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
67
68
69
70
71
72
73
74
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
91
92
93
94
95
96
97
98
99
100
101
102
103
104
105
106
```

</td><td>

```cpp
#include "stdafx.h"
#include "BossHud.h"

#include "Character.h"
#include "UISprite.h"

namespace
{
	// HPゲージ。画面上の中央に、横長に置く（Player のゲージは画面下なので重ならない）。
	constexpr float HP_WIDTH = 720.0f;
	constexpr float HP_HEIGHT = 12.0f;
	constexpr float HP_BORDER = 2.0f;
	constexpr float HP_TOP = 100.0f;  // 画面の上端から、ゲージの中央までの距離。
	// HPゲージの左端の中央。
	const Vector2 HP_POSITION = { -HP_WIDTH * 0.5f, static_cast<float>(UI_SPACE_HEIGHT) * 0.5f - HP_TOP };
	const Vector4 HP_COLOR = { 0.9f, 0.28f, 0.22f, 1.0f };      // 赤。
	const Vector4 HP_BACK_COLOR = { 0.08f, 0.08f, 0.08f, 0.55f };
	const Vector4 HP_BORDER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };
	const Vector4 HP_TRAIL_COLOR = { 1.0f, 0.92f, 0.8f, 0.8f };  // 少し暖かい白。

	// 名前は HPゲージの上に小さく。
	constexpr float NAME_SCALE = 0.55f;
	constexpr float NAME_GAP = 24.0f;  // ゲージの中央から、名前の中央までの距離。
	const Vector2 NAME_POSITION = { 0.0f, HP_POSITION.y + NAME_GAP };
	const Vector4 NAME_SHADOW_COLOR = { 0.0f, 0.0f, 0.0f, 0.7f };
	constexpr float NAME_SHADOW_OFFSET = 1.5f;

	constexpr float TRAIL_DELAY = 0.5f;  // 減ってから、遅れて減るバーが縮み始めるまでの秒数。
	constexpr float TRAIL_SPEED = 0.5f;  // 1秒にゲージ全体の 50% 縮む。
	constexpr float FADE_SECONDS = 0.3f; // フェードイン・アウトにかける秒数。
}

void BossHud::Init()
{
	Gauge::Settings hp;
	hp.width = HP_WIDTH;
	hp.height = HP_HEIGHT;
	hp.borderSize = HP_BORDER;
	hp.borderColor = HP_BORDER_COLOR;
	hp.backColor = HP_BACK_COLOR;
	hp.barColor = HP_COLOR;
	hp.barImagePath = UISprite::GAUGE_PATH;
	hp.useTrail = true;
	hp.trailColor = HP_TRAIL_COLOR;
	hp.trailDelay = TRAIL_DELAY;
	hp.trailSpeed = TRAIL_SPEED;
	m_hp.Init(hp);
	m_hp.SetPosition(HP_POSITION);

	m_name.SetPosition(NAME_POSITION);
	m_name.SetScale(NAME_SCALE);
	m_name.SetShadow(NAME_SHADOW_OFFSET, NAME_SHADOW_COLOR);

	SetAlpha(0.0f);
}

void BossHud::Show(const Character* boss, const wchar_t* name)
{
	m_boss = boss;
	m_name.SetText(name);
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}
	// 戦い始めに白いバーが減っていく演出にならないよう、今のHPにそろえる。
	m_hp.ResetTrail();
	m_targetAlpha = 1.0f;
}

void BossHud::Hide()
{
	m_boss = nullptr;
	m_targetAlpha = 0.0f;
}

void BossHud::Update(float deltaTime)
{
	if (m_boss != nullptr) {
		m_hp.SetRate(m_boss->GetStatus().GetHpRate());
	}

	// 目標の透明度へ、一定の速さで近づける。
	const float step = deltaTime / FADE_SECONDS;
	if (m_alpha < m_targetAlpha) {
		SetAlpha(min(m_targetAlpha, m_alpha + step));
	}
	else if (m_alpha > m_targetAlpha) {
		SetAlpha(max(m_targetAlpha, m_alpha - step));
	}

	m_name.Update(deltaTime);
	m_hp.Update(deltaTime);
}

void BossHud::Draw(RenderContext& rc)
{
	m_name.Draw(rc);
	m_hp.Draw(rc);
}

void BossHud::SetAlpha(float alpha)
{
	m_alpha = alpha;
	// 名前とゲージに同じ透明度を入れる。0 のときは部品の側で描かない。
	m_name.SetAlpha(alpha);
	m_hp.SetAlpha(alpha);
}
```

</td></tr></table>

**確かめ方（これで出来上がり）:** `UIManager::ShowBoss()` を呼ぶと、画面上の中央に名前と赤い HP ゲージがフェードインして出る。`HideBoss()` でフェードアウトする。
