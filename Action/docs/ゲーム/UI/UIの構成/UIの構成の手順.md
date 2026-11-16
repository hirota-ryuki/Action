# UIの構成の手順

設計: [UIの構成の設計](UIの構成の設計.md)
経緯: [UIの経緯](../UIの経緯.md)

画面の UI の部品（`UIElement`・`Text`・`Icon`）と、UI をまとめて持つ `UIManager` を作り、`Game` から作るまでの手順です。

**出来上がるもの:** `Game` が `UIManager` を作り、毎フレーム更新して描く流れができます。まだ何も描かないので、画面は変わりません。[HUDの手順](../HUD/HUDの手順.md) で、ここで作った部品を使って HP ゲージなどを出します。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- ゲームの手順書は [README の「ゲームを作る順番」](../../../README.md#ゲームを作る順番) の順に進める。この手順の前に、[炎魔法発射後の燃える床の手順](../../魔法/炎魔法発射後の燃える床/炎魔法発射後の燃える床の手順.md) まで済ませておく。

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
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる変数・引数・関数・型・マクロ・C++ の書き方を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、このゲームで作ったもの・エンジン（k2Engine）の機能・C++ の標準ライブラリ・C++ の書き方のどれかがわかります。
- [前提知識](../../../前提知識.md) で「よく知っている」にあるもの（クラス・継承・ポインター・テンプレート・`std::unique_ptr`・`K2_ASSERT`・`nullptr` など）は、表に載せません。「まあまあ」「怪しい」「微妙」にあるもの（`std::make_unique`・`std::move` など）は載せます。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) | 新しく作る | 1 |
| [Text.h](../../../../Game/Source/UI/Elements/Text.h) | 新しく作る | 2 |
| [FontEngine.h](../../../../k2EngineLow/graphics/font/FontEngine.h) | 追加 | 2 |
| [Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) | 新しく作る | 2 |
| [UISprite.h](../../../../Game/Source/UI/Elements/UISprite.h) | 新しく作る | 3 |
| [Icon.h](../../../../Game/Source/UI/Elements/Icon.h) | 新しく作る | 3 |
| [Icon.cpp](../../../../Game/Source/UI/Elements/Icon.cpp) | 新しく作る | 3 |
| [UIManager.h](../../../../Game/Source/UI/UIManager.h) | 新しく作る | 4 |
| [UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) | 新しく作る | 4 |
| [Game.h](../../../../Game/Source/Core/Game.h) | 追加 | 4 |
| [Game.cpp](../../../../Game/Source/Core/Game.cpp) | 追加・書き換え | 4 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: 部品の基底クラス UIElement を作る

部品は「言われた値を描く」だけで、キャラクターのことは知りません。座標は画面の中心が原点で、右が +x、上が +y です（画面は `UI_SPACE_WIDTH` × `UI_SPACE_HEIGHT` = 1920 × 1080）。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) | 新しく作る（位置・見えるか・透明度、描く流れ） |

`Source` フィルターの下に `UI` フィルター、その下に `Elements` フィルターを作り、`UIElement.h` を作ります。ファイルの場所は `Action/Game/Source/UI/Elements/` です。

### 1-1. UIElement クラスの形を書く

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) を新しく作り、次の8行を書きます（1〜8行目）。

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
- **なぜ:** Gauge・Icon・Text は、位置や見えるかなど同じ決まりを持つ。土台のクラスを先に作れば、決まりを1か所に書ける。
- **効果:** 部品をどれも `UIElement` として、同じ呼び方で扱えるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `UIElement` | UI の部品に共通の決まりを持たせるため | 部品の基底クラス。Gauge・Icon・Text はこれを継承する | この手順で作る |


### 1-2. 位置を決める・読む関数を足す

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の **7行目**（`virtual ~UIElement() = default;` の下の空の行）の下、`};` の上に、8行足します（8〜15行目になります）。

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

- **なぜ:** 位置は全部の部品が持つ。基底クラスに書けば、部品ごとに同じ関数を書かずに済む。
- **効果:** どの部品にも、同じ方法で位置を決めたり読んだりできる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetPosition` | 部品を画面のどこに置くか決めるため | 位置を覚える。座標は画面の中心が原点 | この手順で作る |
| 引数 | `position` | 置きたい位置を渡すため | 位置（`Vector2`）。`const &` なので、コピーせず読むだけ | この手順で作る |
| 型 | `Vector2` | x と y の2つで、画面の位置を表すため | 2次元の値。x と y を持つ | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h) |
| メンバー変数 | `m_position` | 決めた位置を覚えておくため | 部品の位置。宣言は 1-8 で書く | この手順で作る |
| 関数 | `GetPosition` | 今の位置を読むため | `m_position` を返す。`const &` で返すのでコピーしない | この手順で作る |

### 1-3. 見えるかを決める・読む関数を足す

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の **15行目** `}` の下、`};` の上に、9行足します（16〜24行目になります）。

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
- **なぜ:** 部品を一時的に隠したいことがある（ボスがいない間のゲージなど）。部品を消さずに、描かないだけにできる。
- **効果:** `SetVisible(false)` だけで隠せ、`true` に戻せばまた出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetVisible` | 部品を出すか隠すか決めるため | 見えるかを覚える | この手順で作る |
| 引数 | `isVisible` | 出す（`true`）・隠す（`false`）を渡すため | 見えるかの `bool` | この手順で作る |
| メンバー変数 | `m_isVisible` | 見えるかを覚えておくため | `true` なら見える。宣言は 1-8 で書く | この手順で作る |
| 関数 | `IsVisible` | 今見えるかを読むため | `m_isVisible` を返す | この手順で作る |


### 1-4. 透明度を決める関数を足す

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の **24行目** `}` の下、`};` の上に、5行足します（25〜29行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 25 -->
<table class="code-lines"><tr><td>

```cpp
25
26
27
28
29
```

</td><td>

```cpp
	// 透明度（0 で見えない、1 で不透明）。フェードに使う。部品の色のアルファに掛ける。
	void SetAlpha(float alpha)
	{
		m_alpha = min(1.0f, max(0.0f, alpha));
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 28 | 0（見えない）〜1（不透明）の外に出ないようにする。フェードに使う |

- **なぜ:** 透明度は、値をそのまま入れさせず、0〜1 の外に出ないようにして入れる。部品の色に掛けるのは、部品ごとの描く処理がする（`ApplyAlpha`）。
- **効果:** フェードで値を行き過ぎても、色がおかしくならない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetAlpha` | 部品の透明度を決めるため | 0〜1 に収めてから `m_alpha` に入れる | この手順で作る |

### 1-5. 更新と、描く関数を足す

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の **29行目** `}` の下、`};` の上に、11行足します（30〜40行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 30 -->
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
39
40
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
| 32 | 毎フレーム、描く前に1回呼ぶ。派生クラスが必ず書く |
| 34〜40 | 見えないとき（見えない設定か、透明度 0）は何もしない。見えるときだけ派生クラスの `OnDraw()` を呼ぶ。この関数は派生クラスで変えない |
- **なぜ:** 描く前に、位置や色を描画用のものへ移す時間が要る。また、見えないときに描かない判断は全部の部品で同じなので、`Draw` に1回だけ書く。
- **効果:** 派生クラスは、`Update` と `OnDraw` だけ書けばよい。見えない部品を描いてしまう間違いも防げる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Update` | 毎フレーム、描く前に値を描画用のものへ反映するため | `= 0` なので、派生クラスが必ず中身を書く。`IGameObject` の `Update` とは別のもの | この手順で作る |
| 引数 | `deltaTime` | 部品が時間で動くときに使うため | 前のフレームからの経過時間（秒） | この手順で作る（渡すのは 4-4 の `UIManager`） |
| 関数 | `Draw` | 部品を描くため | 見えるときだけ `OnDraw` を呼ぶ。派生クラスでは変えない | この手順で作る |
| 引数 | `rc` | 描く先を渡すため | `RenderContext`。`&` なので本物を渡す | k2EngineLow（エンジン）。`Render` が受け取ったものをそのまま渡す |
| 関数 | `OnDraw` | 派生クラスごとに描き方を変えるため | 本当に描く処理。宣言は 1-6 で書く | この手順で作る |


### 1-6. 派生クラスが書く、描く中身の関数を足す

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の **40行目** `}` の下、`};` の上に、4行足します（41〜44行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 41 -->
<table class="code-lines"><tr><td>

```cpp
41
42
43
44
```

</td><td>

```cpp

protected:
	virtual void OnDraw(RenderContext& rc) = 0;

```

</td></tr></table>

- **なぜ:** 描く中身は部品ごとに違う（文字・画像）。`Draw` からだけ呼ばせたいので、`public` ではなく `protected` に置く。
- **効果:** 外から `OnDraw` を直接呼べなくなり、見えない部品を描いてしまうのを防げる。

### 1-7. 色に透明度を掛ける関数を足す

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の **44行目**（`virtual void OnDraw(RenderContext& rc) = 0;` の下の空の行）の下、`};` の上に、5行足します（45〜49行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 45 -->
<table class="code-lines"><tr><td>

```cpp
45
46
47
48
49
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
| 48 | 色のアルファに透明度を掛ける。部品ごとにフェードの処理を書かずに済む |

- **なぜ:** 色のアルファに透明度を掛ける処理は、どの部品でも同じ。基底クラスに1回書けば、部品ごとに書かずに済む。
- **効果:** 部品は `ApplyAlpha` を通した色を渡すだけで、フェードに対応できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `ApplyAlpha` | フェード中の色を作るため | 色のアルファ（`w`）に `m_alpha` を掛けた新しい色を返す | この手順で作る |

### 1-8. 持つ値を足す

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の **49行目** `}` の下、`};` の上に、6行足します（50〜55行目になります）。

<!-- add: Action/Game/Source/UI/Elements/UIElement.h 50 -->
<table class="code-lines"><tr><td>

```cpp
50
51
52
53
54
55
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
| 51 | 位置。`protected` なので、派生クラスが描くときに使える |
- **なぜ:** 位置は派生クラスが描くときに読むので `protected`、見えるかと透明度は `Set`・`Get` 関数だけで触らせたいので `private` にする。
- **効果:** 値が決まった入口（`SetAlpha` など）を通るので、範囲の外の値が入らない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `Vector2::Zero` | 位置の最初の値を決めるため | x も y も 0 の `Vector2`（画面の中心） | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h) |


### 1-9. コメントを書く

[UIElement.h](../../../../Game/Source/UI/Elements/UIElement.h) の **2行目**（`#pragma once` の下の空の行）の下、`class UIElement` の上に、3行足します（3〜5行目になります）。

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

- **なぜ:** この部品が何で、座標がどうなっているかを、あとで読む人（自分）がすぐ思い出せるようにする。
- **効果:** 座標の向き（右が +x、上が +y）を間違えずに部品を使える。

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
| [Text.h](../../../../Game/Source/UI/Elements/Text.h) | 新しく作る（文字・色・大きさ・寄せ方・影） |
| [FontEngine.h](../../../../k2EngineLow/graphics/font/FontEngine.h) | 追加（文字の幅と高さを測る関数） |
| [Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) | 新しく作る（文字を測る、寄せる） |

### 2-1. Text クラスの形を書く

[Text.h](../../../../Game/Source/UI/Elements/Text.h) を新しく作り、次の8行を書きます（1〜8行目）。

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
- **なぜ:** 文字を出す部品を、`UIElement` から継承して作る。最初は空の形だけ書いて、少しずつ足す。
- **効果:** 位置・見えるか・透明度を、書かずにそのまま使える。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Text` | 画面に文字を出すため | 文字の部品。`UIElement` を継承する | この手順で作る |


### 2-2. 寄せ方の種類を書く

[Text.h](../../../../Game/Source/UI/Elements/Text.h) の **7行目** `public:` の下、`};` の上に、7行足します（8〜14行目になります）。

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
- **なぜ:** 文字を置く位置の基準が、左端・中央・右端のどれかを選べるようにする。数値より名前の方が間違えにくいので `enum class` にする。
- **効果:** 右寄せ（HP の数字など）や中央寄せ（見出し）を、名前で指定できる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Align` | 寄せ方を名前で選ぶため | 左・中央・右のどれか。`enum class` | この手順で作る |
| 値 | `Left`・`Center`・`Right` | どこを位置の基準にするか決めるため | `Align` の3つの値。文字の左端・中央・右端が位置に来る | この手順で作る |


### 2-3. 文字を決める関数を宣言する

[Text.h](../../../../Game/Source/UI/Elements/Text.h) の **14行目**（`};` の下の空の行）の下、`};` の上に、3行足します（15〜17行目になります）。

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

- **なぜ:** 文字が毎フレーム同じなら、描くものへ渡し直すのは無駄。変わったときだけ書き換える関数を用意する。
- **効果:** HP などの値を毎フレーム渡しても、重くならない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetText` | 表示する文字を決めるため | 文字を覚えて、幅と高さを測る。中身は 2-12 で書く | この手順で作る |
| 引数 | `text` | 出したい文字を渡すため | 文字列。`%` は `%%` と書く | この手順で作る |
| 型 | `wchar_t` | 日本語などの文字を扱うため | 1文字を2バイトで持つ文字の型。`const wchar_t*` は文字列。`L"…"` で書く | C++ の書き方 |

### 2-4. 色と大きさを決める関数を足す

[Text.h](../../../../Game/Source/UI/Elements/Text.h) の **17行目** `void SetText(const wchar_t* text);` の下に、8行足します（18〜25行目になります）。

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

- **なぜ:** 色と大きさは、文字ごとに変えたい。値だけ覚えておいて、描くときに使う。
- **効果:** 文字の色と大きさを、あとから自由に変えられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetColor` | 文字の色を決めるため | 色を覚える | この手順で作る |
| 引数 | `color` | 色を渡すため | `Vector4`（赤・緑・青・アルファ） | この手順で作る |
| メンバー変数 | `m_color` | 色を覚えておくため | 文字の色。宣言は 2-7 で書く | この手順で作る |
| 関数 | `SetScale` | 文字の大きさを決めるため | 大きさの倍率を覚える | この手順で作る |
| 引数 | `scale` | 大きさの倍率を渡すため | 1 でそのままの大きさ | この手順で作る |
| メンバー変数 | `m_scale` | 大きさを覚えておくため | 倍率。宣言は 2-7 で書く | この手順で作る |

### 2-5. 寄せ方と影を決める関数を足す

[Text.h](../../../../Game/Source/UI/Elements/Text.h) の **25行目** `}` の下、`};` の上に、10行足します（26〜35行目になります）。

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
- **なぜ:** 寄せ方と影も、文字ごとに変えたい。影（縁取り）を付けると、明るい背景でも文字が読みやすくなる。
- **効果:** 文字ごとに、寄せ方と影の有無を決められる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetAlign` | 寄せ方を決めるため | 寄せ方を覚える | この手順で作る |
| 引数 | `align` | 寄せ方を渡すため | `Align` の値 | この手順で作る |
| メンバー変数 | `m_align` | 寄せ方を覚えておくため | 宣言は 2-7 で書く | この手順で作る |
| 関数 | `SetShadow` | 文字に影を付けるため | ずらす量と色を覚える | この手順で作る |
| 引数 | `offset` | 影をどれだけずらすか渡すため | ずらすピクセル数 | この手順で作る |
| メンバー変数 | `m_shadowOffset`・`m_shadowColor` | 影の値を覚えておくため | 影のずらす量と色。宣言は 2-7 で書く | この手順で作る |


### 2-6. 更新と、描く関数を宣言する

[Text.h](../../../../Game/Source/UI/Elements/Text.h) の **35行目** `}` の下、`};` の上に、6行足します（36〜41行目になります）。

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

- **なぜ:** `UIElement` で決めた、毎フレームの `Update` と描く `OnDraw` の中身を、文字用に書く約束をする。`override` を付けると、名前や引数の間違いをビルドで気づける。
- **効果:** 次の手順から、文字用の `Update` と `OnDraw` を書ける。

### 2-7. 持つ値を足す

[Text.h](../../../../Game/Source/UI/Elements/Text.h) の **41行目**（`void OnDraw(RenderContext& rc) override;` の下の空の行）の下、`};` の上に、9行足します（42〜50行目になります）。

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
- **なぜ:** 文字を描くものと、文字・大きさ・色などの値をまとめて持つ。測った幅と高さを覚えておけば、毎フレーム測り直さずに済む。
- **効果:** 寄せ方に合わせた位置を、毎フレーム安く求められる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_font` | 文字を画面に描くため | エンジンの文字を描くもの（`FontRender`） | k2Engine（エンジン）の [FontRender.h](../../../../k2Engine/graphics/FontRender.h) |
| 型 | `std::wstring` | `wchar_t` の文字列を持つため | `std::string` の、`wchar_t` 版 | C++ の標準ライブラリ（<string>。stdafx.h で読み込み済み） |
| メンバー変数 | `m_text` | 今の文字を覚えておくため | 表示している文字。同じ文字が来たかを比べるのにも使う | この手順で作る |
| メンバー変数 | `m_textSize` | 寄せるために、文字の幅と高さを覚えるため | 大きさ 1 のときの幅と高さ。`SetText` のときに測る | この手順で作る |
| 値 | `g_vec4White` | 色の最初の値を白にするため | 白（1, 1, 1, 1）の `Vector4` | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h) |


### 2-8. コメントを書く

[Text.h](../../../../Game/Source/UI/Elements/Text.h) の **4行目**（`#include "UIElement.h"` の下の空の行）の下、`class Text : public UIElement` の上に、1行足します（5〜5行目になります）。

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

- **なぜ:** 位置の基準（横は寄せ方の端、縦は文字の中央）は、コードを見ただけではわからない。
- **効果:** 部品を使う人が、位置の決まりを間違えない。

### 2-9. エンジンに、文字の幅を測る関数を足す

エンジンの [FontEngine.h](../../../../k2EngineLow/graphics/font/FontEngine.h) に足します。

[FontEngine.h](../../../../k2EngineLow/graphics/font/FontEngine.h) の **52行目** `void EndDraw(RenderContext& rc);` の下に、9行足します（53〜61行目になります）。

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
- **なぜ:** 右寄せ・中央寄せには、文字の幅が要る。エンジンのフォントの機能（DirectXTK の `SpriteFont`）が幅を測れるので、それを呼ぶ関数を足す。
- **効果:** 文字の幅と高さが、描く前にピクセルでわかるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `MeasureString` | 文字の幅と高さを知るため | 文字列を大きさ 1 で描いたときの幅と高さを返す | k2EngineLow（エンジン）の [FontEngine.h](../../../../k2EngineLow/graphics/font/FontEngine.h)（この手順で足す） |
| 型 | `DirectX::XMFLOAT2` | 幅と高さの2つの値を受け取るため | float を2つ持つ型 | 外部ライブラリ（DirectXMath。エンジンが読み込み済み） |
| 関数 | `DirectX::XMStoreFloat2` | 計算用の値を `XMFLOAT2` に取り出すため | `SpriteFont` が返した値を `XMFLOAT2` に入れる | 外部ライブラリ（DirectXMath。エンジンが読み込み済み） |
| メンバー変数 | `m_spriteFont` | 文字の幅を測るため | 文字を描くための `DirectX::SpriteFont`。`MeasureString` で測れる | k2EngineLow（エンジン）の [FontEngine.h](../../../../k2EngineLow/graphics/font/FontEngine.h)（型は外部ライブラリ DirectXTK） |


### 2-10. Text.cpp を作り、使うものを読み込む

[Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) を新しく作り、次の3行を書きます（1〜3行目）。

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

- **なぜ:** `Text.h` に書いた関数の中身を書くための、`.cpp` を作る。
- **効果:** 次の手順から、`Text::` の関数を書ける。

### 2-11. 文字の位置の基準を決める

[Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) の **3行目**（`#include "Text.h"` の下の空の行）の下（ファイルの最後）に、7行足します（4〜10行目になります）。

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

- **なぜ:** エンジンの `Font` は、基準（pivot）をピクセルで扱う。左上を基準にして、寄せるのは自分で位置をずらして行う方が、3つの寄せ方を同じ書き方で扱える。
- **効果:** 文字の位置が、左端と上端で決まる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | 名前のない `namespace { … }` | この `.cpp` の中だけで使う名前を作るため | ほかのファイルから見えない。名前がぶつからない | C++ の書き方 |
| 値 | `TOP_LEFT_PIVOT` | 文字の左上を基準にするため | `{ 0, 1 }`。エンジンに渡すと、位置が文字の左上になる | この手順で作る |

### 2-12. 文字を決める関数を書く

[Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) の **10行目**（`}` の下の空の行）の下（ファイルの最後）に、7行足します（11〜17行目になります）。

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
- **なぜ:** 文字を決めたときに、描くものへ渡し、幅と高さも測っておく。測るのは文字が変わったときだけで済む。
- **効果:** `SetText` を呼ぶだけで、寄せ方に使う幅と高さが用意される。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `g_graphicsEngine` | エンジンの描く機能を使うため | 描く機能の本体（`GraphicsEngine`）を指すポインター | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |
| 関数 | `GetFontEngine` | 文字を測る機能を取り出すため | `FontEngine` を返す | k2EngineLow（エンジン）の [GraphicsEngine.h](../../../../k2EngineLow/graphics/GraphicsEngine.h) |


### 2-13. 文字が変わっていなければ何もしない

[Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) の **12行目** `{` の下、`m_text = text;` の上に、6行足します（13〜18行目になります）。

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

- **なぜ:** `nullptr` が来ても落ちないように、空の文字にしておく。同じ文字なら、測り直しも描き直しもしない。
- **効果:** 毎フレーム `SetText` を呼んでも重くならず、`nullptr` でも落ちない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `L""` | 空の `wchar_t` の文字列を書くため | `L` を付けると `wchar_t` の文字列になる。`L""` は空 | C++ の書き方 |

### 2-14. 毎フレームの関数を書き、幅と高さを求める

[Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) の **23行目**（`}` の下の空の行）の下（ファイルの最後）に、7行足します（24〜30行目になります）。

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

- **なぜ:** 位置を決めるには、まず文字の幅と高さ（大きさ 1 のときの値 × 倍率）が要る。
- **効果:** 倍率を変えても、寄せる位置が正しく求まる。

### 2-15. 寄せ方に合わせて、左端と上端を求める

[Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) の **28行目**（`const float height = m_textSize.y * m_scale;` の下の空の行）の下、`}` の上に、9行足します（29〜37行目になります）。

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
- **なぜ:** 位置は「寄せ方の端」で、エンジンに渡すのは「左端と上端」。そのため、寄せ方に合わせて幅の分だけずらす。
- **効果:** 右寄せ・中央寄せの文字が、指定した位置に収まる。


### 2-16. 描くものに渡す

[Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) の **37行目** `const float top = m_position.y + height * 0.5f;` の下に、6行足します（38〜43行目になります）。

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

- **なぜ:** 求めた位置・大きさ・色を、描くものに渡す。色には透明度を掛ける。
- **効果:** フェード中も文字と影の色が合い、寄せ方が画面に出る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetPivot` | 位置の基準を決めるため | 文字の左上からのピクセル数で基準を決める（`TOP_LEFT_PIVOT` を渡す） | k2Engine（エンジン）の [FontRender.h](../../../../k2Engine/graphics/FontRender.h) |
| 関数 | `SetPosition`（文字） | 文字を置くため | 左端・上端・奥行き（0）を渡して、文字の位置を決める | k2Engine（エンジン）の [FontRender.h](../../../../k2Engine/graphics/FontRender.h) |
| 関数 | `SetShadowParam` | 影を付けるため | 影を描くか・ずらす量・色を決める。ずらす量が 0 なら描かない | k2Engine（エンジン）の [FontRender.h](../../../../k2Engine/graphics/FontRender.h) |

### 2-17. 描く中身の関数を書く

[Text.cpp](../../../../Game/Source/UI/Elements/Text.cpp) の **45行目**（`}` の下の空の行）の下（ファイルの最後）に、7行足します（46〜52行目になります）。

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

- **なぜ:** 文字が空のとき、描く必要はない。空でなければ、描くものに描かせる。
- **効果:** 空の文字で無駄に描かない。

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
| [UISprite.h](../../../../Game/Source/UI/Elements/UISprite.h) | 新しく作る（UI で使う画像のパス） |
| [Icon.h](../../../../Game/Source/UI/Elements/Icon.h) | 新しく作る（画像を1枚出す） |
| [Icon.cpp](../../../../Game/Source/UI/Elements/Icon.cpp) | 新しく作る（位置と色を渡して描く） |

### 3-1. UI で使う画像のパスを書く

[UISprite.h](../../../../Game/Source/UI/Elements/UISprite.h) を新しく作り、次の10行を書きます（1〜10行目）。

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
- **なぜ:** 画像のパスを文字で何か所にも書くと、直すときに探し回る。名前を付けて、1か所にまとめる。
- **効果:** 画像の場所を変えるときに、ここだけ直せばよい。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `UISprite::WHITE_PATH` | 白い画像の場所を名前で呼ぶため | 白い画像の `.dds` のパス。色を掛けて使う | この手順で作る |
| 値 | `UISprite::GAUGE_PATH` | ゲージ用の画像の場所を名前で呼ぶため | 上が白く下が少し暗い画像の `.dds` のパス | この手順で作る |


### 3-2. Icon クラスの形を書く

[Icon.h](../../../../Game/Source/UI/Elements/Icon.h) を新しく作り、次の9行を書きます（1〜9行目）。

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
- **なぜ:** 画像を1枚出す部品を、`UIElement` から継承して作る。最初は形と準備の関数だけ書く。
- **効果:** 位置・見えるか・透明度を、書かずにそのまま使える。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Icon` | 画像を画面に出すため | 画像の部品。`UIElement` を継承する | この手順で作る |
| 関数 | `Init` | 画像と大きさを決めるため | 画像のパスと幅・高さで準備する。中身は 3-6 で書く | この手順で作る |
| 引数 | `filePath`・`width`・`height` | どの画像を、どの大きさで出すか渡すため | 画像のパスと、幅・高さ（ピクセル） | この手順で作る |


### 3-3. 色を決める関数と、更新の関数を足す

[Icon.h](../../../../Game/Source/UI/Elements/Icon.h) の **8行目** `void Init(const char* filePath, float width, float height);` の下に、7行足します（9〜15行目になります）。

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
- **なぜ:** 色を掛けて、白い画像を好きな色の四角にできるようにする。毎フレームの更新は、`UIElement` で決めた約束どおりに書く。
- **効果:** 画像がなくても、白い画像に色を付けて仮のアイコンを出せる。


### 3-4. 描く関数と、持つ値を足す

[Icon.h](../../../../Game/Source/UI/Elements/Icon.h) の **15行目**（`void Update(float deltaTime) override;` の下の空の行）の下、`};` の上に、6行足します（16〜21行目になります）。

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
- **なぜ:** 描く中身は `OnDraw` に書く。画像を描くもの（`SpriteRender`）と、色を持つ。
- **効果:** 派生クラスとして、描く準備が整う。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_sprite` | 画像を画面に描くため | エンジンの画像を描くもの（`SpriteRender`） | この手順で作る |


### 3-5. コメントを書く

[Icon.h](../../../../Game/Source/UI/Elements/Icon.h) の **4行目**（`#include "UIElement.h"` の下の空の行）の下、`class Icon : public UIElement` の上に、2行足します（5〜6行目になります）。

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

- **なぜ:** 画像がないものを仮の四角で出す、というこの部品の使い方を書いておく。
- **効果:** あとから使う人が、画像がなくても使えるとわかる。

### 3-6. Icon.cpp を作り、準備の関数を書く

[Icon.cpp](../../../../Game/Source/UI/Elements/Icon.cpp) を新しく作り、次の8行を書きます（1〜8行目）。

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

- **なぜ:** 画像のパスと大きさは、最初に1回だけ描くものに渡せばよい。
- **効果:** `Init` を呼ぶだけで画像が出せる状態になる。

### 3-7. 更新と、描く関数を書く

[Icon.cpp](../../../../Game/Source/UI/Elements/Icon.cpp) の **8行目**（`}` の下の空の行）の下（ファイルの最後）に、11行足します（9〜19行目になります）。

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

- **なぜ:** 毎フレーム、位置と透明度を掛けた色を、描くものに渡す。描くのは `OnDraw` で行う。
- **効果:** 位置を動かしたり、フェードしたりしたとき、画像に反映される。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetMulColor` | 画像に色を掛けるため | 画像の色に、渡した色を掛ける。白い画像なら、その色になる | k2Engine（エンジン）の [SpriteRender.h](../../../../k2Engine/graphics/SpriteRender.h) |

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
| [UIManager.h](../../../../Game/Source/UI/UIManager.h) | 新しく作る（UI をまとめて持つ） |
| [UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) | 新しく作る（まだ何もしない） |
| [Game.h](../../../../Game/Source/Core/Game.h) | 追加（前方宣言と、メンバー） |
| [Game.cpp](../../../../Game/Source/Core/Game.cpp) | 追加（`#include`、作る、消す）・書き換え（優先度のコメント） |

`Source\UI` フィルターに `UIManager.h` と `UIManager.cpp` を作ります。

### 4-1. UIManager クラスの形を書く

[UIManager.h](../../../../Game/Source/UI/UIManager.h) を新しく作り、次の9行を書きます（1〜9行目）。

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
- **なぜ:** UI 全体をまとめて持つものを、`IGameObject` として作る。`Game` が作るので、ゲームの流れに乗って更新・描画される。
- **効果:** `Update` と `Render` が毎フレーム呼ばれるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `UIManager` | 画面の UI をまとめて持つため | UI の係。毎フレーム更新して描く | この手順で作る |


### 4-2. コメントを書く

[UIManager.h](../../../../Game/Source/UI/UIManager.h) の **2行目**（`#pragma once` の下の空の行）の下、`class UIManager : public IGameObject` の上に、4行足します（3〜6行目になります）。

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

- **なぜ:** `UIManager` を呼ぶのは `Game` だけにする、という決まりと、優先度を後ろにする理由を書いておく。
- **効果:** あとから「キャラクターが UI を直接呼ぶ」ような間違いをしにくくなる。

### 4-3. UIManager.cpp を作り、最初の関数を書く

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) を新しく作り、次の8行を書きます（1〜8行目）。

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

- **なぜ:** `.cpp` を作って、`Start` を書く。今は準備することがないので `true` を返すだけ。
- **効果:** ビルドが通り、`UIManager` を作れるようになる。

### 4-4. 毎フレームの関数と、描く関数を書く

[UIManager.cpp](../../../../Game/Source/UI/UIManager.cpp) の **8行目**（`}` の下の空の行）の下（ファイルの最後）に、9行足します（9〜17行目になります）。

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
| 11 | 経過時間。部品の `Update()` に渡す（[HUDの手順](../HUD/HUDの手順.md) で使う） |
| 16 | あとに描いたものが上に重なる |
- **なぜ:** 毎フレームの更新に使う経過時間をここで取り、あとで部品の `Update` へ渡す。描く順番も、ここで決める。
- **効果:** 次の手順書で、部品を足すだけで動くようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `g_gameTime` | ゲームの時間を調べるため | 時間をまとめて持つもの（`GameTime`）を指すポインター | k2EngineLow（エンジン）の [k2EngineLow.h](../../../../k2EngineLow/k2EngineLow.h) |
| 関数 | `GetFrameDeltaTime` | 部品の動きを時間で決めるため | 前のフレームからの経過時間（秒）を返す | k2EngineLow（エンジン）の [GameTime.h](../../../../k2EngineLow/time/GameTime.h) |


### 4-5. Game.h に UIManager があることを知らせる

[Game.h](../../../../Game/Source/Core/Game.h) の **6行目** `class BossStage;` の下に、1行足します（7行目になります）。

<!-- add: Action/Game/Source/Core/Game.h 7 -->
<table class="code-lines"><tr><td>

```cpp
7
```

</td><td>

```cpp
class UIManager;
```

</td></tr></table>

- **なぜ:** `Game.h` では `UIManager` のポインターを持つだけなので、`#include` せず「そういう型がある」とだけ知らせる。
- **効果:** `Game.h` を読み込む全ファイルが、`UIManager.h` を読み込まずに済み、ビルドが速くなる。

### 4-6. UIManager を持つメンバーを足す

[Game.h](../../../../Game/Source/Core/Game.h) の **28行目** `GameCamera* m_gameCamera = nullptr;` の下、`nsK2Engine::SkyCube* m_skyCube = nullptr;  // IBLの環境マップ生成も兼ねる。` の上に、1行足します（29行目になります）。

<!-- add: Action/Game/Source/Core/Game.h 29 -->
<table class="code-lines"><tr><td>

```cpp
29
```

</td><td>

```cpp
	UIManager* m_uiManager = nullptr;
```

</td></tr></table>
- **なぜ:** `Game` が作った `UIManager` を覚えておき、消すときに使う。
- **効果:** ゲームを終えるときに、`UIManager` を消せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_uiManager` | 作った `UIManager` を覚えておくため | `UIManager` を指すポインター。最初は `nullptr` | この手順で作る（`Game.h`） |


### 4-7. UIManager.h を読み込む

[Game.cpp](../../../../Game/Source/Core/Game.cpp) の **10行目** `#include "BurningGround.h"` の下に、1行足します（11行目になります）。

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

- **なぜ:** `Game.cpp` で `UIManager` を作って消すには、クラスの中身を知る必要がある。
- **効果:** `NewGO<UIManager>` と `DeleteGO` が書ける。

### 4-8. 終わるときに、UI を先に消す

[Game.cpp](../../../../Game/Source/Core/Game.cpp) の **30行目** `{` の下、`DeleteGO(m_bossStage);` の上に、2行足します（31〜32行目になります）。

<!-- add: Action/Game/Source/Core/Game.cpp 31 -->
<table class="code-lines"><tr><td>

```cpp
31
32
```

</td><td>

```cpp
	// UI はキャラクターを読んでいるので、キャラクターより先に消す。
	DeleteGO(m_uiManager);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 31〜32 | UI はキャラクターを読んでいるので、キャラクターより先に消す |

- **なぜ:** UI はキャラクターの値を読む。キャラクターを先に消すと、UI が消えたものを読んでしまう。
- **効果:** 終わるときに、消えたキャラクターを読んで落ちるのを防げる。

### 4-9. 優先度のコメントを書き換える

[Game.cpp](../../../../Game/Source/Core/Game.cpp) の **64行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/Game/Source/Core/Game.cpp 64 -->
<table class="code-lines"><tr><td>

```cpp
64
```

</td><td>

```cpp
	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ。
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/Game/Source/Core/Game.cpp 64 -->
<table class="code-lines"><tr><td>

```cpp
64
```

</td><td>

```cpp
	// 優先度0=ステージ（最初に配置）、優先度1=キャラ、優先度2=カメラ、優先度3=UI（キャラが動いたあとの値を出す）。
```

</td></tr></table>
- **なぜ:** UI を優先度 3 で作ることにしたので、優先度の説明に足しておく。
- **効果:** 優先度の決まりが、コメントを見ればわかる。


### 4-10. Start() で UIManager を作る

[Game.cpp](../../../../Game/Source/Core/Game.cpp) の **70行目** `m_gameCamera = NewGO<GameCamera>(2, "gameCamera");` の下に、1行足します（71行目になります）。

<!-- add: Action/Game/Source/Core/Game.cpp 71 -->
<table class="code-lines"><tr><td>

```cpp
71
```

</td><td>

```cpp
	m_uiManager = NewGO<UIManager>(3, "uiManager");
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 71 | 優先度 3 で作る。キャラクターとカメラが動いたあとに更新する |

- **なぜ:** 優先度 3 にすると、キャラクター（1）とカメラ（2）の更新が済んでから、UI が更新される。
- **効果:** キャラクターが動いた後の値を、同じフレームで出せる。

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

---

## もとからあるソースの変更前と変更後

この手順書で書き換える、もとからあるソース（エンジンなど）を、変更前（左）と変更後（右）で並べます。「行」は変更前の行番号です。片側が空いている行は、消した行か足した行です。両側が同じ行は、場所を示すための変えない行です。

[FontEngine.h](../../../../k2EngineLow/graphics/font/FontEngine.h)

| 行 | 変更前 | 変更後 |
|---|---|---|
| 52 | `void EndDraw(RenderContext& rc);` | `void EndDraw(RenderContext& rc);` |
|  |  | `/// <summary>` |
|  |  | `/// 文字列を大きさ 1 で描いたときの幅と高さ（ピクセル）を測る。右寄せ・中央寄せに使う。` |
|  |  | `/// </summary>` |
|  |  | `Vector2 MeasureString(const wchar_t* text) const` |
|  |  | `{` |
|  |  | `DirectX::XMFLOAT2 size;` |
|  |  | `DirectX::XMStoreFloat2(&size, m_spriteFont->MeasureString(text));` |
|  |  | `return Vector2(size.x, size.y);` |
|  |  | `}` |
| 53 | `private:` | `private:` |
