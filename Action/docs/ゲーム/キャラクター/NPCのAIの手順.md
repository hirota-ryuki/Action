# NPCのAIの手順

設計: [NPCのAIの設計](NPCのAIの設計.md)
経緯: [入力の経緯](入力の経緯.md)

NPC の AI（`NpcInputSource`）を作り、Enemy の入力元にするまでの手順です。AI は「どうしたいか」だけを決めて `CharacterInput` に入れ、実際の移動や発射はキャラクター側が行います。

**出来上がるもの:** Enemy が、敵対する相手を探して近づき、射程に入ったら撃つ入力を作ります。今の Enemy の設定は、探す範囲を 0 にして止めてあります（動かすときは 900 に戻す）。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [入力の手順](入力の手順.md) を済ませておく（順番は [README の「ゲームを作る順番」](../../README.md#ゲームを作る順番)）。相手は [陣営と当たり判定の手順](陣営と当たり判定の手順.md) で作った `HitTargetRegistry` から探す。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | NPC の AI `NpcInputSource` | ビルドが通る |
| 2 | Enemy の入力元を AI にする | ビルドが通る |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- 消すところは、消す行を枠で示します（番号は消す前の番号）。書き換えるところは、「前」と「後」の枠を並べています。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) | 新しく作る | 1 |
| [NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) | 新しく作る | 1 |
| [Enemy.h](../../../Game/Source/Enemy/Enemy.h) | 削除・追加・書き換え | 2 |
| [Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) | 追加 | 2 |

新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: NPC の AI NpcInputSource を作る

AI も、プレイヤーの操作と同じく入力元（`IInputSource`）として作ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) | 新しく作る（AI の設定と、クラスの宣言） |
| [NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) | 新しく作る（相手を探して近づき、撃つ入力を作る） |

`Source\Character\Input` フィルターに `NpcInputSource.h` と `NpcInputSource.cpp` を作ります。

### 1-1. NpcInputSource クラスの形を書く

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) を新しく作り、次の11行を書きます（1〜11行目）。

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

### 1-2. AI の設定 Settings を書く（探す・近づく）

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **7行目** `public:` の下、`NpcInputSource() = default;` の上に、7行足します（8〜14行目になります）。

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

### 1-3. AI の設定に、撃つための値を足す

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **12行目** `float moveRate = 1.0f;         // 移動入力の大きさ（0〜1）。` の下に、6行足します（13〜18行目になります）。

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

### 1-4. 設定を変える関数を足す

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **22行目** `explicit NpcInputSource(const Settings& settings);` の下に、4行足します（23〜26行目になります）。

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

### 1-5. 設定と、撃つための時間を持つメンバーを足す

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **27行目** `void Update(const InputSourceContext& context, CharacterInput& input) override;` の下に、7行足します（28〜34行目になります）。

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

### 1-6. 使い方のコメントを書く

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **4行目**（`#include "CharacterInput.h"` の下の空の行）の下、`class NpcInputSource : public IInputSource` の上に、2行足します（5〜6行目になります）。

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

### 1-7. NpcInputSource.cpp を作り、コンストラクターを書く

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) を新しく作り、次の8行を書きます（1〜8行目）。

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

### 1-8. 毎フレームの関数を書き、入力を「何もしない」にする

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **8行目** `}` の下に、12行足します（9〜20行目になります）。

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
20
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
	input.magicSwitch = 0;
	input.hasAim = false;

}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 12〜18 | まず全部「何もしない」にする。このあと、要るものだけ入れる |

### 1-9. 一番近い敵を探す

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **19行目**（`input.hasAim = false;` の下の空の行）の下、`}` の上に、9行足します（20〜28行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 20 -->
<table class="code-lines"><tr><td>

```cpp
20
21
22
23
24
25
26
27
28
```

</td><td>

```cpp
	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);

	if (target == nullptr) {
		m_hasTarget = false;
		m_isFiring = false;
		input.fire.Update(false);
		return;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 20〜21 | 自分の位置から、探す範囲の中で一番近い、敵対する相手を探す。自分は除く |
| 23〜28 | 見つからなければ、撃つのをやめて終わる |

### 1-10. 見つけた直後は、少し待つ

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **28行目** `}` の下、`}` の上に、6行足します（29〜34行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 29 -->
<table class="code-lines"><tr><td>

```cpp
29
30
31
32
33
34
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
| 29〜33 | 前のフレームで見つけていなかったら、最初に撃つまでの時間を入れる。見つけた直後にいきなり撃たないように |

### 1-11. 相手までの向きと距離を求める

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **34行目**（`}` の下の空の行）の下、`}` の上に、4行足します（35〜38行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 35 -->
<table class="code-lines"><tr><td>

```cpp
35
36
37
38
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
| 36〜37 | 相手の中心までの向き。高さの差は無視して、水平に追う |
| 38 | 相手までの距離（の2乗） |

### 1-12. 相手の方を向いて近づく

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **38行目** `const float distanceSq = toTarget.LengthSq();` の下に、9行足します（39〜47行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 39 -->
<table class="code-lines"><tr><td>

```cpp
39
40
41
42
43
44
45
46
47
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
| 40〜42 | 相手の方を向く |
| 43〜45 | 止まる距離より遠ければ、相手の方へ動く |

### 1-13. 撃つための時間を進める

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **47行目**（`}` の下の空の行）の下、`}` の上に、11行足します（48〜58行目になります）。

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
56
57
58
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
| 49 | 撃てる距離にいるか |
| 50〜52 | 次に撃てるまでの時間を減らす |
| 53〜58 | 押し続けている最中なら、押す時間を減らし、なくなったら離す |

### 1-14. 撃ち始める

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **58行目** `}` の下、`}` の上に、7行足します（59〜65行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 59 -->
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
	else if (m_settings.canFire && isInFireRange && m_fireTimer <= 0.0f) {
		m_isFiring = true;
		m_fireHoldTimer = m_settings.fireHoldSeconds;
		m_fireTimer = m_settings.fireInterval;
	}
	input.fire.Update(m_isFiring);

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 59〜63 | 撃ってよくて、撃てる距離にいて、時間が来ていれば、撃ち始める |
| 64 | 撃つボタンを押しているかを入れる |

### 1-15. 相手の胸のあたりを狙う

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **65行目**（`input.fire.Update(m_isFiring);` の下の空の行）の下、`}` の上に、5行足します（66〜70行目になります）。

<!-- add: Action/Game/Source/Character/Input/NpcInputSource.cpp 66 -->
<table class="code-lines"><tr><td>

```cpp
66
67
68
69
70
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
| 67 | 照準を使う |
| 68〜69 | 足元から `castHeight` だけ上から撃つ |
| 70 | 相手の当たり判定の中心へ向けて撃つ |

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
71
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
	input.magicSwitch = 0;
	input.hasAim = false;

	const HitTarget* target = HitTargetRegistry::FindNearestHostile(
		context.position, m_settings.searchRange, context.faction, context.self);

	if (target == nullptr) {
		m_hasTarget = false;
		m_isFiring = false;
		input.fire.Update(false);
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
	input.fire.Update(m_isFiring);

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

[入力の手順](入力の手順.md) で Enemy に置いた「何も入力しない入力元」を消し、AI に替えます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Enemy.h](../../../Game/Source/Enemy/Enemy.h) | 削除（何も入力しない入力元）・追加（`#include`、コメント）・書き換え（入力元を AI に） |
| [Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) | 追加（AI の設定を作る関数、コンストラクターで AI に設定を渡す） |

### 2-1. 何も入力しない入力元を消す

[Enemy.h](../../../Game/Source/Enemy/Enemy.h) の **5〜13行目**を消します。

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

### 2-2. NpcInputSource.h を読み込む

[Enemy.h](../../../Game/Source/Enemy/Enemy.h) の **3行目** `#include "Character.h"` の下に、1行足します（4〜4行目になります）。

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

### 2-3. 返す入力元を AI にする

[Enemy.h](../../../Game/Source/Enemy/Enemy.h) の **26行目**を、次のように書き換えます。

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

### 2-4. 持つ入力元を AI にする

[Enemy.h](../../../Game/Source/Enemy/Enemy.h) の **31行目**を、次のように書き換えます。

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

### 2-5. Enemy のコメントを書き足す

[Enemy.h](../../../Game/Source/Enemy/Enemy.h) の **6行目** `// 敵キャラクター。` の下に、1行足します（7〜7行目になります）。

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

### 2-6. AI の設定を作る関数を書く

[Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) の **7行目** `constexpr float HIT_CENTER_HEIGHT = 60.0f;  // 足元から当たり判定の中心までの高さ。` の下に、9行足します（8〜16行目になります）。

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

### 2-7. 撃つための設定を足す

[Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) の **14行目** `settings.stopDistance = 90.0f;   // この距離まで近づいたら停止する（近づきすぎ防止）。` の下に、5行足します（15〜19行目になります）。

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

### 2-8. コンストラクターで AI に設定を渡す

[Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) の **25行目** `: Character(Faction::Enemy)` の下に、1行足します（26〜26行目になります）。

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

**確かめ方（これで出来上がり）:** ソリューションのビルドが通る（探す範囲が 0 なので、Enemy はまだ動かない）。
