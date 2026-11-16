# NPCのAIの手順

設計: [NPCのAIの設計](NPCのAIの設計.md)
経緯: [入力の経緯](../入力/入力の経緯.md)

NPC の AI（`NpcInputSource`）を作り、Enemy の入力元にするまでの手順です。AI は「どうしたいか」だけを決めて `CharacterInput` に入れ、実際の移動や発射はキャラクター側が行います。

**出来上がるもの:** Enemy が、敵対する相手を探して近づき、射程に入ったら撃つ入力を作ります。今の Enemy の設定は、探す範囲を 0 にして止めてあります（動かすときは 900 に戻す）。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [入力の手順](../入力/入力の手順.md) を済ませておく（順番は [README の「ゲームを作る順番」](../../../README.md#ゲームを作る順番)）。相手は [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った `HitTargetRegistry` から探す。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | NPC の AI `NpcInputSource` | ビルドが通る |
| 2 | Enemy の入力元を AI にする | ビルドが通る |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- 消すところは、消す行を枠で示します（番号は消す前の番号）。書き換えるところは、「前」と「後」の枠を並べています。
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる変数・引数・関数・型・マクロ・C++ の書き方を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、このゲームで作ったもの・エンジン（k2Engine）の機能・C++ の標準ライブラリ・C++ の書き方のどれかがわかります。
- [前提知識](../../../前提知識.md) で「よく知っている」にあるもの（クラス・継承・`virtual`・`override`・ポインター・`nullptr`・`const`・`= default` など）は、表に載せません。「まあまあ」「怪しい」「微妙」にあるものは載せます。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) | 新しく作る | 1 |
| [NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) | 新しく作る | 1 |
| [Enemy.h](../../../../Game/Source/Enemy/Enemy.h) | 削除・追加・書き換え | 2 |
| [Enemy.cpp](../../../../Game/Source/Enemy/Enemy.cpp) | 追加 | 2 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: NPC の AI NpcInputSource を作る

AI も、プレイヤーの操作と同じく入力元（`IInputSource`）として作ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) | 新しく作る（AI の設定と、クラスの宣言） |
| [NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) | 新しく作る（相手を探して近づき、撃つ入力を作る） |

`Source\Character\Input` フィルターに `NpcInputSource.h` と `NpcInputSource.cpp` を作ります。

### 1-1. NpcInputSource クラスの形を書く

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) を新しく作り、次の11行を書きます（1〜11行目）。

<!-- new: Action/Game/Source/Character/Input/NpcInputSource.h -->
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
```

</td><td>

```cpp
#pragma once

#include "CharacterInput.h"

class NpcInputSource : public IInputSource
{
public:
	NpcInputSource() = default;
	explicit NpcInputSource(const Settings& settings);
	void Update(const InputSourceContext& context, CharacterInput& input) override;
};
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 5 | `IInputSource` を継承する |
| 8 | 設定を渡さずに作るときのコンストラクター |
| 9 | 設定（1-2・1-3）を渡して作るコンストラクター |
- **なぜ:** AI も入力元（`IInputSource`）にしておくと、プレイヤーの操作と同じ形でキャラクターに入力を渡せる。キャラクター側を、AI 用に作り変えなくてよい。
- **効果:** Enemy が持つ入力元を差し替えるだけで、プレイヤーの操作と AI を入れ替えられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `NpcInputSource` | 敵の動きを決めるため | NPC の AI の入力元。相手を探して近づき、撃つ入力を作る | この手順で作る |
| 型 | `IInputSource` | プレイヤーの操作も AI も、同じ形でキャラクターに入力を渡すため | 毎フレーム入力を作る `Update` を持つ基底クラス | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h)） |
| C++ の書き方 | `explicit` | 思いがけない自動の変換を防ぐため | 引数が1つのコンストラクターに付ける。`Settings` から `NpcInputSource` へ、勝手に変わらないようにする | C++ の書き方 |
| 型 | `Settings` | AI の設定（距離・時間）を1つにまとめるため | 探す範囲や撃つ間隔などの値を持つ構造体。1-2・1-3 で作る | この手順で作る |
| 関数 | `Update` | 毎フレーム、入力を作るため | `CharacterInput` に値を書き込む。`IInputSource` の `Update` を上書きする | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h) の `IInputSource`） |
| 型 | `InputSourceContext` | AI が、操作する自分のことを知るため | 自分の位置・陣営・自分自身・1フレームの時間など、入力元が知ってよい情報 | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h)） |
| 型 | `CharacterInput` | キャラクターが受け取る1フレーム分の入力をまとめるため | 移動の向き・ボタン・照準などを持つ構造体。AI はこれに書き込む | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h)） |


### 1-2. AI の設定 Settings を書く（探す・近づく）

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **7行目** `public:` の下、`NpcInputSource() = default;` の上に、7行足します（8〜14行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 8 -->
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
	struct Settings
	{
		float searchRange = 900.0f;    // この距離以内の敵対する相手を狙う。
		float stopDistance = 90.0f;    // この距離まで近づいたら止まる。
		float moveRate = 1.0f;         // 移動入力の大きさ（0〜1）。
	};

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 10 | この距離より近くにいる、敵対する相手を狙う |
| 11 | この距離まで近づいたら止まる |
| 12 | 移動の入力の大きさ（0〜1） |
- **なぜ:** 距離などの数値を AI のコードに直接書かず、設定の構造体にまとめる。数値の意味が名前でわかり、Enemy ごとに値を変えられる。
- **効果:** 数値を変えるだけで、探す範囲や近づき方を変えられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `searchRange` | 狙う相手を絞るため | この距離以内にいる、敵対する相手だけを狙う（既定 900） | この手順で作る（`Settings`） |
| メンバー変数 | `stopDistance` | 相手に近づきすぎないため | この距離まで近づいたら止まる（既定 90） | この手順で作る（`Settings`） |
| メンバー変数 | `moveRate` | 歩く速さを決めるため | 移動の入力の大きさ（0〜1）。1 なら全力で動く | この手順で作る（`Settings`） |


### 1-3. AI の設定に、撃つための値を足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **12行目** `float moveRate = 1.0f;         // 移動入力の大きさ（0〜1）。` の下に、6行足します（13〜18行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 13 -->
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
		bool canFire = true;           // 魔法を撃つか。
		float fireRange = 700.0f;      // この距離以内なら撃つ。
		float fireInterval = 4.0f;     // 撃つ間隔（秒）。
		float firstFireDelay = 1.5f;   // 相手を見つけてから最初に撃つまでの秒数。
		float fireHoldSeconds = 0.0f;  // 発射ボタンを押し続ける秒数。長押しの魔法用。0なら1フレームだけ押す。
		float castHeight = 60.0f;      // 足元からこの高さで撃つ。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13〜14 | 撃つか。この距離より近ければ撃つ |
| 15〜16 | 撃つ間隔と、相手を見つけてから最初に撃つまでの時間（秒） |
| 17 | 撃つボタンを押し続ける時間。長押しの魔法用。0 なら1フレームだけ押す |
| 18 | 足元からこの高さで撃つ |
- **なぜ:** 撃つかどうか・間隔・距離も、1-2 と同じく設定にまとめる。AI のコードを書き換えずに、撃ち方を変えられる。
- **効果:** 撃たない敵や、ゆっくり撃つ敵を、設定だけで作れるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `canFire` | 撃たない敵を作れるようにするため | `false` にすると、魔法を撃たない | この手順で作る（`Settings`） |
| メンバー変数 | `fireRange` | 遠すぎる相手に撃たないため | この距離以内にいるときだけ撃つ | この手順で作る（`Settings`） |
| メンバー変数 | `fireInterval` | 撃ちすぎないため | 1回撃ってから、次に撃つまでの秒数 | この手順で作る（`Settings`） |
| メンバー変数 | `firstFireDelay` | 見つけた瞬間に撃たないため | 相手を見つけてから、最初に撃つまでの秒数 | この手順で作る（`Settings`） |
| メンバー変数 | `fireHoldSeconds` | 長押しで撃つ魔法に使うため | 発射ボタンを押し続ける秒数。0 なら1フレームだけ押す | この手順で作る（`Settings`） |
| メンバー変数 | `castHeight` | 足元ではなく胸のあたりから撃つため | 足元からこの高さで撃つ | この手順で作る（`Settings`） |


### 1-4. 設定を変える関数を足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **22行目** `explicit NpcInputSource(const Settings& settings);` の下に、4行足します（23〜26行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 23 -->
<table class="code-lines"><tr><td>

```cpp
23
24
25
26
```

</td><td>

```cpp
	void SetSettings(const Settings& settings)
	{
		m_settings = settings;
	}
```

</td></tr></table>

- **なぜ:** 設定は、AI を作ったあとで変えたいことがある。設定を外から入れ替える関数があれば、作り直さずに済む。
- **効果:** AI を作ったあとでも、設定をまとめて差し替えられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `SetSettings` | あとから AI の設定を変えるため | 受け取った設定を、`m_settings` に入れる | この手順で作る |
| メンバー変数 | `m_settings` | AI が今の設定を覚えておくため | 今使っている `Settings`。1-5 で宣言する | この手順で作る |

### 1-5. 設定と、撃つための時間を持つメンバーを足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **27行目** `void Update(const InputSourceContext& context, CharacterInput& input) override;` の下に、7行足します（28〜34行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 28 -->
<table class="code-lines"><tr><td>

```cpp
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

private:
	Settings m_settings;
	float m_fireTimer = 0.0f;      // 次に撃てるまでの秒数。
	float m_fireHoldTimer = 0.0f;  // 発射ボタンを押し続ける残り秒数。
	bool m_hasTarget = false;      // 前フレームに相手を見つけていたか。
	bool m_isFiring = false;       // 発射ボタンを押している最中か。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 31 | 次に撃てるまでの残り時間 |
| 32 | 撃つボタンを押し続ける残り時間 |
| 33 | 前のフレームで相手を見つけていたか |
| 34 | 撃つボタンを押している最中か |
- **なぜ:** 時間や「前のフレームのこと」は、1回の `Update` では終わらない。次のフレームまで覚えておくために、メンバー変数にする。
- **効果:** 「次に撃てるまであと何秒か」「前のフレームで相手を見つけていたか」を、フレームをまたいで覚えられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_fireTimer` | 撃つ間隔を守るため | 次に撃てるまでの残り秒数。0 以下で撃てる | この手順で作る |
| メンバー変数 | `m_fireHoldTimer` | 長押しの魔法を押し続けるため | 発射ボタンを押し続ける残り秒数 | この手順で作る |
| メンバー変数 | `m_hasTarget` | 相手を見つけた瞬間がわかるようにするため | 前のフレームで相手を見つけていたか | この手順で作る |
| メンバー変数 | `m_isFiring` | 押している最中かを覚えるため | 発射ボタンを押している最中か | この手順で作る |


### 1-6. 使い方のコメントを書く

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **4行目**（`#include "CharacterInput.h"` の下の空の行）の下、`class NpcInputSource : public IInputSource` の上に、2行足します（5〜6行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 5 -->
<table class="code-lines"><tr><td>

```cpp
5
6
```

</td><td>

```cpp
// NPCのAI。敵対する相手を探して近づき、射程に入ったら魔法を撃つ入力を作る。
// AIは「どうしたいか」だけを決め、実際の移動や発射はキャラクター側が行う。
```

</td></tr></table>

- **なぜ:** このクラスが何をして、何をしないか（実際に動かすのはキャラクター側）を、ヘッダーを開いた人がすぐわかるようにする。
- **効果:** AI にキャラクターを直接動かす処理を足してしまう間違いを、防ぎやすくなる。

### 1-7. NpcInputSource.cpp を作り、コンストラクターを書く

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) を新しく作り、次の8行を書きます（1〜8行目）。

<!-- new: Action/Game/Source/Character/Input/NpcInputSource.cpp -->
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
#include "NpcInputSource.h"
#include "HitTarget.h"

NpcInputSource::NpcInputSource(const Settings& settings)
	: m_settings(settings)
{
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 3 | 相手を探すのに `HitTargetRegistry` を使う |
| 6 | 設定を覚える |

- **なぜ:** 渡された設定を `m_settings` に覚えさせる。コンストラクターの初期化子リストに書くと、作る時点で値が入る。
- **効果:** `NpcInputSource(settings)` と書くだけで、設定済みの AI が作れるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 引数 | `settings` | 作るときに設定を受け取るため | AI の設定。`const &` なので、コピーせずに読むだけ | 使う側が決める（Enemy が渡す。手順2） |
| C++ の書き方 | `: m_settings(settings)` | 作る時点で、メンバー変数に値を入れるため | メンバー初期化子リスト。`m_settings` を `settings` のコピーで作る | C++ の書き方 |

### 1-8. 毎フレームの関数を書き、入力を「何もしない」にする

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **8行目** `}` の下（ファイルの最後）に、11行足します（9〜19行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 9 -->
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

void NpcInputSource::Update(const InputSourceContext& context, CharacterInput& input)
{
	input.moveDirection = Vector3::Zero;
	input.lookDirection = Vector3::Zero;
	input.isRun = false;
	input.jump.Update(false);
	input.dash.Update(false);
	input.hasAim = false;

}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 12〜17 | まず全部「何もしない」にする。このあと、要るものだけ入れる |
- **なぜ:** `input` には前のフレームの値が残っている。先に全部「何もしない」に戻しておけば、前のフレームの入力が残って動き続けることがない。
- **効果:** このあと、要る入力だけを入れればよくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 引数 | `context` | AI が自分の位置などを知るため | `InputSourceContext`。自分の位置・陣営・1フレームの時間などを読む | 使う側が決める（キャラクターが毎フレーム渡す。もとの型は [入力の手順](../入力/入力の手順.md) で作った） |
| 引数 | `input` | 作った入力を、キャラクターに返すため | `CharacterInput`。`&`（参照）なので、書き込むとそのまま呼んだ側に届く | 使う側が決める（キャラクターが渡す） |
| メンバー変数 | `moveDirection` | 動きたい向きを伝えるため | 移動したい向き（水平）。長さ 0〜1 が速さの割合 | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `lookDirection` | 向きたい向きを伝えるため | 向きたい向き。ゼロなら、動く向きを向く | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `isRun` | 走るかを伝えるため | `true` なら走る | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `jump` | ジャンプのボタンを伝えるため | ジャンプのボタンの状態（`ButtonState`） | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `dash` | ダッシュのボタンを伝えるため | ダッシュのボタンの状態（`ButtonState`） | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| 関数 | `Update`（`jump.Update(false)`） | ボタンの押した瞬間・離した瞬間を求めるため | 「今押しているか」を渡すと、`isTrigger`・`isPress`・`isRelease` を更新する。`false` は「押していない」 | [入力の手順](../入力/入力の手順.md) で作った（`ButtonState`） |
| メンバー変数 | `hasAim` | 狙う向きを使うかを伝えるため | `true` なら、`aimOrigin` から `aimDirection` へ撃つ | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |


### 1-9. 一番近い敵を探す

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **18行目**（`input.hasAim = false;` の下の空の行）の下、`}` の上に、9行足します（19〜27行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 19 -->
<table class="code-lines"><tr><td>

```cpp
19
20
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
	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);

	if (target == nullptr) {
		m_hasTarget = false;
		m_isFiring = false;
		input.Action(ActionButton::Action1).Update(false);
		return;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 19〜20 | 自分の位置から、探す範囲の中で一番近い、敵対する相手を探す。自分は除く |
| 22〜27 | 見つからなければ、撃つのをやめて終わる |
- **なぜ:** 敵対する相手の一覧は `HitTargetRegistry` が持っている。そこに「一番近い敵対する相手」を探させれば、AI は一覧の中身や陣営の決まりを知らずに済む。
- **効果:** 近くに敵がいるかと、どの相手を狙うかが、この数行でわかるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `HitTarget` | 魔法が当たる相手を表すため | 当たり判定。位置・陣営・持ち主を持つ | [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った（[HitTarget.h](../../../../Game/Source/Character/Components/HitTarget.h)） |
| 変数 | `target` | 狙う相手を持つため | 見つけた相手の `HitTarget`。見つからなければ `nullptr` | この手順で作る |
| 型 | `HitTargetRegistry` | 当たり判定を1か所で探せるようにするため | 登録された `HitTarget` の一覧を持ち、相手を探す関数を持つ | [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った（[HitTarget.h](../../../../Game/Source/Character/Components/HitTarget.h)） |
| 関数 | `FindNearestHostile` | 一番近い敵を見つけるため | 位置・距離・陣営・除く相手を渡すと、その距離以内で一番近い敵対する `HitTarget` を返す。いなければ `nullptr` | [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った（[HitTarget.h](../../../../Game/Source/Character/Components/HitTarget.h)） |
| メンバー変数 | `position` | 自分の位置から探すため | 自分の足元の位置 | [入力の手順](../入力/入力の手順.md) で作った（`InputSourceContext`） |
| メンバー変数 | `faction` | 敵か味方かを決めるため | 自分の陣営。これと敵対する陣営が相手になる | [入力の手順](../入力/入力の手順.md) で作った（`InputSourceContext`） |
| メンバー変数 | `self` | 自分を狙わないため | 操作している自分自身。探す相手から除く | [入力の手順](../入力/入力の手順.md) で作った（`InputSourceContext`） |
| 関数 | `Action` | 撃つボタン（`Action1`）の状態を書き込むため | そのボタンの `ButtonState` を返す。`Update(false)` で離した状態になる | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| 値 | `ActionButton::Action1` | どのボタンかを指すため | 1つ目のスキルのボタン。敵は氷魔法を登録する | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput.h`） |


### 1-10. 見つけた直後は、少し待つ

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **27行目** `}` の下、`}` の上に、6行足します（28〜33行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 28 -->
<table class="code-lines"><tr><td>

```cpp
28
29
30
31
32
33
```

</td><td>

```cpp
	if (!m_hasTarget) {
		// 見つけた直後にいきなり撃たないよう、少し待つ。
		m_fireTimer = m_settings.firstFireDelay;
		m_hasTarget = true;
	}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 28〜32 | 前のフレームで見つけていなかったら、最初に撃つまでの時間を入れる。見つけた直後にいきなり撃たないように |
- **なぜ:** 相手を見つけた瞬間に撃つと、不意打ちで避けようがない。最初の1回だけ待ち時間を入れる。「前のフレームで見つけていたか」を `m_hasTarget` で見れば、見つけた瞬間を1回だけ見分けられる。
- **効果:** 見つけてから `firstFireDelay` 秒は撃たなくなる。


### 1-11. 相手までの向きと距離を求める

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **33行目**（`}` の下の空の行）の下、`}` の上に、4行足します（34〜37行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 34 -->
<table class="code-lines"><tr><td>

```cpp
34
35
36
37
```

</td><td>

```cpp
	// 高低差を無視して水平方向に追う。
	Vector3 toTarget = target->GetCenter() - context.position;
	toTarget.y = 0.0f;
	const float distanceSq = toTarget.LengthSq();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 35〜36 | 相手の中心までの向き。高さの差は無視して、水平に追う |
| 37 | 相手までの距離（の2乗） |

- **なぜ:** 高さの差まで入れて追うと、相手が高い所にいるときに上を向いてしまう。`y` を 0 にして、水平だけで追う。距離は、平方根を使わずに済む2乗のまま持つ（計算が軽い）。
- **効果:** 相手が高い所や低い所にいても、水平に近づける。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `toTarget` | 相手のいる向きを求めるため | 自分から相手の中心へ向かうベクトル（高さは 0） | この手順で作る |
| 関数 | `GetCenter` | 相手の中心を知るため | 当たり判定の中心の位置を返す | [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った（[HitTarget.h](../../../../Game/Source/Character/Components/HitTarget.h) の `HitTarget`） |
| 関数 | `LengthSq` | 距離を軽く比べるため | ベクトルの長さの2乗を返す。平方根を使わない | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h)（`Vector3`） |
| 変数 | `distanceSq` | 近づく・撃つの判定に使うため | 相手までの距離の2乗 | この手順で作る |

### 1-12. 相手の方を向いて近づく

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **37行目** `const float distanceSq = toTarget.LengthSq();` の下、`}` の上に、9行足します（38〜46行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 38 -->
<table class="code-lines"><tr><td>

```cpp
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
	if (distanceSq > FLT_EPSILON) {
		Vector3 direction = toTarget;
		direction.Normalize();
		input.lookDirection = direction;
		if (distanceSq > m_settings.stopDistance * m_settings.stopDistance) {
			input.moveDirection = direction * m_settings.moveRate;
		}
	}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 39〜41 | 相手の方を向く |
| 42〜44 | 止まる距離より遠ければ、相手の方へ動く |

- **なぜ:** 長さが 0 に近いベクトルは、向きを決められない（`Normalize` で壊れる）。0 に近いときは何もしない。止まる距離は、距離も2乗にそろえて比べる。
- **効果:** 相手の真上や真下にいても壊れず、止まる距離までは近づき、そこで止まる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 値 | `FLT_EPSILON` | 「ほぼ 0」を見分けるため | `float` で、1 に足すと 1 と区別できる最も小さい値（約 0.0000001）。これより大きければ、0 ではないとみなす | C++ の標準ライブラリ（`<cfloat>`。`stdafx.h` から読み込み済みのヘッダー経由） |
| 変数 | `direction` | 相手へ向かう向きを持つため | `toTarget` を長さ 1 にした向き | この手順で作る |
| 関数 | `Normalize` | 向きだけを取り出すため | ベクトルの長さを 1 にする | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h)（`Vector3`） |

### 1-13. 撃つための時間を進める

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **46行目**（`}` の下の空の行）の下、`}` の上に、11行足します（47〜57行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 47 -->
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
	// 撃つかどうかを決める。
	const bool isInFireRange = distanceSq <= m_settings.fireRange * m_settings.fireRange;
	if (m_fireTimer > 0.0f) {
		m_fireTimer -= context.deltaTime;
	}
	if (m_isFiring) {
		m_fireHoldTimer -= context.deltaTime;
		if (m_fireHoldTimer <= 0.0f) {
			m_isFiring = false;
		}
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 48 | 撃てる距離にいるか |
| 49〜51 | 次に撃てるまでの時間を減らす |
| 52〜57 | 押し続けている最中なら、押す時間を減らし、なくなったら離す |
- **なぜ:** 撃つ時間は、ゲームの1回の更新ごとに `deltaTime` ずつ減らす。フレームの数ではなく秒で数えるので、速さの違うPCでも撃つ間隔が変わらない。
- **効果:** 撃てる距離にいるかと、次に撃てるまでの残り時間が、毎フレーム正しく更新される。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `isInFireRange` | 撃てる距離かを持つため | 相手までの距離が `fireRange` 以内か | この手順で作る |
| メンバー変数 | `deltaTime` | 時間を秒で進めるため | 前のフレームから今のフレームまでの秒数 | [入力の手順](../入力/入力の手順.md) で作った（`InputSourceContext`） |


### 1-14. 撃ち始める

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **57行目** `}` の下、`}` の上に、7行足します（58〜64行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 58 -->
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
	else if (m_settings.canFire && isInFireRange && m_fireTimer <= 0.0f) {
		m_isFiring = true;
		m_fireHoldTimer = m_settings.fireHoldSeconds;
		m_fireTimer = m_settings.fireInterval;
	}
	input.Action(ActionButton::Action1).Update(m_isFiring);

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 58〜62 | 撃ってよくて、撃てる距離にいて、時間が来ていれば、撃ち始める |
| 62 | 撃つボタンを押しているかを入れる |
- **なぜ:** 撃ってよい条件（撃つ設定・射程・時間）が全部そろったときだけ、撃ち始める。押している最中は、新しく撃ち始めない（`else if`）。
- **効果:** 設定どおりの間隔で、射程に入っているときだけ撃つようになる。


### 1-15. 相手の胸のあたりを狙う

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **64行目**（`input.Action(ActionButton::Action1).Update(m_isFiring);` の下の空の行）の下、`}` の上に、5行足します（65〜69行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 65 -->
<table class="code-lines"><tr><td>

```cpp
65
66
67
68
69
```

</td><td>

```cpp
	// 相手の胸のあたりを狙う。
	input.hasAim = true;
	input.aimOrigin = context.position;
	input.aimOrigin.y += m_settings.castHeight;
	input.aimDirection = target->GetCenter() - input.aimOrigin;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 66 | 照準を使う |
| 67〜68 | 足元から `castHeight` だけ上から撃つ |
| 69 | 相手の当たり判定の中心へ向けて撃つ |

- **なぜ:** 足元から相手の中心へ撃つと、地面に向けて撃つことになる。胸のあたりの高さから撃たせる。
- **効果:** 魔法が、相手の当たり判定に向かって飛ぶようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `aimOrigin` | 撃つ位置を伝えるため | 撃ち始める位置 | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `aimDirection` | 撃つ向きを伝えるため | 撃つ向き（長さは 1 でなくてよい） | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |

### 出来上がった NpcInputSource.h

<!-- full: Action/Game/Source/Character/Input/NpcInputSource.h -->
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
```

</td><td>

```cpp
#pragma once

#include "CharacterInput.h"

// NPCのAI。敵対する相手を探して近づき、射程に入ったら魔法を撃つ入力を作る。
// AIは「どうしたいか」だけを決め、実際の移動や発射はキャラクター側が行う。
class NpcInputSource : public IInputSource
{
public:
	struct Settings
	{
		float searchRange = 900.0f;    // この距離以内の敵対する相手を狙う。
		float stopDistance = 90.0f;    // この距離まで近づいたら止まる。
		float moveRate = 1.0f;         // 移動入力の大きさ（0〜1）。
		bool canFire = true;           // 魔法を撃つか。
		float fireRange = 700.0f;      // この距離以内なら撃つ。
		float fireInterval = 4.0f;     // 撃つ間隔（秒）。
		float firstFireDelay = 1.5f;   // 相手を見つけてから最初に撃つまでの秒数。
		float fireHoldSeconds = 0.0f;  // 発射ボタンを押し続ける秒数。長押しの魔法用。0なら1フレームだけ押す。
		float castHeight = 60.0f;      // 足元からこの高さで撃つ。
	};

	NpcInputSource() = default;
	explicit NpcInputSource(const Settings& settings);
	void SetSettings(const Settings& settings)
	{
		m_settings = settings;
	}
	void Update(const InputSourceContext& context, CharacterInput& input) override;

private:
	Settings m_settings;
	float m_fireTimer = 0.0f;      // 次に撃てるまでの秒数。
	float m_fireHoldTimer = 0.0f;  // 発射ボタンを押し続ける残り秒数。
	bool m_hasTarget = false;      // 前フレームに相手を見つけていたか。
	bool m_isFiring = false;       // 発射ボタンを押している最中か。
};
```

</td></tr></table>

### 出来上がった NpcInputSource.cpp

<!-- full: Action/Game/Source/Character/Input/NpcInputSource.cpp -->
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
```

</td><td>

```cpp
#include "stdafx.h"
#include "NpcInputSource.h"
#include "HitTarget.h"

NpcInputSource::NpcInputSource(const Settings& settings)
	: m_settings(settings)
{
}

void NpcInputSource::Update(const InputSourceContext& context, CharacterInput& input)
{
	input.moveDirection = Vector3::Zero;
	input.lookDirection = Vector3::Zero;
	input.isRun = false;
	input.jump.Update(false);
	input.dash.Update(false);
	input.hasAim = false;

	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);

	if (target == nullptr) {
		m_hasTarget = false;
		m_isFiring = false;
		input.Action(ActionButton::Action1).Update(false);
		return;
	}
	if (!m_hasTarget) {
		// 見つけた直後にいきなり撃たないよう、少し待つ。
		m_fireTimer = m_settings.firstFireDelay;
		m_hasTarget = true;
	}

	// 高低差を無視して水平方向に追う。
	Vector3 toTarget = target->GetCenter() - context.position;
	toTarget.y = 0.0f;
	const float distanceSq = toTarget.LengthSq();
	if (distanceSq > FLT_EPSILON) {
		Vector3 direction = toTarget;
		direction.Normalize();
		input.lookDirection = direction;
		if (distanceSq > m_settings.stopDistance * m_settings.stopDistance) {
			input.moveDirection = direction * m_settings.moveRate;
		}
	}

	// 撃つかどうかを決める。
	const bool isInFireRange = distanceSq <= m_settings.fireRange * m_settings.fireRange;
	if (m_fireTimer > 0.0f) {
		m_fireTimer -= context.deltaTime;
	}
	if (m_isFiring) {
		m_fireHoldTimer -= context.deltaTime;
		if (m_fireHoldTimer <= 0.0f) {
			m_isFiring = false;
		}
	}
	else if (m_settings.canFire && isInFireRange && m_fireTimer <= 0.0f) {
		m_isFiring = true;
		m_fireHoldTimer = m_settings.fireHoldSeconds;
		m_fireTimer = m_settings.fireInterval;
	}
	input.Action(ActionButton::Action1).Update(m_isFiring);

	// 相手の胸のあたりを狙う。
	input.hasAim = true;
	input.aimOrigin = context.position;
	input.aimOrigin.y += m_settings.castHeight;
	input.aimDirection = target->GetCenter() - input.aimOrigin;
}
```

</td></tr></table>

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: Enemy の入力元を AI にする

[入力の手順](../入力/入力の手順.md) で Enemy に置いた「何も入力しない入力元」を消し、AI に替えます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Enemy.h](../../../../Game/Source/Enemy/Enemy.h) | 削除（何も入力しない入力元）・追加（`#include`、コメント）・書き換え（入力元を AI に） |
| [Enemy.cpp](../../../../Game/Source/Enemy/Enemy.cpp) | 追加（AI の設定を作る関数、コンストラクターで AI に設定を渡す） |

### 2-1. 何も入力しない入力元を消す

[Enemy.h](../../../../Game/Source/Enemy/Enemy.h) の **5〜13行目**を消します。

<!-- del: Action/Game/Source/Enemy/Enemy.h 5 -->
<table class="code-lines"><tr><td>

```cpp
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
// NPCのAIができるまでの、何も入力しない入力元。NPCのAIの手順で消す。
class EmptyInputSource : public IInputSource
{
public:
	void Update(const InputSourceContext& context, CharacterInput& input) override
	{
	}
};

```

</td></tr></table>
- **なぜ:** AI ができたので、仮に置いていた「何も入力しない入力元」は要らない。残すと、使われないクラスが残る。
- **効果:** Enemy の入力元が、AI だけになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `EmptyInputSource` | AI ができるまで、Enemy が何もしないようにするため | `Update` が空の入力元。入力を何も入れない | [入力の手順](../入力/入力の手順.md) で作った（`Enemy.h`）。この手順で消す |


### 2-2. NpcInputSource.h を読み込む

[Enemy.h](../../../../Game/Source/Enemy/Enemy.h) の **3行目** `#include "Character.h"` の下に、1行足します（4〜4行目になります）。

<!-- add: Action/Game/Source/Enemy/Enemy.h 4 -->
<table class="code-lines"><tr><td>

```cpp
4
```

</td><td>

```cpp
#include "NpcInputSource.h"
```

</td></tr></table>

- **なぜ:** Enemy が `NpcInputSource` を使えるように、ヘッダーを読み込む。
- **効果:** `Enemy.h` で `NpcInputSource` が書けるようになる。

### 2-3. 返す入力元を AI にする

[Enemy.h](../../../../Game/Source/Enemy/Enemy.h) の **26行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/Game/Source/Enemy/Enemy.h 26 -->
<table class="code-lines"><tr><td>

```cpp
26
```

</td><td>

```cpp
		return m_emptyInputSource;
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/Game/Source/Enemy/Enemy.h 26 -->
<table class="code-lines"><tr><td>

```cpp
26
```

</td><td>

```cpp
		return m_npcInputSource;
```

</td></tr></table>
- **なぜ:** キャラクターは、決まった関数から入力元を受け取る。返す入力元を AI にすれば、Enemy は AI の入力で動く。
- **効果:** Enemy が、何も入力しない状態ではなく、AI の入力で動くようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_emptyInputSource` | AI ができるまでの入力元を持つため | 何も入力しない入力元。ここで使うのをやめる | [入力の手順](../入力/入力の手順.md) で作った（`Enemy.h`） |
| メンバー変数 | `m_npcInputSource` | Enemy が AI を持つため | Enemy の AI（`NpcInputSource`） | この手順で作る |


### 2-4. 持つ入力元を AI にする

[Enemy.h](../../../../Game/Source/Enemy/Enemy.h) の **31行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/Game/Source/Enemy/Enemy.h 31 -->
<table class="code-lines"><tr><td>

```cpp
31
```

</td><td>

```cpp
	EmptyInputSource m_emptyInputSource;
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/Game/Source/Enemy/Enemy.h 31 -->
<table class="code-lines"><tr><td>

```cpp
31
```

</td><td>

```cpp
	NpcInputSource m_npcInputSource;
```

</td></tr></table>
- **なぜ:** 2-3 で返す入力元が `m_npcInputSource` になったので、持つ変数も AI に変える。
- **効果:** Enemy が作られると、AI も一緒に作られる。


### 2-5. Enemy のコメントを書き足す

[Enemy.h](../../../../Game/Source/Enemy/Enemy.h) の **6行目** `// 敵キャラクター。` の下に、1行足します（7〜7行目になります）。

<!-- add: Action/Game/Source/Enemy/Enemy.h 7 -->
<table class="code-lines"><tr><td>

```cpp
7
```

</td><td>

```cpp
// 入力元（既定ではAI）から CharacterInput を受け取って動くだけで、何をするかはAIが決める。
```

</td></tr></table>

- **なぜ:** Enemy が入力元（AI）の入力で動くだけで、何をするかは AI が決める、という作りをコメントに残す。
- **効果:** あとで `Enemy.h` を読む人が、動きを決めているのが AI だとすぐわかる。

### 2-6. AI の設定を作る関数を書く

[Enemy.cpp](../../../../Game/Source/Enemy/Enemy.cpp) の **7行目** `constexpr float HIT_CENTER_HEIGHT = 60.0f;  // 足元から当たり判定の中心までの高さ。` の下に、9行足します（8〜16行目になります）。

<!-- add: Action/Game/Source/Enemy/Enemy.cpp 8 -->
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
16
```

</td><td>

```cpp

	NpcInputSource::Settings MakeAiSettings()
	{
		NpcInputSource::Settings settings;
		// 今は敵を止めておく（相手を探さないので、動かず・向きも変えず・撃たない）。動かすときは 900 に戻す。
		settings.searchRange = 0.0f;     // この距離以内に入ると追跡を開始する。
		settings.stopDistance = 90.0f;   // この距離まで近づいたら停止する（近づきすぎ防止）。
		return settings;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13 | 探す範囲を 0 にして、敵を止めておく。動かすときは 900 に戻す |
| 14 | この距離まで近づいたら止まる |
- **なぜ:** AI の設定は、Enemy の側で決める。関数にまとめておくと、コンストラクターが短くなる。今は探す範囲を 0 にして、敵を止めておく。
- **効果:** 設定を直す場所が、この関数の1か所になる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `MakeAiSettings` | Enemy 用の AI の設定を作るため | 設定を作って返す。このファイルの中だけで使う | この手順で作る（`Enemy.cpp`） |
| 変数 | `settings` | 設定を組み立てるため | 返す `NpcInputSource::Settings`。値を変えてから返す | この手順で作る |


### 2-7. 撃つための設定を足す

[Enemy.cpp](../../../../Game/Source/Enemy/Enemy.cpp) の **14行目** `settings.stopDistance = 90.0f;   // この距離まで近づいたら停止する（近づきすぎ防止）。` の下に、5行足します（15〜19行目になります）。

<!-- add: Action/Game/Source/Enemy/Enemy.cpp 15 -->
<table class="code-lines"><tr><td>

```cpp
15
16
17
18
19
```

</td><td>

```cpp
		settings.canFire = true;
		settings.fireRange = 700.0f;
		settings.fireInterval = 4.0f;    // 凍結(2.5秒)より長くして、凍らされ続けないようにする。
		settings.firstFireDelay = 1.5f;
		settings.castHeight = 60.0f;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 17 | 撃つ間隔は 4 秒。凍結（2.5 秒）より長くして、凍らされ続けないようにする |
- **なぜ:** 撃つ値も、Enemy の側で決める。撃つ間隔を凍結の長さより長くしておかないと、凍らされている間にまた撃たれてしまう。
- **効果:** Enemy が、探した相手に、決めた間隔で撃つようになる。


### 2-8. コンストラクターで AI に設定を渡す

[Enemy.cpp](../../../../Game/Source/Enemy/Enemy.cpp) の **25行目** `: Character(Faction::Enemy)` の下に、1行足します（26〜26行目になります）。

<!-- add: Action/Game/Source/Enemy/Enemy.cpp 26 -->
<table class="code-lines"><tr><td>

```cpp
26
```

</td><td>

```cpp
	, m_npcInputSource(MakeAiSettings())
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 26 | 2-6・2-7 の設定で AI を作る |
- **なぜ:** Enemy を作るときに、設定を持った AI を作る。初期化子リストなので、AI は作る時点で設定が入っている。
- **効果:** Enemy が作られたときから、2-6・2-7 の設定で AI が動く。


**確かめ方（これで出来上がり）:** ソリューションのビルドが通る（探す範囲が 0 なので、Enemy はまだ動かない）。
