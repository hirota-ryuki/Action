# HUDの手順

設計: [HUDの設計](HUDの設計.md)
経緯: [UIの経緯](UIの経緯.md)

画面に Player の HP ゲージ・状態異常のアイコンと、ボスの HP ゲージを出すまでの手順です。

**出来上がるもの:** 画面下の中央に、Player の細い緑の HP ゲージと「今のHP / 最大HP」の数字が出ます。凍結中・燃焼中は、ゲージの右に水色・橙色の四角が出ます。ボスの HP ゲージ（画面上の中央）は、`UIManager::ShowBoss()` を呼んだときだけ出ます。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [UIの構成の手順](UIの構成の手順.md) を済ませておく（ゲームの手順書の順番は [README の「ゲームを作る順番」](../../README.md#ゲームを作る順番)）。この手順では、そこで作った `UIManager`・`Text`・`Icon`・`UISprite` を使う。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | 横向きのバーの部品 `Gauge` | ビルドが通る |
| 2 | `PlayerHud` で、Player の HP ゲージと数字を出す | 画面下に HP が出る |
| 3 | `PlayerHud` に、状態異常のアイコンを足す | 凍結・燃焼で四角が出る |
| 4 | `BossHud` で、ボスの名前と HP ゲージを出す | `ShowBoss()` で上に出る |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) | 新しく作る | 1 |
| [Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) | 新しく作る | 1 |
| [PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) | 新しく作る・追加 | 2・3 |
| [PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) | 新しく作る・追加 | 2・3 |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加 | 2・4 |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加 | 2・4 |
| [Game.cpp](../../../Game/Source/Core/Game.cpp) | 追加 | 2 |
| [BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) | 新しく作る | 4 |
| [BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) | 新しく作る | 4 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: 横向きのバーの部品 Gauge を作る

HP・クールダウンなどに使う、横向きのバーです。位置は「バーの左端の中央」で、中身は左端から右へ伸び縮みします。減った分が少し残ってから追いかけて減る「遅れて減るバー」も出せます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) | 新しく作る（ゲージの設定、クラスの宣言） |
| [Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) | 新しく作る（割合を決める、遅れて減るバー、描く） |

`Source\UI\Elements` フィルターに `Gauge.h` と `Gauge.cpp` を作ります。

### 1-1. Gauge クラスの形を書く

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) を新しく作り、次の9行を書きます（1〜9行目）。

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

### 1-2. ゲージの設定を書く（大きさと色）

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) の **8行目** `public:` の下、`};` の上に、10行足します（9〜18行目になります）。

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

### 1-3. 画像と、遅れて減るバーの設定を足す

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) の **16行目** `Vector4 barColor = { 0.85f, 0.15f, 0.15f, 1.0f };   // 中身。` の下に、5行足します（17〜21行目になります）。

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

### 1-4. 準備と、割合を決める関数を宣言する

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) の **23行目**（`};` の下の空の行）の下、`};` の上に、6行足します（24〜29行目になります）。

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

### 1-5. 割合と設定を読む関数を足す

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) の **29行目** `void ResetTrail();` の下に、9行足します（30〜38行目になります）。

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

### 1-6. 更新と、描く関数を宣言する

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) の **38行目**（`}` の下の空の行）の下、`};` の上に、5行足します（39〜43行目になります）。

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

### 1-7. バーを置く関数を宣言する

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) の **43行目**（`void OnDraw(RenderContext& rc) override;` の下の空の行）の下、`};` の上に、4行足します（44〜47行目になります）。

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

### 1-8. 持つ値を足す

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) の **47行目**（`void UpdateBar(SpriteRender& sprite, float x, float rate, const Vector4& color);` の下の空の行）の下、`};` の上に、8行足します（48〜55行目になります）。

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

### 1-9. コメントを書く

[Gauge.h](../../../Game/Source/UI/Elements/Gauge.h) の **5行目**（`#include "UISprite.h"` の下の空の行）の下、`class Gauge : public UIElement` の上に、4行足します（6〜9行目になります）。

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

### 1-10. Gauge.cpp を作り、使うものを読み込む

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) を新しく作り、次の5行を書きます（1〜5行目）。

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

### 1-11. 画像の基準を決める

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **5行目**（`#include "UISprite.h"` の下の空の行）の下（ファイルの最後）に、6行足します（6〜11行目になります）。

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

### 1-12. 準備の関数を書き、4枚の画像を読む

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **11行目**（`}` の下の空の行）の下（ファイルの最後）に、10行足します（12〜21行目になります）。

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

### 1-13. 画像の基準を左端の中央にする

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **19行目** `m_bar.Init(m_settings.barImagePath, m_settings.width, m_settings.height);` の下に、4行足します（20〜23行目になります）。

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

### 1-14. 割合を決める関数を書く

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **25行目**（`}` の下の空の行）の下（ファイルの最後）に、9行足します（26〜34行目になります）。

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

### 1-15. 減ったときは、遅れて減るバーを待たせる

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **28行目** `rate = min(1.0f, max(0.0f, rate));` の下に、4行足します（29〜32行目になります）。

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

### 1-16. 遅れて減るバーをそろえる関数を書く

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **38行目**（`}` の下の空の行）の下（ファイルの最後）に、6行足します（39〜44行目になります）。

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

### 1-17. 毎フレームの関数を書き、4枚を置く

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **44行目**（`}` の下の空の行）の下（ファイルの最後）に、10行足します（45〜54行目になります）。

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

### 1-18. 遅れて減るバーを縮める

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **46行目** `{` の下に、11行足します（47〜57行目になります）。

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

### 1-19. バーを置く関数を書く

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **65行目**（`}` の下の空の行）の下（ファイルの最後）に、8行足します（66〜73行目になります）。

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

### 1-20. 描く関数を書く

[Gauge.cpp](../../../Game/Source/UI/Elements/Gauge.cpp) の **73行目**（`}` の下の空の行）の下（ファイルの最後）に、12行足します（74〜85行目になります）。

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
| [PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) | 新しく作る（Player の HUD） |
| [PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) | 新しく作る（HP ゲージと数字） |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加（`#include`、Player を渡す関数、メンバー） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加（準備・更新・描く） |
| [Game.cpp](../../../Game/Source/Core/Game.cpp) | 追加（Player を渡す） |

`Source\UI` フィルターの下に `Hud` フィルターを作り、`PlayerHud.h` と `PlayerHud.cpp` を作ります。

### 2-1. PlayerHud クラスの形を書く

[PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) を新しく作り、次の12行を書きます（1〜12行目）。

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

### 2-2. Player を渡す関数と、更新・描く関数を足す

[PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) の **11行目** `void Init();` の下に、7行足します（12〜18行目になります）。

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

### 2-3. 持つ値を足す

[PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) の **18行目** `void Draw(RenderContext& rc);` の下に、5行足します（19〜23行目になります）。

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

### 2-4. コメントを書く

[PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) の **7行目**（`class Player;` の下の空の行）の下、`class PlayerHud` の上に、3行足します（8〜10行目になります）。

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

### 2-5. PlayerHud.cpp を作り、使うものを読み込む

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) を新しく作り、次の6行を書きます（1〜6行目）。

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

### 2-6. 値を置く場所を作る

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **6行目**（`#include "UISprite.h"` の下の空の行）の下（ファイルの最後）に、4行足します（7〜10行目になります）。

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

### 2-7. HP ゲージの大きさと置く高さを決める

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **8行目** `{` の下、`}` の上に、5行足します（9〜13行目になります）。

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

### 2-8. HP ゲージの位置と色を決める

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **13行目** `constexpr float HP_BOTTOM = 80.0f;  // 画面の下端から、ゲージの中央までの高さ。` の下に、7行足します（14〜20行目になります）。

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

### 2-9. HP の数字の見た目を決める

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **20行目**（`const Vector4 HP_TRAIL_COLOR = { 1.0f, 1.0f, 1.0f, 0.75f };` の下の空の行）の下、`}` の上に、6行足します（21〜26行目になります）。

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

### 2-10. 準備の関数を書く

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **28行目**（`}` の下の空の行）の下（ファイルの最後）に、8行足します（29〜36行目になります）。

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

### 2-11. HP ゲージの設定を入れる

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **31行目** `Gauge::Settings hp;` の下に、9行足します（32〜40行目になります）。

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

### 2-12. HP の数字を準備する

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **43行目**（`m_hp.SetPosition(HP_POSITION);` の下の空の行）の下、`}` の上に、4行足します（44〜47行目になります）。

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

### 2-13. 毎フレーム、HP の割合を入れる

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **49行目**（`}` の下の空の行）の下（ファイルの最後）に、12行足します（50〜61行目になります）。

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

### 2-14. HP の数字を作る

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **57行目**（`m_hp.SetRate(status.GetHpRate());` の下の空の行）の下、`m_hp.Update(deltaTime);` の上に、7行足します（58〜64行目になります）。

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

### 2-15. 描く関数を書く

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **68行目**（`}` の下の空の行）の下（ファイルの最後）に、8行足します（69〜76行目になります）。

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

### 2-16. UIManager.h で PlayerHud.h を読み込む

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **1行目** `#pragma once` の下に、4行足します（2〜5行目になります）。

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

### 2-17. Player を渡す関数と、PlayerHud を持つメンバーを足す

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **16行目** `void Render(RenderContext& rc) override;` の下に、8行足します（17〜24行目になります）。

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

### 2-18. 最初に PlayerHud を準備する

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **5行目** `{` の下、`return true;` の上に、2行足します（6〜7行目になります）。

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

### 2-19. 毎フレーム PlayerHud を更新する

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **13行目** `const float deltaTime = g_gameTime->GetFrameDeltaTime();` の下に、1行足します（14〜14行目になります）。

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

### 2-20. PlayerHud を描く

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **19行目** `// 後に描いたものが上に重なる。` の下に、1行足します（20〜20行目になります）。

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

### 2-21. Game から Player を渡す

[Game.cpp](../../../Game/Source/Core/Game.cpp) の **78行目** `m_uiManager = NewGO<UIManager>(3, "uiManager");` の下に、1行足します（79〜79行目になります）。

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

**確かめ方:** 起動すると、画面下の中央に緑の HP ゲージと「100 / 100」が出る。

---

## 手順3: PlayerHud に、状態異常のアイコンを足す

画像ができるまでは、色付きの四角で出します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) | 追加（`#include`、アイコン2つ） |
| [PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) | 追加（大きさと色、置く、見えるかを決める、更新・描く） |

### 3-1. Icon.h を読み込む

[PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) の **3行目** `#include "Gauge.h"` の下に、1行足します（4〜4行目になります）。

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

### 3-2. アイコンを持つメンバーを足す

[PlayerHud.h](../../../Game/Source/UI/Hud/PlayerHud.h) の **27行目** `Text m_hpNumber;  // 「今のHP / 最大HP」。` の下に、2行足します（28〜29行目になります）。

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

### 3-3. アイコンの大きさと色を決める

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **26行目** `constexpr float NUMBER_SHADOW_OFFSET = 1.5f;` の下に、6行足します（27〜32行目になります）。

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

### 3-4. アイコンを置く位置を求める

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **53行目** `m_hpNumber.SetShadow(NUMBER_SHADOW_OFFSET, NUMBER_SHADOW_COLOR);` の下に、5行足します（54〜58行目になります）。

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

### 3-5. アイコンを準備する

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **58行目**（`const float burningX = frozenX + ICON_SIZE + ICON_GAP;` の下の空の行）の下、`}` の上に、7行足します（59〜65行目になります）。

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

### 3-6. 状態異常のときだけ見えるようにする

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **82行目**（`m_hpNumber.SetText(number);` の下の空の行）の下、`m_hp.Update(deltaTime);` の上に、4行足します（83〜86行目になります）。

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

### 3-7. アイコンを更新する

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **88行目** `m_hpNumber.Update(deltaTime);` の下に、2行足します（89〜90行目になります）。

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

### 3-8. アイコンを描く

[PlayerHud.cpp](../../../Game/Source/UI/Hud/PlayerHud.cpp) の **99行目** `m_hpNumber.Draw(rc);` の下に、2行足します（100〜101行目になります）。

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
| [BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) | 新しく作る（ボスの HUD） |
| [BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) | 新しく作る（名前とゲージ、フェード） |
| [UIManager.h](../../../Game/Source/UI/UIManager.h) | 追加（`#include`、出す・消す関数、メンバー） |
| [UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) | 追加（準備・更新・描く） |

`Source\UI\Hud` フィルターに `BossHud.h` と `BossHud.cpp` を作ります。

### 4-1. BossHud クラスの形を書く

[BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) を新しく作り、次の12行を書きます（1〜12行目）。

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

### 4-2. 出す・消す関数を宣言する

[BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) の **11行目** `void Init();` の下に、5行足します（12〜16行目になります）。

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

### 4-3. 出しているか、更新・描く関数を足す

[BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) の **16行目** `void Hide();` の下に、6行足します（17〜22行目になります）。

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

### 4-4. 持つ値を足す

[BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) の **22行目** `void Draw(RenderContext& rc);` の下に、9行足します（23〜31行目になります）。

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

### 4-5. コメントを書く

[BossHud.h](../../../Game/Source/UI/Hud/BossHud.h) の **7行目**（`class Character;` の下の空の行）の下、`class BossHud` の上に、3行足します（8〜10行目になります）。

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

### 4-6. BossHud.cpp を作り、使うものを読み込む

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) を新しく作り、次の6行を書きます（1〜6行目）。

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

### 4-7. 値を置く場所を作る

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **6行目**（`#include "UISprite.h"` の下の空の行）の下（ファイルの最後）に、4行足します（7〜10行目になります）。

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

### 4-8. HP ゲージの大きさと置く高さを決める

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **8行目** `{` の下、`}` の上に、5行足します（9〜13行目になります）。

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

### 4-9. HP ゲージの位置と色を決める

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **13行目** `constexpr float HP_TOP = 100.0f;  // 画面の上端から、ゲージの中央までの距離。` の下に、7行足します（14〜20行目になります）。

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

### 4-10. 名前の見た目を決める

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **20行目**（`const Vector4 HP_TRAIL_COLOR = { 1.0f, 0.92f, 0.8f, 0.8f };  // 少し暖かい白。` の下の空の行）の下、`}` の上に、7行足します（21〜27行目になります）。

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

### 4-11. 遅れて減るバーとフェードの時間を決める

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **27行目**（`constexpr float NAME_SHADOW_OFFSET = 1.5f;` の下の空の行）の下、`}` の上に、3行足します（28〜30行目になります）。

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

### 4-12. 準備の関数を書く

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **32行目**（`}` の下の空の行）の下（ファイルの最後）に、9行足します（33〜41行目になります）。

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

### 4-13. HP ゲージの設定を入れる

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **35行目** `Gauge::Settings hp;` の下に、11行足します（36〜46行目になります）。

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

### 4-14. 名前を準備する

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **49行目**（`m_hp.SetPosition(HP_POSITION);` の下の空の行）の下、`SetAlpha(0.0f);` の上に、4行足します（50〜53行目になります）。

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

### 4-15. 出す関数を書く

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **56行目**（`}` の下の空の行）の下（ファイルの最後）に、12行足します（57〜68行目になります）。

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

### 4-16. 消す関数を書く

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **68行目**（`}` の下の空の行）の下（ファイルの最後）に、6行足します（69〜74行目になります）。

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

### 4-17. 毎フレーム、HP の割合を入れる

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **74行目**（`}` の下の空の行）の下（ファイルの最後）に、10行足します（75〜84行目になります）。

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

### 4-18. 目指す透明度へ近づける

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **80行目**（`}` の下の空の行）の下、`m_name.Update(deltaTime);` の上に、9行足します（81〜89行目になります）。

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

### 4-19. 描く関数を書く

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **93行目**（`}` の下の空の行）の下（ファイルの最後）に、6行足します（94〜99行目になります）。

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

### 4-20. 透明度を入れる関数を書く

[BossHud.cpp](../../../Game/Source/UI/Hud/BossHud.cpp) の **99行目**（`}` の下の空の行）の下（ファイルの最後）に、7行足します（100〜106行目になります）。

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

### 4-21. UIManager.h で BossHud.h を読み込む

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **3行目** `#include "PlayerHud.h"` の下に、1行足します（4〜4行目になります）。

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

### 4-22. Character があることを知らせる

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **6行目** `class Player;` の下に、1行足します（7〜7行目になります）。

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

### 4-23. ボスの HP を出す・消す関数を足す

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **23行目** `}` の下に、9行足します（24〜32行目になります）。

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

### 4-24. BossHud を持つメンバーを足す

[UIManager.h](../../../Game/Source/UI/UIManager.h) の **35行目** `PlayerHud m_playerHud;` の下に、1行足します（36〜36行目になります）。

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

### 4-25. 最初に BossHud を準備する

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **7行目** `m_playerHud.Init();` の下に、1行足します（8〜8行目になります）。

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

### 4-26. 毎フレーム BossHud を更新する

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **15行目** `m_playerHud.Update(deltaTime);` の下に、1行足します（16〜16行目になります）。

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

### 4-27. BossHud を描く

[UIManager.cpp](../../../Game/Source/UI/UIManager.cpp) の **22行目** `m_playerHud.Draw(rc);` の下に、1行足します（23〜23行目になります）。

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
