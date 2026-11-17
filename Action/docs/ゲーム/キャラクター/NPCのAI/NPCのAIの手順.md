# NPCのAIの手順

設計: [NPCのAIの設計](NPCのAIの設計.md)
経緯: [NPCのAIの経緯](NPCのAIの経緯.md)

NPC の AI（`NpcInputSource`）を、何もない状態から作る手順です。AI は「どうしたいか」だけを決めて `CharacterInput` に入れ、実際の移動や攻撃・魔法はキャラクター側が行います。

**出来上がるもの:** 敵対する相手を見つけたら、水平に相手を向いて近づき、相手との距離を「近い・中くらい・遠い」の3段に分けて、段ごとに決めたボタン（`Action1`〜`Action3`）を押す入力元です。
中くらい・遠いときは、決め直す間隔ごとに、決めた確率で走ってから、近くまで来て押します。溜める魔法のために、ボタンを押し続ける秒数も段ごとに決められます。
**今は、この AI を使っているキャラクターはありません**（黒騎士は専用の `BlackKnightInputSource`）。使うキャラクターを作るのは、この手順の外です。そのため、確かめるのはビルドが通ることだけです。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [入力の手順](../入力/入力の手順.md) を済ませておく（順番は [README の「ゲームを作る順番」](../../../README.md#ゲームを作る順番)）。ボタン（`ActionButton`）と入力元（`IInputSource`）はそこで作る。相手は [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った `HitTargetRegistry` から探す。
- 押したボタンで何が起きるかは、使うキャラクターが `SkillController` に登録したスキルで決まる（[スキルの設計](../../スキル/スキルの設計.md)）。この手順では、まだ登録しない。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | NPC の AI `NpcInputSource` の形と設定、相手を探して近づき、狙う | ビルドが通る |
| 2 | 距離の3段と、段ごとのボタン | ビルドが通る |
| 3 | 押すボタンを決める（間隔・走る・押し続ける） | ビルドが通る |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる変数・引数・関数・型・マクロ・C++ の書き方を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、このゲームで作ったもの・エンジン（k2Engine）の機能・C++ の標準ライブラリ・C++ の書き方のどれかがわかります。
- [前提知識](../../../前提知識.md) で「よく知っている」にあるもの（クラス・継承・`virtual`・`override`・ポインター・`nullptr`・`const`・`enum class`・`static_cast`・`= default` など）は、表に載せません。「まあまあ」「怪しい」「微妙」にあるものは載せます。
- この手順は足すだけで、前の小手順のコードを書き換えません。そのため、途中のコードには、まだ使っていない設定や、値を返さない道がある関数が出てきます。ビルドは各手順の最後に確かめます。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) | 新しく作る | 1、2、3 |
| [NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) | 新しく作る | 1、2、3 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: NpcInputSource の形と設定を作り、相手を探して近づき、狙う

AI も、プレイヤーの操作と同じく入力元（`IInputSource`）として作ります。この手順では、相手を見つけて、そちらを向いて近づき、狙う向きを決めるところまで作ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) | 新しく作る（クラスの宣言、設定 `Settings`、設定を持つメンバー） |
| [NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) | 新しく作る（入力を空にし、相手を探して近づき、狙う） |

`Source\Character\Input` フィルターに `NpcInputSource.h` と `NpcInputSource.cpp` を作ります。

### 1-1. NpcInputSource クラスの形を書く

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) を新しく作り、次の9行を書きます（1〜9行目）。

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
```

</td><td>

```cpp
#pragma once

#include "CharacterInput.h"

class NpcInputSource : public IInputSource
{
public:
	void Update(const InputSourceContext& context, CharacterInput& input) override;
};
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 3 | `IInputSource` と `CharacterInput` を使うために読み込む |
| 5 | `IInputSource`（入力元）を継承する |
| 8 | 毎フレーム呼ばれて、入力を作る関数（1-2 で書く） |

- **なぜ:** AI も入力元（`IInputSource`）にしておくと、プレイヤーの操作と同じ形でキャラクターに入力を渡せる。キャラクター側を、AI 用に作り変えなくてよい。
- **効果:** キャラクターが持つ入力元を差し替えるだけで、プレイヤーの操作と AI を入れ替えられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `NpcInputSource` | 敵の動きを決めるため | NPC の AI の入力元。相手を探して近づき、距離に合わせてボタンを押す入力を作る | この手順で作る |
| 型 | `IInputSource` | プレイヤーの操作も AI も、同じ形でキャラクターに入力を渡すため | 毎フレーム入力を作る `Update` を持つ基底クラス | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h)） |
| 関数 | `Update` | 毎フレーム、入力を作るため | `CharacterInput` に値を書き込む。`IInputSource` の `Update` を上書きする | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h)） |
| 型 | `InputSourceContext` | AI が、操作する自分のことを知るため | 自分の位置・陣営・自分自身・1フレームの時間など、入力元が知ってよい情報 | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h)） |
| 型 | `CharacterInput` | キャラクターが受け取る1フレーム分の入力をまとめるため | 移動の向き・ボタン・照準などを持つ構造体。AI はこれに書き込む | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h)） |


### 1-2. NpcInputSource.cpp を作り、空の Update を書く

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) を新しく作り、次の6行を書きます（1〜6行目）。

<!-- new: Action/Game/Source/Character/Input/NpcInputSource.cpp -->
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
#include "NpcInputSource.h"

void NpcInputSource::Update(const InputSourceContext& context, CharacterInput& input)
{
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 1 | `stdafx.h` を読み込む（Visual Studio のプロジェクトの決まり） |
| 2 | `NpcInputSource.h` を読み込む |
| 4 | 1-1 で宣言した `Update` の中身。今は空 |

- **なぜ:** ヘッダーで宣言した関数は、`.cpp` で定義しないとリンクできない。先に空で定義しておき、1-7 から中身を足していく。
- **効果:** `NpcInputSource` がビルドできる形になる（何も入力しない入力元）。


### 1-3. AI の設定 Settings を書く（探す・近づく・狙う）

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **7行目** `public:` の下に、8行足します（8〜15行目になります）。

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
15
```

</td><td>

```cpp
	struct Settings
	{
		float searchRange = 900.0f;      // この距離以内の敵対する相手を追う（ここまでが「遠い」）。
		float stopDistance = 150.0f;     // この距離まで近づいたら止まる。
		float moveRate = 1.0f;           // 移動入力の大きさ（0〜1）。
		float castHeight = 60.0f;        // 足元からこの高さから狙う（魔法を撃つ高さ）。
	};

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 10 | 相手を探す範囲。「ここまでが遠い」というコメントは、手順2 で3段に分けたときの意味 |
| 11 | 相手に近づいて止まる距離 |
| 12 | 移動入力の大きさ |
| 13 | 狙う高さ（足元から） |

- **なぜ:** 探す範囲・止まる距離・歩く速さ・狙う高さは、キャラクターごとに変えたい。値を `Settings` にまとめておけば、AI のコードを書き換えずに変えられる。
- **効果:** 使うキャラクターが、`Settings` の値だけで AI の動きを変えられるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Settings` | AI の設定（距離・時間・ボタン）を1つにまとめるため | 探す範囲や止まる距離などの値を持つ構造体。手順2・3 でも足す | この手順で作る |
| メンバー変数 | `searchRange` | 狙う相手を絞るため | この距離以内にいる、敵対する相手だけを追う（既定 900） | この手順で作る（`Settings`） |
| メンバー変数 | `stopDistance` | 相手に近づきすぎないため | この距離まで近づいたら止まる（既定 150。黒騎士の剣が届く距離） | この手順で作る（`Settings`） |
| メンバー変数 | `moveRate` | 歩く速さを決めるため | 移動の入力の大きさ（0〜1）。1 なら全力で動く | この手順で作る（`Settings`） |
| メンバー変数 | `castHeight` | 足元ではなく胸のあたりを狙うため | 足元からこの高さを、狙う向きの出発点にする（既定 60） | この手順で作る（`Settings`） |


### 1-4. コンストラクターと、設定を変える関数を書く

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **15行目**（空の行）の下に、6行足します（16〜21行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 16 -->
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
	NpcInputSource() = default;
	explicit NpcInputSource(const Settings& settings);
	void SetSettings(const Settings& settings)
	{
		m_settings = settings;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 16 | 設定を渡さずに作るときのコンストラクター |
| 17 | 設定（1-3）を渡して作るコンストラクター |
| 18〜21 | 設定をあとから入れ替える関数 |

- **なぜ:** 設定は、作るときに渡したいことも、作ったあとで変えたいこともある。どちらもできるようにしておく。
- **効果:** AI を作るときに設定を渡せる。作ったあとでも、設定をまとめて差し替えられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `explicit` | 思いがけない自動の変換を防ぐため | 引数が1つのコンストラクターに付ける。`Settings` から `NpcInputSource` へ、勝手に変わらないようにする | C++ の書き方 |
| 関数 | `SetSettings` | あとから AI の設定を変えるため | 受け取った設定を、`m_settings` に入れる | この手順で作る |
| メンバー変数 | `m_settings` | AI が今の設定を覚えておくため | 今使っている `Settings`。1-6 で宣言する | この手順で作る |


### 1-5. コンストラクターを書く

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **3行目**（空の行）の下に、5行足します（4〜8行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 4 -->
<table class="code-lines"><tr><td>

```cpp
4
5
6
7
8
```

</td><td>

```cpp
NpcInputSource::NpcInputSource(const Settings& settings)
	: m_settings(settings)
{
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 4 | `settings` を受け取って、`m_settings` に入れる |

- **なぜ:** コンストラクターを `.cpp` に定義する。作る時点で設定を入れておけば、あとから `SetSettings` を呼ばなくてよい。
- **効果:** `NpcInputSource(settings)` で、設定を持った AI を作れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 引数 | `settings` | 作るときに設定を受け取るため | AI の設定。`const &` なので、コピーせずに読むだけ | 使う側が決める（AI を使うキャラクターが渡す） |
| C++ の書き方 | `: m_settings(settings)` | 作る時点で、メンバー変数に値を入れるため | メンバー初期化子リスト。`m_settings` を `settings` のコピーで作る | C++ の書き方 |


### 1-6. 設定を持つメンバーを足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **22行目** `void Update(const InputSourceContext& context, CharacterInput& input) override;` の下に、3行足します（23〜25行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 23 -->
<table class="code-lines"><tr><td>

```cpp
23
24
25
```

</td><td>

```cpp

private:
	Settings m_settings;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 24 | 外から使わないものを置く場所 |
| 25 | 今の設定 |

- **なぜ:** 1-4 の `SetSettings` と 1-5 のコンストラクターが入れる先を宣言する。外から触らせないので `private` に置く。
- **効果:** AI が、今の設定を覚えておけるようになる。


### 1-7. Update の最初で、入力を「何もしない」にする

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **10行目** `{` の下に、7行足します（11〜17行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 11 -->
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
	input.moveDirection = Vector3::Zero;
	input.lookDirection = Vector3::Zero;
	input.isRun = false;
	input.jump.Update(false);
	input.dash.Update(false);
	input.hasAim = false;

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11〜12 | 移動したい向き・向きたい向きを 0 にする |
| 13 | 走るのをやめる |
| 14〜15 | ジャンプとダッシュを「押していない」にする |
| 16 | 狙う向きを使わない（相手を見つけたら 1-11 で入れる） |

- **なぜ:** 入力には前のフレームの値が残っている。先に全部「何もしない」に戻しておけば、相手がいないフレームに、前の動きが残らない。
- **効果:** 相手が見つからないフレームは、AI が何も入力しない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 引数 | `context` | AI が自分の位置などを知るため | `InputSourceContext`。自分の位置・陣営・1フレームの時間などを読む | 使う側が決める（キャラクターが毎フレーム渡す。もとの型は 入力の手順 で作った） |
| 引数 | `input` | 作った入力を、キャラクターに返すため | `CharacterInput`。`&`（参照）なので、書き込むとそのまま呼んだ側に届く | 使う側が決める（キャラクターが渡す） |
| メンバー変数 | `moveDirection` | 動きたい向きを伝えるため | 移動したい向き（水平）。長さ 0〜1 が速さの割合 | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `lookDirection` | 向きたい向きを伝えるため | 向きたい向き。ゼロなら、動く向きを向く | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `isRun` | 走るかを伝えるため | `true` なら走る | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `jump` | ジャンプのボタンを伝えるため | ジャンプのボタンの状態（`ButtonState`） | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `dash` | ダッシュのボタンを伝えるため | ダッシュのボタンの状態（`ButtonState`） | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| 関数 | `Update`（`jump.Update(false)`） | ボタンの押した瞬間・離した瞬間を求めるため | 「今押しているか」を渡すと、`isTrigger`・`isPress`・`isRelease` を更新する。`false` は「押していない」 | [入力の手順](../入力/入力の手順.md) で作った（`ButtonState`） |
| メンバー変数 | `hasAim` | 狙う向きを使うかを伝えるため | `true` なら、`aimOrigin` から `aimDirection` へ撃つ | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| 値 | `Vector3::Zero` | 「向きなし」を表すため | x・y・z が全部 0 のベクトル | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h)（`Vector3`） |


### 1-8. HitTarget.h を読み込む

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **2行目** `#include "NpcInputSource.h"` の下に、1行足します（3行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 3 -->
<table class="code-lines"><tr><td>

```cpp
3
```

</td><td>

```cpp
#include "HitTarget.h"
```

</td></tr></table>

- **なぜ:** 相手を探す関数（1-9）は `HitTarget.h` にあるので、読み込む。
- **効果:** `HitTargetRegistry` と `HitTarget` が `NpcInputSource.cpp` で使えるようになる。


### 1-9. 敵対する一番近い相手を探す

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **18行目**（空の行）の下に、5行足します（19〜23行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 19 -->
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
	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);
	if (target == nullptr) {
		return;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 19〜20 | 自分の位置から `searchRange` 以内の、敵対する一番近い相手を探す |
| 21〜23 | いなければ、ここで終わる（入力は 1-7 で「何もしない」になっている） |

- **なぜ:** 相手はクラス（`Player` など）でなく、陣営で探す。AI が `Player` を直接知らなくてよく、味方・中立の NPC にも同じ AI を使える。
- **効果:** 敵対する相手がいるときだけ、1-10 以降の処理に進む。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `HitTarget` | 魔法が当たる相手を表すため | 当たり判定。位置・陣営・持ち主を持つ | [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った（[HitTarget.h](../../../../Game/Source/Character/Components/HitTarget.h)） |
| 変数 | `target` | 狙う相手を持つため | 見つけた相手の `HitTarget`。見つからなければ `nullptr` | この手順で作る |
| 型 | `HitTargetRegistry` | 当たり判定を1か所で探せるようにするため | 登録された `HitTarget` の一覧を持ち、相手を探す関数を持つ | [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った（[HitTarget.h](../../../../Game/Source/Character/Components/HitTarget.h)） |
| 関数 | `FindNearestHostile` | 一番近い敵を見つけるため | 位置・距離・陣営・除く相手を渡すと、その距離以内で一番近い敵対する `HitTarget` を返す。いなければ `nullptr` | [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った（[HitTarget.h](../../../../Game/Source/Character/Components/HitTarget.h) の `HitTargetRegistry`） |
| メンバー変数 | `position` | 自分の位置から探すため | 自分の足元の位置 | [入力の手順](../入力/入力の手順.md) で作った（`InputSourceContext`） |
| メンバー変数 | `faction` | 敵か味方かを決めるため | 自分の陣営。これと敵対する陣営が相手になる | [入力の手順](../入力/入力の手順.md) で作った（`InputSourceContext`） |
| メンバー変数 | `self` | 自分を狙わないため | 操作している自分自身。探す相手から除く | [入力の手順](../入力/入力の手順.md) で作った（`InputSourceContext`） |


### 1-10. 相手の方を向いて、近づく

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **23行目** `}` の下に、13行足します（24〜36行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 24 -->
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
33
34
35
36
```

</td><td>

```cpp
	// 高低差を無視して、水平方向に相手を向いて近づく。
	Vector3 toTarget = target->GetCenter() - context.position;
	toTarget.y = 0.0f;
	const float distance = toTarget.Length();
	if (distance > FLT_EPSILON) {
		Vector3 direction = toTarget;
		direction.Normalize();
		input.lookDirection = direction;
		if (distance > m_settings.stopDistance) {
			input.moveDirection = direction * m_settings.moveRate;
		}
	}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 24 | コメント |
| 25〜26 | 相手の中心へ向かうベクトルを作る。高さ（y）は 0 にして、水平だけにする |
| 27 | 相手までの距離 |
| 28〜35 | 距離が 0 でなければ、向きを求めて `lookDirection` に入れる。`stopDistance` より遠ければ `moveDirection` にも入れて近づく |

- **なぜ:** 高低差を無視して水平に追う。段差の上の相手に、斜め上を向いて歩かないようにするため。
- **効果:** 相手の方を向き、止まる距離まで歩いて近づく入力ができる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `toTarget` | 相手のいる向きを求めるため | 自分から相手の中心へ向かうベクトル（高さは 0） | この手順で作る |
| 関数 | `GetCenter` | 相手の中心を知るため | 当たり判定の中心の位置を返す | [陣営と当たり判定の手順](../陣営と当たり判定/陣営と当たり判定の手順.md) で作った（[HitTarget.h](../../../../Game/Source/Character/Components/HitTarget.h) の `HitTarget`） |
| 変数 | `distance` | 近づく・段を分ける判定に使うため | 相手までの水平の距離 | この手順で作る |
| 値 | `FLT_EPSILON` | 「ほぼ 0」を見分けるため | `float` で、1 に足すと 1 と区別できる最も小さい値（約 0.0000001）。これより大きければ、0 ではないとみなす | C++ の標準ライブラリ（`<cfloat>`。`stdafx.h` から読み込み済みのヘッダー経由） |
| 変数 | `direction` | 相手へ向かう向きを持つため | `toTarget` を長さ 1 にした向き | この手順で作る |
| 関数 | `Normalize` | 向きだけを取り出すため | ベクトルの長さを 1 にする | k2EngineLow（エンジン）の [Vector.h](../../../../k2EngineLow/math/Vector.h)（`Vector3`） |


### 1-11. 相手の胸のあたりを狙う

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **36行目**（空の行）の下に、5行足します（37〜41行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 37 -->
<table class="code-lines"><tr><td>

```cpp
37
38
39
40
41
```

</td><td>

```cpp
	// 足元から castHeight の高さから、相手の胸のあたりを狙う。
	input.hasAim = true;
	input.aimOrigin = context.position;
	input.aimOrigin.y += m_settings.castHeight;
	input.aimDirection = target->GetCenter() - input.aimOrigin;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 37 | コメント |
| 38 | 狙う向きを使う |
| 39〜40 | 撃つ位置は、自分の足元から `castHeight` だけ上 |
| 41 | 狙う向きは、そこから相手の中心へ |

- **なぜ:** 足元から相手の中心へ向けると、魔法が地面に向かって飛ぶ。胸の高さから向けると、相手の胴に向かって飛ぶ。
- **効果:** 魔法を撃つスキルが、相手の方へ飛ぶ向きを受け取れるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `aimOrigin` | 撃つ位置を伝えるため | 撃ち始める位置 | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |
| メンバー変数 | `aimDirection` | 撃つ向きを伝えるため | 撃つ向き（長さは 1 でなくてよい） | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |


### 手順1の確かめ方

- ビルドが通る。
- この時点では、使うキャラクターも、押すボタンもまだありません。

---

## 手順2: 距離の3段と、段ごとのボタンを足す

相手との距離を「近い・中くらい・遠い」に分け、段ごとに押すボタンを持てるようにします。黒騎士の AI（[黒騎士のAIの設計](../../敵/黒騎士のAI/黒騎士のAIの設計.md)）と同じ分け方です。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) | 追加（`Settings` の距離とボタン、`Range`、`RangeAction`、関数の宣言） |
| [NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) | 追加（距離を3段に分ける関数、段のボタンを返す関数） |

### 2-1. 設定に、近い・中くらいの距離を足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **10行目** `float searchRange = 900.0f;      // この距離以内の敵対する相手を追う（ここまでが「遠い」）。` の下に、2行足します（11〜12行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 11 -->
<table class="code-lines"><tr><td>

```cpp
11
12
```

</td><td>

```cpp
		float nearRange = 220.0f;        // この距離以内が「近い」。
		float midRange = 600.0f;         // この距離以内が「中くらい」。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11 | 「近い」の距離 |
| 12 | 「中くらい」の距離。これより遠く、`searchRange` までが「遠い」 |

- **なぜ:** 3段の境目を、設定の値にする。魔法を撃つ敵は、近づかれる前に撃ちたいので、剣で戦う黒騎士とは別の値にできる。
- **効果:** 距離の分け方を、使うキャラクターごとに変えられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `nearRange` | 「近い」を決めるため | この距離以内が「近い」（既定 220。黒騎士の剣が届く距離） | この手順で作る（`Settings`） |
| メンバー変数 | `midRange` | 「中くらい」を決めるため | この距離以内が「中くらい」（既定 600） | この手順で作る（`Settings`） |


### 2-2. 距離の3段を表す Range を書く

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **17行目**（空の行）の下に、8行足します（18〜25行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 18 -->
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
	// 相手との距離の分け方。
	enum class Range
	{
		Near,
		Middle,
		Far,
	};

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 18 | コメント |
| 19 | `enum class` で型を作る |
| 21〜23 | 近い・中くらい・遠い |

- **なぜ:** 段を数字ではなく名前で書く。`enum class` なので、ほかの列挙型と取り違えない。
- **効果:** `if` や `switch` で、`Range::Near` のように読める。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Range` | 相手との距離の分け方を表すため | `Near`（近い）・`Middle`（中くらい）・`Far`（遠い）を持つ `enum class` | この手順で作る |


### 2-3. 距離から段を決める関数を宣言する

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **34行目** `private:` の下に、2行足します（35〜36行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 35 -->
<table class="code-lines"><tr><td>

```cpp
35
36
```

</td><td>

```cpp
	Range ClassifyRange(float distance) const;

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 35 | 距離から段を決める関数 |

- **なぜ:** 段を決める処理を関数にして、`Update` に直接書かない。`Update` が長くなりすぎないようにする。
- **効果:** 2-4 で中身を書く関数の宣言ができる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `ClassifyRange` | 距離から段を決めるため | 距離を渡すと、`Near`・`Middle`・`Far` のどれかを返す | この手順で作る |


### 2-4. 距離から段を決める関数を書く

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **9行目**（空の行）の下に、11行足します（10〜20行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 10 -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp
NpcInputSource::Range NpcInputSource::ClassifyRange(float distance) const
{
	if (distance <= m_settings.nearRange) {
		return Range::Near;
	}
	if (distance <= m_settings.midRange) {
		return Range::Middle;
	}
	return Range::Far;
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 10 | 戻り値の型の `Range` は、クラスの外で書くので `NpcInputSource::` を付ける |
| 12〜14 | `nearRange` 以内なら「近い」 |
| 15〜17 | `midRange` 以内なら「中くらい」 |
| 18 | 残りは「遠い」 |

- **なぜ:** 距離を3段に分ける処理を、`Update` から切り出して関数にする。
- **効果:** 距離を渡すだけで、段が決まる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `NpcInputSource::Range` | クラスの中で作った型を、外で書くため | `.cpp` の関数の戻り値の型では、クラスの名前を前に付ける | C++ の書き方 |


### 2-5. 段ごとのボタン RangeAction を書く

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **7行目** `public:` の下に、7行足します（8〜14行目になります）。

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
	// 1つの段で押すボタン。
	struct RangeAction
	{
		ActionButton button = ActionButton::Num;  // 押すボタン。Num ならこの段では何も押さない。
		float holdSeconds = 0.0f;                 // 押し続ける秒数。溜める魔法用。0なら1フレームだけ押す。
	};

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 8 | コメント |
| 9〜13 | 1つの段で押すボタンと、押し続ける秒数 |

- **なぜ:** 押すボタンは、スキルの種類（剣の攻撃・氷魔法・炎魔法）によらず、`ActionButton` で指す。溜める魔法（炎魔法）は押し続けてから離すので、秒数も持たせる。
- **効果:** 段ごとに「どのボタンを、何秒押すか」を決められる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `RangeAction` | 1つの段で押すボタンを表すため | 押すボタンと、押し続ける秒数をまとめた構造体 | この手順で作る |
| 型 | `ActionButton` | 攻撃や魔法のボタンを番号で区別するため | `Action1`・`Action2`・`Action3` と、数を表す `Num` を持つ `enum class` | [入力の手順](../入力/入力の手順.md) で作った（[CharacterInput.h](../../../../Game/Source/Character/Input/CharacterInput.h)） |
| メンバー変数 | `button` | 押すボタンを決めるため | 押すボタン。`ActionButton::Num` なら、この段では何も押さない | この手順で作る（`RangeAction`） |
| 値 | `ActionButton::Num` | 「ボタンなし」を表すため | ボタンの数を表す値。ボタンを選ばないときの印にも使う | [入力の手順](../入力/入力の手順.md) で作った（`ActionButton`） |
| メンバー変数 | `holdSeconds` | 溜める魔法を撃てるようにするため | 押し続ける秒数。0 なら1フレームだけ押す | この手順で作る（`RangeAction`） |


### 2-6. 設定に、段ごとのボタンを足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **22行目** `float castHeight = 60.0f;        // 足元からこの高さから狙う（魔法を撃つ高さ）。` の下に、3行足します（23〜25行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 23 -->
<table class="code-lines"><tr><td>

```cpp
23
24
25
```

</td><td>

```cpp
		RangeAction nearAction;          // 近いときに押すボタン。
		RangeAction midAction;           // 中くらいのときに押すボタン。
		RangeAction farAction;           // 遠いときに押すボタン。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 23 | 近いときに押すボタン |
| 24 | 中くらいのときに押すボタン |
| 25 | 遠いときに押すボタン |

- **なぜ:** 2-5 の `RangeAction` を、3段それぞれに持たせる。
- **効果:** 使うキャラクターが、段ごとに押すボタンを決められる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `nearAction`・`midAction`・`farAction` | 段ごとに押すボタンを決めるため | 近い・中くらい・遠いときの `RangeAction`。初めは `button` が `Num` なので、何も押さない | この手順で作る（`Settings`） |


### 2-7. 段のボタンを返す関数を宣言する

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **45行目** `Range ClassifyRange(float distance) const;` の下に、1行足します（46行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 46 -->
<table class="code-lines"><tr><td>

```cpp
46
```

</td><td>

```cpp
	const RangeAction& GetRangeAction(Range range) const;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 46 | 段を渡すと、その段の `RangeAction` を返す関数 |

- **なぜ:** 「段 → その段のボタン」の対応を、関数1つにまとめる。
- **効果:** 手順3 で、段が変わっても同じ書き方でボタンを取り出せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `GetRangeAction` | 段からボタンを取り出すため | `Range` を渡すと、設定の中のその段の `RangeAction` を返す | この手順で作る |


### 2-8. 段のボタンを返す関数を書く

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **20行目**（空の行）の下に、12行足します（21〜32行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 21 -->
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
32
```

</td><td>

```cpp
const NpcInputSource::RangeAction& NpcInputSource::GetRangeAction(Range range) const
{
	switch (range) {
	case Range::Near:
		return m_settings.nearAction;
	case Range::Middle:
		return m_settings.midAction;
	default:
		return m_settings.farAction;
	}
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 21 | 戻り値は、設定の中の `RangeAction` への参照 |
| 23 | `switch` で段を分ける |
| 24〜25 | 近い |
| 26〜27 | 中くらい |
| 28〜29 | 遠い（`default`） |

- **なぜ:** `switch` で3段を分ける。`default` を「遠い」にして、`Range` に段が増えたときも、返す値がなくならないようにする。
- **効果:** 段を渡すだけで、押すボタンと押し続ける秒数が取り出せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `const RangeAction&`（戻り値） | 設定の中の値を、コピーせずに返すため | 参照を返すので、`m_settings` の中の `RangeAction` をそのまま読める | C++ の書き方 |


### 手順2の確かめ方

- ビルドが通る。
- この時点では、まだ誰も `ClassifyRange` と `GetRangeAction` を呼びません。

---

## 手順3: 押すボタンを決める

距離の段と、間隔・走る確率・押し続ける秒数から、その段でどのボタンを押すかを決めて、入力に入れます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) | 追加（`<random>`、`Settings` の間隔、状態を覚えるメンバー、`DecideButton`） |
| [NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) | 追加（ボタンを決める関数 `DecideButton`、`Update` のボタンを押す所） |

### 3-1. 設定に、間隔・走る確率・待つ時間を足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **21行目** `float moveRate = 1.0f;           // 移動入力の大きさ（0〜1）。` の下に、4行足します（22〜25行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 22 -->
<table class="code-lines"><tr><td>

```cpp
22
23
24
25
```

</td><td>

```cpp
		float decideInterval = 2.0f;     // 中くらい・遠いとき、行動を決め直す間隔（秒）。
		float runChance = 0.5f;          // 中くらい・遠いときに走る確率（0〜1）。
		float actionInterval = 3.5f;     // ボタンを押してから、次に押せるまでの秒数。
		float firstActionDelay = 0.6f;   // 相手を見つけてから最初に押せるまでの秒数。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 22 | 中くらい・遠いときに、行動を決め直す間隔 |
| 23 | 中くらい・遠いときに、走る確率 |
| 24 | ボタンを押してから、次に押せるまでの秒数 |
| 25 | 相手を見つけてから、最初に押せるまでの秒数 |

- **なぜ:** 押す間隔・走る確率・待つ時間も、設定の値にする。
- **効果:** 使うキャラクターごとに、AI の攻めかたの速さを変えられる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `decideInterval` | 行動を決め直す間隔を決めるため | 歩いている間に、走るかボタンを押すかを決め直す秒数（既定 2） | この手順で作る（`Settings`） |
| メンバー変数 | `runChance` | 走るかどうかを確率で決めるため | 決め直すときに、走る確率（0〜1。既定 0.5） | この手順で作る（`Settings`） |
| メンバー変数 | `actionInterval` | 押しすぎないため | ボタンを押してから、次に押せるまでの秒数（既定 3.5） | この手順で作る（`Settings`） |
| メンバー変数 | `firstActionDelay` | 見つけた瞬間に押さないため | 相手を見つけてから、最初に押せるまでの秒数（既定 0.6） | この手順で作る（`Settings`） |


### 3-2. 状態を覚えるメンバーを足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **52行目** `Settings m_settings;` の下に、4行足します（53〜56行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 53 -->
<table class="code-lines"><tr><td>

```cpp
53
54
55
56
```

</td><td>

```cpp
	bool m_hasTarget = false;                     // 前フレームに相手を見つけていたか。
	bool m_isRunning = false;                     // 走って近づいている最中か。
	float m_decideTimer = 0.0f;                   // 次に行動を決め直すまでの秒数。
	float m_actionTimer = 0.0f;                   // 次にボタンを押せるまでの秒数。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 53 | 前のフレームに相手を見つけていたか |
| 54 | 走って近づいている最中か |
| 55 | 次に行動を決め直すまでの秒数 |
| 56 | 次にボタンを押せるまでの秒数 |

- **なぜ:** 相手を見つけた瞬間・走っている最中・待ち時間は、フレームをまたいで覚えておく必要がある。
- **効果:** AI が、前のフレームまでの状態を覚えたまま動ける。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `m_hasTarget` | 相手を見つけた瞬間がわかるようにするため | 前のフレームで相手を見つけていたか | この手順で作る |
| メンバー変数 | `m_isRunning` | 走って近づいている最中かを覚えるため | `true` の間は、決め直さず、近くまで来るのを待つ | この手順で作る |
| メンバー変数 | `m_decideTimer` | 行動を決め直す間隔を数えるため | 次に決め直すまでの残り秒数 | この手順で作る |
| メンバー変数 | `m_actionTimer` | ボタンを押しすぎないため | 次にボタンを押せるまでの残り秒数。0 以下で押せる | この手順で作る |


### 3-3. 相手がいなくなったら、見つけていない状態に戻す

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **44行目** `if (target == nullptr) {` の下に、1行足します（45行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 45 -->
<table class="code-lines"><tr><td>

```cpp
45
```

</td><td>

```cpp
		m_hasTarget = false;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 45 | 相手がいないので、「前のフレームに見つけていた」を `false` に戻す |

- **なぜ:** 相手を見失ったあと、また見つけたときに、「見つけた直後」の処理（3-4）をやり直すため。
- **効果:** 相手が一度いなくなって戻ってきたとき、最初の待ち時間が入る。


### 3-4. 見つけた直後は、歩き出して少し待つ

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **47行目** `}` の下に、8行足します（48〜55行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 48 -->
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
	if (!m_hasTarget) {
		// 見つけた直後は歩き出し、少し待ってから行動を決める。
		m_hasTarget = true;
		m_isRunning = false;
		m_decideTimer = m_settings.decideInterval;
		m_actionTimer = m_settings.firstActionDelay;
	}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 48 | 前のフレームに見つけていなかった、つまり今見つけたとき |
| 49 | コメント |
| 50 | 見つけたことを覚える |
| 51 | 走っていない状態から始める |
| 52 | 決め直すまでの時間を、間隔に合わせる |
| 53 | 最初に押せるまでの時間を、待ち時間に合わせる |

- **なぜ:** 見つけてすぐにボタンを押すと、相手から見て不意打ちになる。少し歩かせてから行動を決める。
- **効果:** 見つけた直後は歩いて近づき、`firstActionDelay` が過ぎてから最初のボタンを押す。


### 3-5. ボタンを押せるまでの時間を進める

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **68行目**（空の行）の下に、4行足します（69〜72行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 69 -->
<table class="code-lines"><tr><td>

```cpp
69
70
71
72
```

</td><td>

```cpp
	if (m_actionTimer > 0.0f) {
		m_actionTimer -= context.deltaTime;
	}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 69〜71 | 待ち時間が残っていれば、前のフレームからの秒数だけ減らす |

- **なぜ:** 待ち時間は、フレームの数でなく秒で数える。フレームレートが変わっても、押す間隔が変わらない。
- **効果:** `m_actionTimer` が 0 以下になったときに、ボタンを押せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `deltaTime` | 時間を秒で進めるため | 前のフレームから今のフレームまでの秒数 | [入力の手順](../入力/入力の手順.md) で作った（`InputSourceContext`） |


### 3-6. 押すボタンを決める関数を宣言する

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **50行目** `const RangeAction& GetRangeAction(Range range) const;` の下に、2行足します（51〜52行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 51 -->
<table class="code-lines"><tr><td>

```cpp
51
52
```

</td><td>

```cpp
	// 押すボタンを決める。押さないなら ActionButton::Num を返す。
	ActionButton DecideButton(Range range, float deltaTime);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 51 | コメント |
| 52 | 段と秒数を渡すと、押すボタンを返す関数。押さないなら `ActionButton::Num` を返す |

- **なぜ:** 「どのボタンを押すか」の決め方を関数にまとめて、`Update` から切り出す。
- **効果:** 3-7 以降で、決め方を段ごとに書き足していける。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `DecideButton` | 押すボタンを決めるため | 段と、前のフレームからの秒数を渡すと、押すボタンを返す。押さないなら `ActionButton::Num` | この手順で作る |


### 3-7. 近いときは、間隔が過ぎたらボタンを押す

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **32行目**（空の行）の下に、11行足します（33〜43行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 33 -->
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
42
43
```

</td><td>

```cpp
ActionButton NpcInputSource::DecideButton(Range range, float deltaTime)
{
	if (range == Range::Near) {
		m_decideTimer = m_settings.decideInterval;
		if (m_actionTimer <= 0.0f) {
			return m_settings.nearAction.button;
		}
		return ActionButton::Num;
	}
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 33 | `DecideButton` の定義の始まり |
| 35 | 近いとき |
| 36 | 決め直すまでの時間を、間隔に戻しておく（近いときは決め直さない） |
| 37〜39 | 押せるなら、近いときのボタンを返す |
| 40 | 押せないなら、何も押さない |

- **なぜ:** 近いときは、走るかを決めずに、間隔が過ぎたらすぐ攻撃する。
- **効果:** 相手に近づいたら、近いときのボタンを `actionInterval` ごとに押す。ただし、まだ中くらい・遠いときの処理がないので、この関数は近い以外で値を返さない（3-12 で書き終える）。


### 3-8. 走って近くまで来たら、歩きに戻してすぐ押す

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **35行目** `if (range == Range::Near) {` の下に、5行足します（36〜40行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 36 -->
<table class="code-lines"><tr><td>

```cpp
36
37
38
39
40
```

</td><td>

```cpp
		if (m_isRunning) {
			// 走って近くまで来たら、歩きに戻してすぐに押す。
			m_isRunning = false;
			m_actionTimer = 0.0f;
		}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 36 | 走っている最中なら |
| 37 | コメント |
| 38 | 歩きに戻す |
| 39 | 待ち時間を 0 にして、すぐ押せるようにする |

- **なぜ:** 走って近づいたあとは、その場で間を置かずに攻撃したい。
- **効果:** 「走って、近くまで来たら、すぐに近いときのボタンを押す」という動きができる。


### 3-9. 走っている間は押さない

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **46行目** `}` の下に、4行足します（47〜50行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 47 -->
<table class="code-lines"><tr><td>

```cpp
47
48
49
50
```

</td><td>

```cpp
	if (m_isRunning) {
		// 走っている間は、近くまで来るまで押さない。
		return ActionButton::Num;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 47 | 走っている最中なら |
| 48 | コメント |
| 49 | 何も押さない |

- **なぜ:** 走っている間にボタンを押すと、走りながら攻撃・魔法を撃つことになる。近づくまでは走ることだけをする。
- **効果:** 走っている間は、ボタンも決め直しもしない。


### 3-10. 乱数を持つ

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **2行目**（空の行）の下に、1行足します（3行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 3 -->
<table class="code-lines"><tr><td>

```cpp
3
```

</td><td>

```cpp
#include <random>
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 3 | 乱数の部品 `<random>` |

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **55行目** `Settings m_settings;` の下に、1行足します（56行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 56 -->
<table class="code-lines"><tr><td>

```cpp
56
```

</td><td>

```cpp
	std::mt19937 m_random{ std::random_device{}() };
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 56 | 乱数を作るもの |

- **なぜ:** 走るかどうかを、確率で決める。人が操作するように、毎回同じ動きにはならないようにする。
- **効果:** 起動のたびに違う乱数の並びから始められる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `#include <random>` | 乱数の部品を使うため | 乱数の部品が入った標準ライブラリを読み込む | C++ の標準ライブラリ（`<random>`） |
| 型 | `std::mt19937` | 乱数のもとを作るため | 乱数を作るもの（メルセンヌ・ツイスター）。`rand()` よりかたよりが小さい | C++ の標準ライブラリ（`<random>`） |
| 型 | `std::random_device` | 起動のたびに違う乱数の並びから始めるため | 予測できない値を1つ返す。`std::mt19937` の最初の値にする | C++ の標準ライブラリ（`<random>`） |
| メンバー変数 | `m_random` | 乱数を作るため | 走るかを決める乱数の元 | この手順で作る |


### 3-11. 歩いている間は、間隔ごとに決め直す

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **50行目** `}` の下に、6行足します（51〜56行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 51 -->
<table class="code-lines"><tr><td>

```cpp
51
52
53
54
55
56
```

</td><td>

```cpp
	// 中くらい・遠いときは、歩きながら間隔ごとに決め直す。
	m_decideTimer -= deltaTime;
	if (m_decideTimer > 0.0f || m_actionTimer > 0.0f) {
		return ActionButton::Num;
	}
	m_decideTimer = m_settings.decideInterval;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 51 | コメント |
| 52 | 決め直すまでの時間を、前のフレームからの秒数だけ減らす |
| 53〜55 | まだ時間が残っているか、ボタンを押せない間は、何も押さない |
| 56 | 決め直す時間が来たので、次の時間を決める |

- **なぜ:** 歩いている間に毎フレーム決めると、1秒間に何十回も走るかを決めることになる。間隔ごとに1回だけ決める。
- **効果:** 中くらい・遠いときは、`decideInterval` ごとに行動を決める。


### 3-12. 走るか、その段のボタンを押すかを決める

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **56行目** `m_decideTimer = m_settings.decideInterval;` の下に、6行足します（57〜62行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 57 -->
<table class="code-lines"><tr><td>

```cpp
57
58
59
60
61
62
```

</td><td>

```cpp
	std::uniform_real_distribution<float> chance(0.0f, 1.0f);
	if (chance(m_random) < m_settings.runChance) {
		m_isRunning = true;
		return ActionButton::Num;
	}
	return GetRangeAction(range).button;
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 57 | 0〜1 の乱数を作る準備 |
| 58〜61 | 乱数が `runChance` より小さければ、走り始める。このフレームは押さない |
| 62 | 走らないときは、その段のボタンを返す |

- **なぜ:** 近づくか、その場で撃つかを、確率で決める。いつも同じ動きになると、プレイヤーに読まれてしまう。
- **効果:** 中くらい・遠いときに、確率で走る。走らないときは、その段のボタンを押す。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `chance` | 走るかどうかを、確率で決めるため | 0〜1 の小数を、どの値も同じ確率で作る。`chance(m_random)` で1つ引く | `DecideButton()` の中のローカル変数（型は `std::uniform_real_distribution<float>`） |
| 型 | `std::uniform_real_distribution` | 決めた範囲の乱数を作るため | `(0.0f, 1.0f)` を渡すと、その範囲の `float` の乱数を返す。`rand()` の代わり | C++ の標準ライブラリ（`<random>`。3-10 で読み込み済み） |


### 3-13. 押し続けているボタンを覚えるメンバーを足す

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **60行目** `float m_actionTimer = 0.0f;                   // 次にボタンを押せるまでの秒数。` の下に、2行足します（61〜62行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 61 -->
<table class="code-lines"><tr><td>

```cpp
61
62
```

</td><td>

```cpp
	ActionButton m_holdButton = ActionButton::Num;  // 押し続けているボタン。Num なら押していない。
	float m_holdTimer = 0.0f;                     // ボタンを押し続ける残り秒数。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 61 | 押し続けているボタン。`Num` なら押していない |
| 62 | ボタンを押し続ける残り秒数 |

- **なぜ:** 溜める魔法（炎魔法）は、押し続けてから離したときに撃つ。押している途中のボタンと残り秒数を、フレームをまたいで覚える。
- **効果:** 押し続ける秒数のあいだ、同じボタンを押し続けられる。


### 3-14. 押し続けている間は、ほかのボタンを押さない

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **104行目**（空の行）の下に、14行足します（105〜118行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 105 -->
<table class="code-lines"><tr><td>

```cpp
105
106
107
108
109
110
111
112
113
114
115
116
117
118
```

</td><td>

```cpp
	ActionButton pressButton = ActionButton::Num;  // このフレームに押しているボタン。Num なら押さない。
	if (m_holdButton != ActionButton::Num) {
		// 押し続けている途中。時間が来たら離す（離したフレームも、ほかのボタンは押さない。
		// 炎魔法は離した瞬間に撃つので、同じフレームに別のボタンを押すと重なるため）。
		m_holdTimer -= context.deltaTime;
		if (m_holdTimer > 0.0f) {
			pressButton = m_holdButton;
		}
		else {
			m_holdButton = ActionButton::Num;
		}
	}
	else {
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 105 | 今のフレームに押すボタン。`Num` なら押さない |
| 106 | 押し続けている途中なら |
| 107〜108 | コメント |
| 109 | 残り秒数を減らす |
| 110〜112 | まだ残っているなら、同じボタンを押し続ける |
| 113〜115 | 時間が来たら、押し続けるのをやめる。このフレームは何も押さない |
| 117 | 押し続けていないとき（3-15 で書く） |

- **なぜ:** 離したフレームに別のボタンを押すと、溜め撃ちの発射と重なる。炎魔法は離した瞬間に撃つので、同じフレームに剣を振ると、魔法と剣が重なってしまうため。
- **効果:** 押している間と、離したフレームは、ほかのボタンを押さない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `pressButton` | 今のフレームに押すボタンを決めるため | 押すボタン。`ActionButton::Num` なら何も押さない | `Update()` の中のローカル変数（`NpcInputSource.cpp`） |


### 3-15. 押すボタンを決めて、押し続ける時間を覚える

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **117行目** `else {` の下に、8行足します（118〜125行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 118 -->
<table class="code-lines"><tr><td>

```cpp
118
119
120
121
122
123
124
125
```

</td><td>

```cpp
		const Range range = ClassifyRange(distance);
		pressButton = DecideButton(range, context.deltaTime);
		if (pressButton != ActionButton::Num) {
			m_actionTimer = m_settings.actionInterval;
			m_decideTimer = m_settings.decideInterval;
			m_holdButton = pressButton;
			m_holdTimer = GetRangeAction(range).holdSeconds;
		}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 118 | 距離から段を決める |
| 119 | 段に合わせて、押すボタンを決める（3-7〜3-12） |
| 120 | 押すことにしたら |
| 121〜122 | ボタンを押せる時間と、決め直す時間を、設定の間隔に戻す |
| 123〜124 | 押し続けるボタンと、その秒数を覚える（`holdSeconds` が 0 なら、次のフレームで離す） |

- **なぜ:** ボタンを押した瞬間に、「次に押せるまで」「押し続ける秒数」を決める。
- **効果:** `actionInterval` ごとに、段のボタンを押せる。溜める魔法なら、`holdSeconds` のあいだ押し続けてから離す。


### 3-16. 押すボタンを入力に入れる

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **126行目** `}` の下に、5行足します（127〜131行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 127 -->
<table class="code-lines"><tr><td>

```cpp
127
128
129
130
131
```

</td><td>

```cpp
	input.isRun = m_isRunning;
	for (int button = 0; button < static_cast<int>(ActionButton::Num); button++) {
		input.actions[button].Update(button == static_cast<int>(pressButton));
	}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 127 | 走っているかを入力に入れる |
| 128〜130 | 全部のボタンについて、押すボタンだけ「押している」、残りは「押していない」にする |

- **なぜ:** `ButtonState::Update` に毎フレーム「今押しているか」を渡すと、押した瞬間（`isTrigger`）と離した瞬間（`isRelease`）がわかる。人が押すのと同じ入力になる。
- **効果:** スキルが、押した瞬間に使われ、離した瞬間に溜めが終わる。AI が、プレイヤーと同じ入力でスキルを使える。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `actions` | ボタンごとの状態を持つため | `ActionButton` の数だけの `ButtonState` の配列 | [入力の手順](../入力/入力の手順.md) で作った（`CharacterInput`） |


### 3-17. 相手がいなくなったら、走るのをやめて全部のボタンを離す

[NpcInputSource.cpp](../../../../Game/Source/Character/Input/NpcInputSource.cpp) の **77行目** `m_hasTarget = false;` の下に、5行足します（78〜82行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 78 -->
<table class="code-lines"><tr><td>

```cpp
78
79
80
81
82
```

</td><td>

```cpp
		m_isRunning = false;
		m_holdButton = ActionButton::Num;
		for (ButtonState& action : input.actions) {
			action.Update(false);
		}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 78 | 走るのをやめる |
| 79 | 押し続けているボタンを忘れる |
| 80〜82 | 全部のボタンを「押していない」にする。溜めていた魔法は、ここで離される |

- **なぜ:** 相手を見失ったフレームに、ボタンを押したままにしない。押したままだと、離した瞬間が来ず、溜め撃ちの魔法が撃たれない（または、ずっと溜めたままになる）。
- **効果:** 相手がいなくなると、走るのをやめ、押していたボタンを離す。


### 3-18. クラスの説明のコメントを書く

[NpcInputSource.h](../../../../Game/Source/Character/Input/NpcInputSource.h) の **5行目**（空の行）の下に、7行足します（6〜12行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.h 6 -->
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
// NPCのAI。敵対する相手を見つけたら近づき、相手との距離を「近い・中くらい・遠い」の3段に分けて、押すボタンを決める。
//   近い          攻撃の間隔ごとに、近いときのボタン（nearAction）を押す
//   中くらい・遠い  決め直す間隔ごとに、runChance の確率で走る（isRun）。残りはその段のボタン（midAction・farAction）を押す
//   走っている     近くまで来たら歩きに戻し、すぐに近いときのボタンを押す
// 段ごとのボタンは、そのキャラクターが SkillController に登録したスキル（剣の攻撃でも魔法でもよい）を押す。
// 溜める魔法（炎魔法）のために、ボタンを押し続ける秒数を段ごとに決められる。押し続けている間は、ほかのボタンを押さない。
// AIは「どうしたいか」だけを決め、実際の移動や攻撃・魔法はキャラクター側が行う。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 6 | AI の動きの全体 |
| 7〜9 | 3段ごとの動き |
| 10〜11 | 段のボタンと、押し続ける秒数 |
| 12 | AI と、キャラクターの役割分担 |

- **なぜ:** 距離の分け方と、押すボタンの決め方は、コードだけでは読み取りにくい。クラスの前に、動きの全体を書いておく。
- **効果:** `NpcInputSource` を使うキャラクターを作る人が、ヘッダーだけで AI の動きと、`Settings` に何を入れればよいかがわかる。


### 手順3の確かめ方

- ビルドが通る。`DecideButton` は全部の道で値を返すので、警告も出ない。
- 使うキャラクターがまだないので、動かして確かめることはできません。

---

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
#pragma once

#include <random>
#include "CharacterInput.h"

// NPCのAI。敵対する相手を見つけたら近づき、相手との距離を「近い・中くらい・遠い」の3段に分けて、押すボタンを決める。
//   近い          攻撃の間隔ごとに、近いときのボタン（nearAction）を押す
//   中くらい・遠い  決め直す間隔ごとに、runChance の確率で走る（isRun）。残りはその段のボタン（midAction・farAction）を押す
//   走っている     近くまで来たら歩きに戻し、すぐに近いときのボタンを押す
// 段ごとのボタンは、そのキャラクターが SkillController に登録したスキル（剣の攻撃でも魔法でもよい）を押す。
// 溜める魔法（炎魔法）のために、ボタンを押し続ける秒数を段ごとに決められる。押し続けている間は、ほかのボタンを押さない。
// AIは「どうしたいか」だけを決め、実際の移動や攻撃・魔法はキャラクター側が行う。
class NpcInputSource : public IInputSource
{
public:
	// 1つの段で押すボタン。
	struct RangeAction
	{
		ActionButton button = ActionButton::Num;  // 押すボタン。Num ならこの段では何も押さない。
		float holdSeconds = 0.0f;                 // 押し続ける秒数。溜める魔法用。0なら1フレームだけ押す。
	};

	struct Settings
	{
		float searchRange = 900.0f;      // この距離以内の敵対する相手を追う（ここまでが「遠い」）。
		float nearRange = 220.0f;        // この距離以内が「近い」。
		float midRange = 600.0f;         // この距離以内が「中くらい」。
		float stopDistance = 150.0f;     // この距離まで近づいたら止まる。
		float moveRate = 1.0f;           // 移動入力の大きさ（0〜1）。
		float decideInterval = 2.0f;     // 中くらい・遠いとき、行動を決め直す間隔（秒）。
		float runChance = 0.5f;          // 中くらい・遠いときに走る確率（0〜1）。
		float actionInterval = 3.5f;     // ボタンを押してから、次に押せるまでの秒数。
		float firstActionDelay = 0.6f;   // 相手を見つけてから最初に押せるまでの秒数。
		float castHeight = 60.0f;        // 足元からこの高さから狙う（魔法を撃つ高さ）。
		RangeAction nearAction;          // 近いときに押すボタン。
		RangeAction midAction;           // 中くらいのときに押すボタン。
		RangeAction farAction;           // 遠いときに押すボタン。
	};

	// 相手との距離の分け方。
	enum class Range
	{
		Near,
		Middle,
		Far,
	};

	NpcInputSource() = default;
	explicit NpcInputSource(const Settings& settings);
	void SetSettings(const Settings& settings)
	{
		m_settings = settings;
	}
	void Update(const InputSourceContext& context, CharacterInput& input) override;

private:
	Range ClassifyRange(float distance) const;
	const RangeAction& GetRangeAction(Range range) const;
	// 押すボタンを決める。押さないなら ActionButton::Num を返す。
	ActionButton DecideButton(Range range, float deltaTime);

	Settings m_settings;
	std::mt19937 m_random{ std::random_device{}() };
	bool m_hasTarget = false;                     // 前フレームに相手を見つけていたか。
	bool m_isRunning = false;                     // 走って近づいている最中か。
	float m_decideTimer = 0.0f;                   // 次に行動を決め直すまでの秒数。
	float m_actionTimer = 0.0f;                   // 次にボタンを押せるまでの秒数。
	ActionButton m_holdButton = ActionButton::Num;  // 押し続けているボタン。Num なら押していない。
	float m_holdTimer = 0.0f;                     // ボタンを押し続ける残り秒数。
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
107
108
109
110
111
112
113
114
115
116
117
118
119
120
121
122
123
124
125
126
127
128
129
130
131
132
133
134
135
136
137
138
139
140
141
142
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

NpcInputSource::Range NpcInputSource::ClassifyRange(float distance) const
{
	if (distance <= m_settings.nearRange) {
		return Range::Near;
	}
	if (distance <= m_settings.midRange) {
		return Range::Middle;
	}
	return Range::Far;
}

const NpcInputSource::RangeAction& NpcInputSource::GetRangeAction(Range range) const
{
	switch (range) {
	case Range::Near:
		return m_settings.nearAction;
	case Range::Middle:
		return m_settings.midAction;
	default:
		return m_settings.farAction;
	}
}

ActionButton NpcInputSource::DecideButton(Range range, float deltaTime)
{
	if (range == Range::Near) {
		if (m_isRunning) {
			// 走って近くまで来たら、歩きに戻してすぐに押す。
			m_isRunning = false;
			m_actionTimer = 0.0f;
		}
		m_decideTimer = m_settings.decideInterval;
		if (m_actionTimer <= 0.0f) {
			return m_settings.nearAction.button;
		}
		return ActionButton::Num;
	}
	if (m_isRunning) {
		// 走っている間は、近くまで来るまで押さない。
		return ActionButton::Num;
	}
	// 中くらい・遠いときは、歩きながら間隔ごとに決め直す。
	m_decideTimer -= deltaTime;
	if (m_decideTimer > 0.0f || m_actionTimer > 0.0f) {
		return ActionButton::Num;
	}
	m_decideTimer = m_settings.decideInterval;
	std::uniform_real_distribution<float> chance(0.0f, 1.0f);
	if (chance(m_random) < m_settings.runChance) {
		m_isRunning = true;
		return ActionButton::Num;
	}
	return GetRangeAction(range).button;
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
		m_isRunning = false;
		m_holdButton = ActionButton::Num;
		for (ButtonState& action : input.actions) {
			action.Update(false);
		}
		return;
	}
	if (!m_hasTarget) {
		// 見つけた直後は歩き出し、少し待ってから行動を決める。
		m_hasTarget = true;
		m_isRunning = false;
		m_decideTimer = m_settings.decideInterval;
		m_actionTimer = m_settings.firstActionDelay;
	}

	// 高低差を無視して、水平方向に相手を向いて近づく。
	Vector3 toTarget = target->GetCenter() - context.position;
	toTarget.y = 0.0f;
	const float distance = toTarget.Length();
	if (distance > FLT_EPSILON) {
		Vector3 direction = toTarget;
		direction.Normalize();
		input.lookDirection = direction;
		if (distance > m_settings.stopDistance) {
			input.moveDirection = direction * m_settings.moveRate;
		}
	}

	if (m_actionTimer > 0.0f) {
		m_actionTimer -= context.deltaTime;
	}

	ActionButton pressButton = ActionButton::Num;  // このフレームに押しているボタン。Num なら押さない。
	if (m_holdButton != ActionButton::Num) {
		// 押し続けている途中。時間が来たら離す（離したフレームも、ほかのボタンは押さない。
		// 炎魔法は離した瞬間に撃つので、同じフレームに別のボタンを押すと重なるため）。
		m_holdTimer -= context.deltaTime;
		if (m_holdTimer > 0.0f) {
			pressButton = m_holdButton;
		}
		else {
			m_holdButton = ActionButton::Num;
		}
	}
	else {
		const Range range = ClassifyRange(distance);
		pressButton = DecideButton(range, context.deltaTime);
		if (pressButton != ActionButton::Num) {
			m_actionTimer = m_settings.actionInterval;
			m_decideTimer = m_settings.decideInterval;
			m_holdButton = pressButton;
			m_holdTimer = GetRangeAction(range).holdSeconds;
		}
	}
	input.isRun = m_isRunning;
	for (int button = 0; button < static_cast<int>(ActionButton::Num); button++) {
		input.actions[button].Update(button == static_cast<int>(pressButton));
	}

	// 足元から castHeight の高さから、相手の胸のあたりを狙う。
	input.hasAim = true;
	input.aimOrigin = context.position;
	input.aimOrigin.y += m_settings.castHeight;
	input.aimDirection = target->GetCenter() - input.aimOrigin;
}
```

</td></tr></table>
