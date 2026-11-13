# NPCの攻撃と魔法の使い分けの手順

設計: [NPCのAIの設計](NPCのAIの設計.md)
経緯: [入力の経緯](入力の経緯.md)

NPC の AI が、近接攻撃（`attack`）・メインの魔法（`fire`）・サブの魔法（`subFire`）の3つのボタンを、相手との距離で使い分けて押せるようにする手順です。黒騎士の剣は `attack` で振るようにし、`fire`・`subFire` は魔法だけに使います。

**出来上がるもの:** 敵の `OnStart` で魔法を登録し（`m_magicCaster.AddMagic()`）、`MakeAiSettings` で `settings.fire` を設定すれば、その敵が Player と同じ魔法を撃つ。剣が届く距離なら剣を、届かなければ魔法を使う。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 先に [NPCのAIの手順](NPCのAIの手順.md)・[敵のステートの手順](../敵/敵のステートの手順.md)・[魔法と弾の手順](../魔法/魔法と弾の手順.md) を済ませておく。この手順は、それらで作ったコードの `fire` の扱いを書き換える。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | 近接攻撃のボタン `attack` | ビルドが通る |
| 2 | AI が3つのボタンを使い分ける | ビルドが通る |
| 3 | 黒騎士の剣を `attack` で振る | 黒騎士が前と同じく剣を振る |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあと（書き換えたあと）の行番号です。右の枠のコードを、そのまま書きます。
- 書き換えは「前」と「後」を並べます。「前」の番号は書き換える前、「後」の番号は書き換えたあとの番号です。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [CharacterInput.h](../../../Game/Source/Character/Input/CharacterInput.h) | 書き足す | 1 |
| [NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) | 書き足す・書き換える | 2 |
| [NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) | 書き足す・書き換える | 2 |
| [Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) | 書き換える | 2 |
| [EnemyGroundState.cpp](../../../Game/Source/Enemy/State/EnemyGroundState.cpp) | 書き換える | 3 |
| [EnemyGroundState.h](../../../Game/Source/Enemy/State/EnemyGroundState.h)・[EnemyAttackState.h](../../../Game/Source/Enemy/State/EnemyAttackState.h)・[EnemyState.h](../../../Game/Source/Enemy/State/EnemyState.h) | 書き換える（コメントだけ） | 3 |

---

## 手順1: 近接攻撃のボタン attack を足す

今は黒騎士の剣も魔法も、同じ `fire`（撃つ）のボタンで出ます。黒騎士に魔法を持たせると、剣と魔法が同時に出てしまいます。剣のための別のボタン `attack` を入力に足します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [CharacterInput.h](../../../Game/Source/Character/Input/CharacterInput.h) | 追加（`attack`） |

### 1-1. 入力に attack を足す

[CharacterInput.h](../../../Game/Source/Character/Input/CharacterInput.h) の **29行目** `ButtonState dash;` の下に、1行足します（30行目になります）。

<!-- add: CharacterInput.h -->
<table class="code-lines"><tr><td>

```cpp
30
```

</td><td>

```cpp
	ButtonState attack;                     // 近接攻撃（剣など）。今は敵の AI だけが押す。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 30 | 近接攻撃のボタン。押した瞬間・押している・離した瞬間は、ほかのボタンと同じ `ButtonState` で判定する。Player の入力元（`PlayerInputSource`）はまだ押さないので、いつも「押していない」のまま |

**確かめ方:** ビルドが通る。まだ誰も `attack` を押さないので、ゲームの動きは変わらない。

---

## 手順2: AI が押すボタンを3つにする

今の AI（`NpcInputSource`）が押せるのは `fire` だけです。`attack`・`fire`・`subFire` の3つを、ボタンごとに「届く距離・間隔・押す秒数」を決めて押せるようにします。黒騎士の剣の設定は `attack` に移します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) | 追加（`ActionSettings`・`ActionState`・関数3つ）、書き換え（冒頭の説明・`Settings` の撃つ設定・メンバー変数） |
| [NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) | 追加（関数4つ）、書き換え（`Update` のボタンを押すところ） |
| [Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) | 書き換え（`MakeAiSettings` の剣の設定、`OnStart` のコメント） |

### 2-1. ボタン1つ分の押し方を決める構造体を足す

3つのボタンは、どれも「相手がこの距離に入ったら、この間隔で、この秒数だけ押す」で決められます。その5つの値を1つの構造体にまとめます。

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **9行目** `public:` の下に、10行足します（10〜19行目になります）。

<!-- add: NpcInputSource.h -->
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
```

</td><td>

```cpp
	// ボタン1つ分の押し方。
	struct ActionSettings
	{
		bool isEnabled = false;    // このボタンを押すか。
		float range = 700.0f;      // 相手がこの距離以内なら押す。
		float interval = 4.0f;     // 押す間隔（秒）。
		float firstDelay = 1.5f;   // 相手を見つけてから最初に押すまでの秒数。
		float holdSeconds = 0.0f;  // 押し続ける秒数。溜め撃ちの魔法用。0なら1フレームだけ押す。
	};

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13 | このボタンを押すかどうか。既定は押さない（使うボタンだけ `true` にする） |
| 14 | 相手までの水平の距離がこれ以下なら押す |
| 15 | 一度押したら、次に押せるまでこれだけ待つ |
| 16 | 相手を見つけた直後にいきなり押さないための待ち時間 |
| 17 | 押し続ける秒数。炎魔法のような溜め撃ちは、この秒数だけ溜めてから離す |

### 2-2. Settings の撃つ設定を、ボタン3つ分の設定に書き換える

もとの `canFire`・`fireRange`・`fireInterval`・`firstFireDelay`・`fireHoldSeconds` は、`fire` だけの押し方でした。これを `ActionSettings` 3つに置き換えます。

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **25〜29行目** を、次のように書き換えます。

**前**

<!-- before: NpcInputSource.h -->
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
		bool canFire = true;           // 魔法を撃つか。
		float fireRange = 700.0f;      // この距離以内なら撃つ。
		float fireInterval = 4.0f;     // 撃つ間隔（秒）。
		float firstFireDelay = 1.5f;   // 相手を見つけてから最初に撃つまでの秒数。
		float fireHoldSeconds = 0.0f;  // 発射ボタンを押し続ける秒数。長押しの魔法用。0なら1フレームだけ押す。
```

</td></tr></table>

**後**

<!-- after: NpcInputSource.h -->
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
		// 押せるもののうち、range がいちばん短いものを押す（剣が届くなら剣、届かなければ魔法）。
		// どれかを押している間は、ほかのボタンは押さない。
		ActionSettings attack;         // 近接攻撃（剣など）。
		ActionSettings fire;           // メインの魔法（MagicCaster::AddMagic）。
		ActionSettings subFire;        // サブの魔法（MagicCaster::SetSubMagic）。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 25〜26 | どのボタンを押すかの決め方。剣（届く距離が短い）が届くなら剣を、届かなければ魔法を押す |
| 27〜29 | ボタンごとの押し方。`isEnabled` が既定で `false` なので、使うボタンだけ設定する |

### 2-3. ボタンごとの状態と、使う関数を足す

ボタンごとに「次に押せるまでの秒数」「押し続ける残り秒数」「押している最中か」を持ちます。3つを配列で持てるように、添字の `enum` も足します。

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **41行目** `private:` の下に、8行足します（42〜49行目になります）。

<!-- add: NpcInputSource.h -->
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
```

</td><td>

```cpp
	enum ActionIndex
	{
		ACTION_ATTACK,
		ACTION_FIRE,
		ACTION_SUB_FIRE,
		ACTION_NUM,
	};

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 42〜48 | 3つのボタンの添字。`m_actions` の配列の何番目かに使う |

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **49行目**（`};` の下の空の行）の下に、8行足します（50〜57行目になります）。

<!-- add: NpcInputSource.h -->
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
```

</td><td>

```cpp
	// ボタン1つ分の、押す・待つの状態。
	struct ActionState
	{
		float waitTimer = 0.0f;   // 次に押せるまでの秒数。
		float holdTimer = 0.0f;   // 押し続ける残り秒数。
		bool isPressing = false;  // 押している最中か。
	};

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 53 | 0 以下になったら押せる。押したら `interval` を入れる |
| 54 | 押している間に減らし、0 以下になったら離す |
| 55 | このフレームでボタンを押しているか。`CharacterInput` へはこの値を渡す |

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **57行目**（`};` の下の空の行）の下に、6行足します（58〜63行目になります）。

<!-- add: NpcInputSource.h -->
<table class="code-lines"><tr><td>

```cpp
58
59
60
61
62
63
```

</td><td>

```cpp
	const ActionSettings& GetActionSettings(int index) const;
	// 待ち時間を進め、押す・離すを決める。distanceSq は相手までの水平の距離の2乗。
	void UpdateActions(float distanceSq, float deltaTime);
	void ReleaseAll();
	void ApplyButtons(CharacterInput& input) const;

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 58 | 添字からボタンの設定を返す |
| 60 | 毎フレーム呼び、どのボタンを押すかを決める |
| 61 | 相手を見失ったとき、全部のボタンを離す |
| 62 | ボタンの状態を `CharacterInput` に書き込む |

### 2-4. もとのメンバー変数を、ボタン3つ分の状態に書き換える

もとの `m_fireTimer`・`m_fireHoldTimer`・`m_isFiring` は `fire` だけの状態でした。`ActionState` の配列に置き換えます。`m_hasTarget` は残します。

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **65〜68行目** を、次のように書き換えます。

**前**

<!-- before: NpcInputSource.h -->
<table class="code-lines"><tr><td>

```cpp
65
66
67
68
```

</td><td>

```cpp
	float m_fireTimer = 0.0f;      // 次に撃てるまでの秒数。
	float m_fireHoldTimer = 0.0f;  // 発射ボタンを押し続ける残り秒数。
	bool m_hasTarget = false;      // 前フレームに相手を見つけていたか。
	bool m_isFiring = false;       // 発射ボタンを押している最中か。
```

</td></tr></table>

**後**

<!-- after: NpcInputSource.h -->
<table class="code-lines"><tr><td>

```cpp
65
66
```

</td><td>

```cpp
	ActionState m_actions[ACTION_NUM];
	bool m_hasTarget = false;  // 前フレームに相手を見つけていたか。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 65 | 3つのボタンの状態 |
| 66 | もとからある。コメントの前の空白だけを詰めた |

### 2-5. クラスの説明を直す

[NpcInputSource.h](../../../Game/Source/Character/Input/NpcInputSource.h) の **5〜6行目** を、次のように書き換えます。

**前**

<!-- before: NpcInputSource.h -->
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

**後**

<!-- after: NpcInputSource.h -->
<table class="code-lines"><tr><td>

```cpp
5
6
7
```

</td><td>

```cpp
// NPCのAI。敵対する相手を探して近づき、届く距離に入ったら攻撃・魔法のボタンを押す入力を作る。
// AIは「どうしたいか」だけを決め、実際の移動や発射はキャラクター側が行う。
// 魔法はキャラクターの MagicCaster に登録したものが出る（登録していなければ、fire を押しても何も出ない）。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 7 | AI はボタンを押すだけで、何が出るかはキャラクターに登録した魔法で決まる |

### 2-6. 添字からボタンの設定を返す関数を書く

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **71行目** `}` の下に、12行足します（72〜83行目になります）。

<!-- add: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp

const NpcInputSource::ActionSettings& NpcInputSource::GetActionSettings(int index) const
{
	switch (index) {
	case ACTION_ATTACK:
		return m_settings.attack;
	case ACTION_FIRE:
		return m_settings.fire;
	default:
		return m_settings.subFire;
	}
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 75〜82 | `Settings` は3つを名前で持っているので、添字を名前に読みかえる |

### 2-7. 待ち時間を進め、押しているボタンを離す

`UpdateActions` の前半です。全部のボタンの待ち時間を進め、押し続ける時間が過ぎたものを離します。

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **83行目** `}` の下に、21行足します（84〜104行目になります）。

<!-- add: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp

void NpcInputSource::UpdateActions(float distanceSq, float deltaTime)
{
	// 待ち時間を進め、押し続ける時間が過ぎたものは離す。
	// このフレームで離したものも「使っている」と数え、同じフレームで別のボタンを押さない（離した瞬間に撃つ魔法があるため）。
	bool isBusy = false;
	for (ActionState& state : m_actions) {
		if (state.waitTimer > 0.0f) {
			state.waitTimer -= deltaTime;
		}
		if (state.isPressing) {
			isBusy = true;
			state.holdTimer -= deltaTime;
			if (state.holdTimer <= 0.0f) {
				state.isPressing = false;
			}
		}
	}
	if (isBusy) {
		return;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 89 | どれかを押している（またはこのフレームで離した）か |
| 91〜93 | 次に押せるまでの秒数を減らす。押していないボタンも減らす |
| 94〜100 | 押しているボタンは、押し続ける秒数を減らし、過ぎたら離す。`holdSeconds` が 0 なら、押した次のフレームで離す |
| 102〜104 | 押している途中なら、ほかのボタンは押さない。炎魔法を溜めている間に剣を振ると、溜めが中断されるため |

### 2-8. 押すボタンを1つ選ぶ

`UpdateActions` の後半です。押せるもの（使う・待ち時間が過ぎた・相手が届く距離にいる）のうち、届く距離がいちばん短いものを押します。

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **104行目** `}` の下に、14行足します（105〜118行目になります）。

<!-- add: NpcInputSource.cpp -->
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

	// 押せるもののうち、届く距離がいちばん短いものを押す。
	int chosen = -1;
	float chosenRange = FLT_MAX;
	for (int i = 0; i < ACTION_NUM; i++) {
		const ActionSettings& settings = GetActionSettings(i);
		const bool canPress = settings.isEnabled
			&& m_actions[i].waitTimer <= 0.0f
			&& distanceSq <= settings.range * settings.range;
		if (canPress && settings.range < chosenRange) {
			chosen = i;
			chosenRange = settings.range;
		}
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 111〜113 | 使うボタンで、待ち時間が過ぎていて、相手が届く距離にいれば押せる。距離は2乗のまま比べる |
| 114〜117 | 押せるもののうち、`range` がいちばん短いものを選ぶ。剣（220）と魔法（700 など）なら、剣が届くときは剣になる |

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **118行目** `}` の下に、8行足します（119〜126行目になります）。

<!-- add: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
119
120
121
122
123
124
125
126
```

</td><td>

```cpp
	if (chosen < 0) {
		return;
	}
	ActionState& state = m_actions[chosen];
	state.isPressing = true;
	state.holdTimer = GetActionSettings(chosen).holdSeconds;
	state.waitTimer = GetActionSettings(chosen).interval;
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 119〜121 | 押せるものがなければ何もしない |
| 123〜125 | 選んだボタンを押し始め、押し続ける秒数と、次に押せるまでの秒数を入れる |

### 2-9. 全部離す関数と、入力に書き込む関数を書く

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **126行目** `}` の下に、14行足します（127〜140行目になります）。

<!-- add: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
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
```

</td><td>

```cpp

void NpcInputSource::ReleaseAll()
{
	for (ActionState& state : m_actions) {
		state.isPressing = false;
	}
}

void NpcInputSource::ApplyButtons(CharacterInput& input) const
{
	input.attack.Update(m_actions[ACTION_ATTACK].isPressing);
	input.fire.Update(m_actions[ACTION_FIRE].isPressing);
	input.subFire.Update(m_actions[ACTION_SUB_FIRE].isPressing);
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 130〜132 | 相手を見失ったら全部離す。待ち時間はそのまま |
| 137〜139 | `ButtonState::Update` に「押しているか」を渡す。押した瞬間・離した瞬間は前のフレームと比べて決まる |

### 2-10. Update から使う（相手がいないとき）

ここからは、もとからある `Update` の中で、`fire` だけを扱っていたところを、作った関数に置き換えます。まず、相手がいないときです。

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **25〜26行目** を、次のように書き換えます。

**前**

<!-- before: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
25
26
```

</td><td>

```cpp
		m_isFiring = false;
		input.fire.Update(false);
```

</td></tr></table>

**後**

<!-- after: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
25
26
```

</td><td>

```cpp
		ReleaseAll();
		ApplyButtons(input);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 25 | 全部のボタンを離す |
| 26 | 離したことを入力に書き込む（`attack`・`subFire` も） |

### 2-11. Update から使う（相手を見つけた直後）

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **30〜31行目** を、次のように書き換えます。

**前**

<!-- before: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
30
31
```

</td><td>

```cpp
		// 見つけた直後にいきなり撃たないよう、少し待つ。
		m_fireTimer = m_settings.firstFireDelay;
```

</td></tr></table>

**後**

<!-- after: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
30
31
32
33
```

</td><td>

```cpp
		// 見つけた直後にいきなり攻撃しないよう、少し待つ。
		for (int i = 0; i < ACTION_NUM; i++) {
			m_actions[i].waitTimer = GetActionSettings(i).firstDelay;
		}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 31〜33 | ボタンごとに、最初に押すまでの待ち時間を入れる |

### 2-12. Update から使う（ボタンを押す）

もとの「撃つかどうかを決める」17行を消し、2行に置き換えます。

[NpcInputSource.cpp](../../../Game/Source/Character/Input/NpcInputSource.cpp) の **50〜66行目** を、次のように書き換えます。

**前**

<!-- before: NpcInputSource.cpp -->
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
62
63
64
65
66
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
	else if (m_settings.canFire && isInFireRange && m_fireTimer <= 0.0f) {
		m_isFiring = true;
		m_fireHoldTimer = m_settings.fireHoldSeconds;
		m_fireTimer = m_settings.fireInterval;
	}
	input.fire.Update(m_isFiring);
```

</td></tr></table>

**後**

<!-- after: NpcInputSource.cpp -->
<table class="code-lines"><tr><td>

```cpp
50
51
```

</td><td>

```cpp
	UpdateActions(distanceSq, context.deltaTime);
	ApplyButtons(input);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 50 | どのボタンを押すかを決める |
| 51 | 入力に書き込む |

### 2-13. 黒騎士の剣の設定を attack に移す

黒騎士の AI の設定は、もとは `fire` の設定（`canFire` など）で剣を振る間隔を決めていました。`attack` の設定に移します。値は変えません。

[Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) の **25〜29行目** を、次のように書き換えます。

**前**

<!-- before: Enemy.cpp -->
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
		// 黒騎士は魔法を持たず、fire（撃つ）の入力で剣を振る（EnemyAttackState）。
		settings.canFire = true;
		settings.fireRange = 220.0f;      // この距離以内なら剣を振る（剣の長さ＋踏み込み）。
		settings.fireInterval = 2.5f;     // 剣を振る間隔（秒）。振るのに 1.1秒かかる。
		settings.firstFireDelay = 0.6f;   // 近づいてから最初に振るまでの秒数。
```

</td></tr></table>

**後**

<!-- after: Enemy.cpp -->
<table class="code-lines"><tr><td>

```cpp
25
26
27
28
29
30
```

</td><td>

```cpp
		// 黒騎士は魔法を持たず、attack の入力で剣を振る（EnemyAttackState）。
		settings.attack.isEnabled = true;
		settings.attack.range = 220.0f;      // この距離以内なら剣を振る（剣の長さ＋踏み込み）。
		settings.attack.interval = 2.5f;     // 剣を振る間隔（秒）。振るのに 1.1秒かかる。
		settings.attack.firstDelay = 0.6f;   // 近づいてから最初に振るまでの秒数。
		// 魔法を撃つ敵は、OnStart で m_magicCaster.AddMagic() し、ここで settings.fire を設定する。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 26 | `attack` のボタンを使う |
| 27〜29 | 値はもとと同じ |
| 30 | 魔法を撃つ敵を作るときの書き方 |

[Enemy.cpp](../../../Game/Source/Enemy/Enemy.cpp) の **75行目** を、次のように書き換えます。

**前**

<!-- before: Enemy.cpp -->
<table class="code-lines"><tr><td>

```cpp
75
```

</td><td>

```cpp
	// 黒騎士は魔法を持たない（MagicCaster に何も足さないので、fire を押しても何も撃たない）。
```

</td></tr></table>

**後**

<!-- after: Enemy.cpp -->
<table class="code-lines"><tr><td>

```cpp
75
```

</td><td>

```cpp
	// 黒騎士は魔法を持たない（MagicCaster に何も足さず、AI の fire も使わない）。
```

</td></tr></table>

**確かめ方:** ビルドが通る。黒騎士の AI は `attack` を押すようになったが、ステートはまだ `fire` を見ているので、黒騎士は近づいても剣を振らない（手順3で直す）。

---

## 手順3: 黒騎士の剣を attack で振る

黒騎士のステートは、`fire` を押した瞬間に剣を振るステート（`Attack`）へ移っていました。これを `attack` に変えます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [EnemyGroundState.cpp](../../../Game/Source/Enemy/State/EnemyGroundState.cpp) | 書き換え（`CheckTransition` の1行） |
| [EnemyGroundState.h](../../../Game/Source/Enemy/State/EnemyGroundState.h) | 書き換え（コメント1行） |
| [EnemyAttackState.h](../../../Game/Source/Enemy/State/EnemyAttackState.h) | 書き換え（コメント1行） |
| [EnemyState.h](../../../Game/Source/Enemy/State/EnemyState.h) | 書き換え（コメント1行） |

### 3-1. 剣を振るステートへ移るボタンを変える

[EnemyGroundState.cpp](../../../Game/Source/Enemy/State/EnemyGroundState.cpp) の **22行目** を、次のように書き換えます。

**前**

<!-- before: EnemyGroundState.cpp -->
<table class="code-lines"><tr><td>

```cpp
22
```

</td><td>

```cpp
	if (input.fire.isTrigger) {
```

</td></tr></table>

**後**

<!-- after: EnemyGroundState.cpp -->
<table class="code-lines"><tr><td>

```cpp
22
```

</td><td>

```cpp
	if (input.attack.isTrigger) {
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 22 | `attack` を押した瞬間に `Attack` へ移る |

### 3-2. コメントを直す

動きは変わりません。

[EnemyGroundState.h](../../../Game/Source/Enemy/State/EnemyGroundState.h) の **8行目** を、次のように書き換えます。

**前**

<!-- before: EnemyGroundState.h -->
<table class="code-lines"><tr><td>

```cpp
8
```

</td><td>

```cpp
//   fire を押した → Attack（剣を振る）
```

</td></tr></table>

**後**

<!-- after: EnemyGroundState.h -->
<table class="code-lines"><tr><td>

```cpp
8
```

</td><td>

```cpp
//   attack を押した → Attack（剣を振る）
```

</td></tr></table>

[EnemyAttackState.h](../../../Game/Source/Enemy/State/EnemyAttackState.h) の **5行目** を、次のように書き換えます。

**前**

<!-- before: EnemyAttackState.h -->
<table class="code-lines"><tr><td>

```cpp
5
```

</td><td>

```cpp
// 剣を振っている。AI が fire（撃つ）を押したときに入り、Attack のアニメーションを1回流す。
```

</td></tr></table>

**後**

<!-- after: EnemyAttackState.h -->
<table class="code-lines"><tr><td>

```cpp
5
```

</td><td>

```cpp
// 剣を振っている。AI が attack を押したときに入り、Attack のアニメーションを1回流す。
```

</td></tr></table>

[EnemyState.h](../../../Game/Source/Enemy/State/EnemyState.h) の **23行目** を、次のように書き換えます。

**前**

<!-- before: EnemyState.h -->
<table class="code-lines"><tr><td>

```cpp
23
```

</td><td>

```cpp
	Attack,  // 剣を振っている（AI が fire を押したときに振る）
```

</td></tr></table>

**後**

<!-- after: EnemyState.h -->
<table class="code-lines"><tr><td>

```cpp
23
```

</td><td>

```cpp
	Attack,  // 剣を振っている（AI が attack を押したときに振る）
```

</td></tr></table>

**確かめ方:** ビルドが通る。起動して黒騎士に近づくと、前と同じく 220 の距離で剣を振る。左クリックで撃つ氷魔法は前と同じように出る。

