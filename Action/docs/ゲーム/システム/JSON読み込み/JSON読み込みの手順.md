# JSON読み込みの手順

設計: [JSON読み込みの設計](JSON読み込みの設計.md)
経緯: [JSON読み込みの経緯](JSON読み込みの経緯.md)

nlohmann/json を入れて、ゲームから `JsonFile::Load("player.json")` で JSON ファイルを読めるようにするまでの手順です。

**出来上がるもの:** `#include "System/JsonFile.h"` と書いた `.cpp` で、`Assets/data/` の JSON を読めます。ファイルがない・書き方が壊れているときも落ちず、`value(キー, 既定値)` で読めば既定値で動きます。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 使う nlohmann/json は **3.11.3**。
- ゲームの手順書の流れでは、[FPSの表示の手順](../../UI/FPSの表示/FPSの表示の手順.md) のあと、[プレイヤーの調整値の手順](../../プレイヤー/プレイヤーの調整値/プレイヤーの調整値の手順.md) の前に済ませる（行番号はこの順で数えている）。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | `json.hpp` を `exlib/nlohmann/` に置く | ファイルがある |
| 2 | Game から `exlib` を読めるようにする | ビルドが通る |
| 3 | `JsonFile.h` を作り、2つの関数を宣言する | ビルドが通る |
| 4 | `JsonFile.cpp` を作り、パスの決め方とログの出し方を書く | ビルドが通る |
| 5 | 読めたかどうかを返す `TryLoad()` を書く | ビルドが通る |
| 6 | 結果だけを返す `Load()` を書く | JSON の値が読める |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる変数・引数・関数・型・マクロ・C++ の書き方を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、このゲームで作ったもの・外部ライブラリ（nlohmann/json）・C++ の標準ライブラリ・Windows の API・C++ の書き方のどれかがわかります。
- [前提知識](../../../前提知識.md) で「よく知っている」にあるもの（クラス・`namespace`・`using`・`constexpr`・`std::string`・参照・`std::vector`・`nullptr` など）は、表に載せません。「まあまあ」「怪しい」「微妙」にあるもの（`std::move` など）は載せます。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| `Action/exlib/nlohmann/json.hpp` | 新しく置く（公式のリリースからコピー） | 1 |
| Game のプロジェクトのプロパティ | インクルード ディレクトリに `$(ProjectDir)..\exlib` を足す | 2 |
| [JsonFile.h](../../../../Game/Source/System/JsonFile.h) | 新しく作る | 3 |
| [JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) | 新しく作る・追加 | 4・5・6 |

`stdafx.h` は変えません（理由は手順3）。新しく作る `.h` / `.cpp` は UTF-8（BOM 付き）で保存します。

---

## 手順1: json.hpp を exlib に置く

JSON の中身を読むのは nlohmann/json に任せます。1ファイル（`json.hpp`）だけで使えるライブラリなので、ImGui などと同じ `Action/exlib/` に置きます。

<!-- copy: ../../../exlib/nlohmann/json.hpp -->

1. https://github.com/nlohmann/json/releases/tag/v3.11.3 を開き、「Assets」の `json.hpp` をダウンロードする。
2. `Action/exlib/nlohmann/` フォルダーを作り、`json.hpp` を置く。中身は変えない。

**確かめ方:** `Action/exlib/nlohmann/json.hpp` があり、先頭の方に `version 3.11.3` と書いてある。

---

## 手順2: Game から exlib を読めるようにする

`#include <nlohmann/json.hpp>` と書けるように、Game のインクルードパスに `Action/exlib` を足します。

1. ソリューションエクスプローラーで `Game` を右クリック →「プロパティ」。
2. 上の「構成」を「すべての構成」、「プラットフォーム」を「すべてのプラットフォーム」にする。
3. 「構成プロパティ → VC++ ディレクトリ → インクルード ディレクトリ」で、先頭の `$(ProjectDir)Source;` のすぐ後ろに `$(ProjectDir)..\exlib;` を足して「OK」。

**確かめ方:** ソリューションのビルドが通る（まだどこからも読んでいないので、動きは変わらない）。

---

## 手順3: JsonFile.h を作り、2つの関数を宣言する

ゲームのコードからは、nlohmann/json を直接さわらずに `JsonFile` を通して読むようにします。「結果だけ返す `Load()`」と「読めたかも返す `TryLoad()`」の2つを宣言します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.h](../../../../Game/Source/System/JsonFile.h) | 新しく作る |

`Source\System` フィルターに `JsonFile.h` を作ります。ファイルの場所は `Action/Game/Source/System/` です。

### 3-1. nlohmann/json を読み込み、名前空間を作る

[JsonFile.h](../../../../Game/Source/System/JsonFile.h) を新しく作り、次の8行を書きます（1〜8行目）。

<!-- new: Action/Game/Source/System/JsonFile.h -->
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

// json.hpp は約2.5万行あるため stdafx.h には入れない。JSON を使う .cpp だけでこのヘッダーを読み込む。
#include <nlohmann/json.hpp>

namespace JsonFile
{
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 3〜4 | `json.hpp` は約2.5万行あるので、`stdafx.h` には入れない。JSON を使う `.cpp` だけがこのヘッダーを読む |
| 6 | 関数だけを置くので、クラスにせず名前空間にする |

- **なぜ:** `json.hpp` は約2.5万行ある。`stdafx.h` に入れると、すべての `.cpp` のビルドが遅くなる。JSON を使う `.cpp` だけが、このヘッダーを読めばよい。関数だけを置くので、クラスではなく名前空間にした。
- **効果:** JSON を使わない `.cpp` のビルドが遅くならない。ゲームのコードは `JsonFile::` と書くだけで使える。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `nlohmann/json.hpp` | JSON の中身を読むため | JSON を読み書きするライブラリの、1つだけのヘッダー。手順1で `exlib` に置いた | 外部ライブラリ（nlohmann/json）の [json.hpp](../../../../exlib/nlohmann/json.hpp) |
| C++ の書き方 | `JsonFile` | JSON を読む関数を1か所にまとめるため | `Load` などを入れる名前空間 | この手順で作る |

### 3-2. JSON の型と、置き場所を決める

[JsonFile.h](../../../../Game/Source/System/JsonFile.h) の **7行目** `{` の下、`}` の上に、4行足します（8〜11行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.h 8 -->
<table class="code-lines"><tr><td>

```cpp
8
9
10
11
```

</td><td>

```cpp
	using Json = nlohmann::json;

	// JSON の置き場所。Load にこのフォルダーから数えたパスを渡せる。
	constexpr const char* DATA_DIR = "Assets/data/";
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 8 | `nlohmann::json` を `Json` と短く書けるようにする |
| 11 | JSON の置き場所 |

- **なぜ:** `nlohmann::json` は長いので、短い名前にする。JSON を置くフォルダーも、あちこちに文字列で書かずに、定数1つにまとめる。
- **効果:** あとでフォルダーを変えるときは、この1行だけを直せばよい。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `nlohmann::json` | JSON の値1つを持つため | 数・文字・配列・オブジェクト（キーと値の組）のどれでも入る型 | 外部ライブラリ（nlohmann/json）の [json.hpp](../../../../exlib/nlohmann/json.hpp) |
| 型 | `Json` | `nlohmann::json` を短く書くため | `nlohmann::json` の別名 | この手順で作る |
| 値 | `DATA_DIR` | JSON の置き場所を1か所で決めるため | JSON を置くフォルダー `"Assets/data/"` | この手順で作る |

### 3-3. 結果だけを返す関数を宣言する

[JsonFile.h](../../../../Game/Source/System/JsonFile.h) の **11行目** `constexpr const char* DATA_DIR = "Assets/data/";` の下に、5行足します（12〜16行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.h 12 -->
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

	// path が "Assets/" で始まればそのまま、そうでなければ DATA_DIR から数えたパスとして読む。
	// ファイルがない・書き方が壊れているときは、デバッグ出力に理由を出して空のオブジェクトを返す（落ちない）。
	// 空のオブジェクトでも value() は既定値を返すので、呼ぶ側で失敗を気にしなくてよい。
	Json Load(const std::string& path);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 13〜15 | パスが `"Assets/"` で始まればそのまま、そうでなければ `Assets/data/` から数える。読めなければ空のオブジェクトを返す。空でも `value()` は既定値を返すので、呼ぶ側で失敗を気にしなくてよい |

- **なぜ:** ゲームの側は、ファイル名を渡すだけで JSON を受け取りたい。読めなかったときも空のオブジェクトを返せば、`value()` が既定値を返すので、呼ぶ側で失敗を調べなくて済む。
- **効果:** ファイルがなくても、壊れていても、ゲームが落ちずに既定値で動く。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Load()` | JSON ファイルを読むため | ファイルを読んで `Json` を返す。読めなければ空のオブジェクト `{}` を返す | この手順で作る |
| 引数 | `path` | どのファイルを読むか伝えるため | 読むファイルのパス（例: `"player.json"`） | この手順で作る |
| 関数 | `value()`（コメントに出てくる） | キーがなくても落ちずに値を取るため | `j.value("キー", 既定値)`。キーがなければ既定値を返す | 外部ライブラリ（nlohmann/json）の [json.hpp](../../../../exlib/nlohmann/json.hpp) |

### 3-4. 読めたかも返す関数を宣言する

[JsonFile.h](../../../../Game/Source/System/JsonFile.h) の **16行目** `Json Load(const std::string& path);` の下に、3行足します（17〜19行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.h 17 -->
<table class="code-lines"><tr><td>

```cpp
17
18
19
```

</td><td>

```cpp

	// 読み込みに成功したかを知りたいとき用。成功なら true を返し out に入れる。失敗なら out は空のオブジェクト。
	bool TryLoad(const std::string& path, Json& out);
```

</td></tr></table>

- **なぜ:** 読めたかどうかを知りたい場面（読み込みの確認など）のために、成功・失敗も返す関数を別に用意する。結果は引数の参照に入れる。
- **効果:** 読めたかどうかで処理を分けたいときに、`TryLoad()` を使える。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `TryLoad()` | 読めたかどうかを知るため | 読めたら `true`、読めなければ `false` を返す。結果は `out` に入る | この手順で作る |
| 引数 | `out` | 読んだ結果を呼ぶ側に返すため | 読んだ `Json` を入れる変数（参照）。失敗したときは空のオブジェクト `{}` | この手順で作る |

### 3-5. 使い方のコメントを書く

[JsonFile.h](../../../../Game/Source/System/JsonFile.h) の **5行目**（`#include <nlohmann/json.hpp>` の下の空の行）の下、`namespace JsonFile` の上に、5行足します（6〜10行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.h 6 -->
<table class="code-lines"><tr><td>

```cpp
6
7
8
9
10
```

</td><td>

```cpp
// JSON ファイルを読み込む係。
// 使い方:
//   #include "System/JsonFile.h"
//   auto j = JsonFile::Load("player.json");     // Assets/data/player.json を読む
//   float speed = j.value("runSpeed", 400.0f);  // キーがなければ 400
```

</td></tr></table>

- **なぜ:** 使う人が、ヘッダーを開いただけで使い方がわかるようにする。
- **効果:** 別のファイルで JSON を読みたいときに、このコメントをそのまま真似できる。

### 出来上がった JsonFile.h

<!-- full: Action/Game/Source/System/JsonFile.h -->
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

// json.hpp は約2.5万行あるため stdafx.h には入れない。JSON を使う .cpp だけでこのヘッダーを読み込む。
#include <nlohmann/json.hpp>

// JSON ファイルを読み込む係。
// 使い方:
//   #include "System/JsonFile.h"
//   auto j = JsonFile::Load("player.json");     // Assets/data/player.json を読む
//   float speed = j.value("runSpeed", 400.0f);  // キーがなければ 400
namespace JsonFile
{
	using Json = nlohmann::json;

	// JSON の置き場所。Load にこのフォルダーから数えたパスを渡せる。
	constexpr const char* DATA_DIR = "Assets/data/";

	// path が "Assets/" で始まればそのまま、そうでなければ DATA_DIR から数えたパスとして読む。
	// ファイルがない・書き方が壊れているときは、デバッグ出力に理由を出して空のオブジェクトを返す（落ちない）。
	// 空のオブジェクトでも value() は既定値を返すので、呼ぶ側で失敗を気にしなくてよい。
	Json Load(const std::string& path);

	// 読み込みに成功したかを知りたいとき用。成功なら true を返し out に入れる。失敗なら out は空のオブジェクト。
	bool TryLoad(const std::string& path, Json& out);
}
```

</td></tr></table>

**確かめ方:** ソリューションのビルドが通る（2つの関数はまだ使っていない）。

---

## 手順4: JsonFile.cpp を作り、パスの決め方とログの出し方を書く

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) | 新しく作る（パスを決める関数、ログを出す関数） |

`Source\System` フィルターに `JsonFile.cpp` を作ります。

### 4-1. JsonFile.cpp を作り、名前空間を作る

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) を新しく作り、次の8行を書きます（1〜8行目）。

<!-- new: Action/Game/Source/System/JsonFile.cpp -->
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
#include "JsonFile.h"

#include <fstream>

namespace JsonFile
{
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 4 | ファイルを読む `std::ifstream` を使う |

- **なぜ:** ファイルを読むために、`std::ifstream` が入っている `<fstream>` を読み込む。`.cpp` の中身は、`.h` と同じ名前空間 `JsonFile` の中に書く。
- **効果:** `TryLoad()` で、ファイルを開いて読めるようになる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| C++ の書き方 | `<fstream>` | ファイルを読み書きするため | ファイルを扱う標準のヘッダー。`std::ifstream` が入っている | C++ の標準ライブラリ（`<fstream>`）。この `.cpp` で読み込む |

### 4-2. このファイルの中だけで使う関数を置く場所を作る

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **7行目** `{` の下、`}` の上に、4行足します（8〜11行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 8 -->
<table class="code-lines"><tr><td>

```cpp
8
9
10
11
```

</td><td>

```cpp
	// このファイルの中だけで使う関数。
	namespace
	{
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 9 | 名前のない `namespace` の中は、このファイルの中でだけ使える |

- **なぜ:** パスの決め方やログの出し方は、`JsonFile` の中だけで使えばよい。ヘッダーに書かずに、無名の名前空間に入れれば、ほかのファイルから呼べない。
- **効果:** ゲーム側からは `Load()` と `TryLoad()` だけが見える。

### 4-3. パスを決める関数を書く

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **10行目** `{` の下、`}` の上に、9行足します（11〜19行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 11 -->
<table class="code-lines"><tr><td>

```cpp
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
		// 呼ぶ側が渡したパスを、実際に開くパスにする。
		// "Assets/"（または "Assets\"）で始まるならそのまま使う。
		// それ以外は Assets/data/ から数えたパスとみなし、DATA_DIR を頭に付ける。
		// 例: "player.json" → "Assets/data/player.json"
		std::string ResolvePath(const std::string& path)
		{
			return std::string(DATA_DIR) + path;
		}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 11〜14 | 渡されたパスを、実際に開くパスにする。例: `"player.json"` → `"Assets/data/player.json"` |
| 17 | `Assets/data/` を頭に付ける |

- **なぜ:** 呼ぶ側が毎回 `"Assets/data/"` を書かなくて済むように、パスを決める処理を関数にまとめる。
- **効果:** `Load("player.json")` のような短い書き方で、`Assets/data/player.json` を開ける。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `ResolvePath()` | 渡されたパスを、実際に開くパスにするため | `DATA_DIR` を頭に付けたパスを返す | この手順で作る |

### 4-4. "Assets/" で始まるパスはそのまま使う

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **16行目** `{` の下、`return std::string(DATA_DIR) + path;` の上に、4行足します（17〜20行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 17 -->
<table class="code-lines"><tr><td>

```cpp
17
18
19
20
```

</td><td>

```cpp
			// rfind(…, 0) は「先頭から始まっているか」を調べる書き方。
			if (path.rfind("Assets/", 0) == 0 || path.rfind("Assets\\", 0) == 0) {
				return path;
			}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 18 | `rfind(…, 0)` は「先頭から始まっているか」を調べる書き方。`/` と `\` のどちらでもよい |

- **なぜ:** `Assets/` から始まるパスを渡す人もいる。そのとき `DATA_DIR` を付けると `Assets/data/Assets/…` になって開けないので、そのまま使う。
- **効果:** `Assets/` 以下のほかの場所のファイルも、`Load()` で読める。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `rfind()` | 文字列が、ある文字から始まっているか調べるため | `path.rfind("Assets/", 0)` は、0 番目の位置から後ろへ探す。先頭にあるときだけ `0` を返す | C++ の標準ライブラリ（`std::string` のメンバー関数。`<string>`） |
| C++ の書き方 | `"Assets\\"` | `\` を文字として書くため | 文字列の中の `\\` は、`\` 1文字を表す | C++ の書き方 |

### 4-5. 失敗した理由を出す関数を書く

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **23行目**（`}` の下の空の行）の下、`}` の上に、7行足します（24〜30行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 24 -->
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
		// 失敗した理由を Visual Studio の出力ウィンドウに出す。
		// 頭に [JsonFile] を付けて、ほかのログと見分けられるようにする。
		void LogError(const std::string& message)
		{
			const std::string line = "[JsonFile] " + message + "\n";
			OutputDebugStringA(line.c_str());
		}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 28〜29 | 頭に `[JsonFile]` を付けて、Visual Studio の出力ウィンドウに出す |

- **なぜ:** 失敗しても落とさないので、理由がどこにも出ないと、なぜ読めないかがわからない。出力ウィンドウに出しておけば、パスの間違いにすぐ気づける。
- **効果:** ファイルがない・書き方が壊れているときに、Visual Studio の出力ウィンドウで理由がわかる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `LogError()` | 失敗した理由を出すため | 頭に `[JsonFile]` を付けて、出力ウィンドウに文字を出す | この手順で作る |
| 関数 | `OutputDebugStringA()` | Visual Studio の出力ウィンドウに文字を出すため | 文字列をデバッガーに送る。デバッグ中の Visual Studio の「出力」に出る | Windows の API（`Windows.h`。`stdafx.h` から読み込み済み） |
| 関数 | `c_str()` | `std::string` を、Windows の関数に渡せる形にするため | 文字列の先頭の `const char*` を返す | C++ の標準ライブラリ（`std::string` のメンバー関数。`<string>`） |

### 4-6. コメントを書く

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **5行目**（`#include <fstream>` の下の空の行）の下、`namespace JsonFile` の上に、2行足します（6〜7行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 6 -->
<table class="code-lines"><tr><td>

```cpp
6
7
```

</td><td>

```cpp
// JSON ファイルを読む係。設計は docs/ゲーム/システム/JSON読み込み/JSON読み込みの設計.md を参照。
// 中身の解釈は nlohmann/json に任せ、ここではパスの決め方・ファイルを開く・失敗したときの扱いだけを受け持つ。
```

</td></tr></table>

- **なぜ:** あとでこのファイルを開いた人が、設計書と、何をするファイルかをすぐ知れるようにする。
- **効果:** ファイルの役目と、設計書の場所がわかる。

**確かめ方:** ソリューションのビルドが通る。

---

## 手順5: 読めたかどうかを返す TryLoad() を書く

ファイルを開いて JSON として読み、読めたら `true` を返します。読めなかったときは、例外を投げずに空のオブジェクトを入れて `false` を返します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) | 追加（`TryLoad()` の中身） |

### 5-1. TryLoad() の形を書く

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **33行目** `}` の下、`}` の上に、9行足します（34〜42行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 34 -->
<table class="code-lines"><tr><td>

```cpp
34
35
36
37
38
39
40
41
42
```

</td><td>

```cpp

	bool TryLoad(const std::string& path, Json& out)
	{
		// 先に空のオブジェクトを入れておく。途中で失敗して return しても、out は空の {} になる。
		// null ではなく {} にするのは、null に value() を使うと例外になるが、{} なら既定値が返るため。
		out = Json::object();

		return true;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 39 | 先に空のオブジェクトを入れておく。途中で失敗しても、`out` は空の `{}` になる。`null` ではなく `{}` にするのは、`null` に `value()` を使うと例外になるが、`{}` なら既定値が返るため |

- **なぜ:** 失敗して途中で `return` しても、`out` が前に読んだ値のまま残らないように、最初に空にしておく。`null` ではなく `{}` にするのは、`null` に `value()` を使うと例外になるから。
- **効果:** どこで失敗しても、`out` は空のオブジェクトになり、呼ぶ側は `value()` で既定値を取れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Json::object()` | 空のオブジェクトを作るため | 中身が空の JSON のオブジェクト `{}` を返す | 外部ライブラリ（nlohmann/json）の [json.hpp](../../../../exlib/nlohmann/json.hpp) |

### 5-2. ファイルを開く

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **40行目**（`out = Json::object();` の下の空の行）の下、`return true;` の上に、8行足します（41〜48行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 41 -->
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
		// ファイルを開く。ないとき・開けないときはここで失敗にする。
		const std::string fullPath = ResolvePath(path);
		std::ifstream file(fullPath);
		if (!file) {
			LogError("ファイルを開けません: " + fullPath);
			return false;
		}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 42〜43 | 実際に開くパスにして、開く |
| 44〜47 | 開けなければ理由を出して、失敗にする |

- **なぜ:** ファイルがなければ JSON として読めないので、先に開けるか確かめる。開けなければ、理由を出して失敗にする。
- **効果:** ファイルがないときに、落ちずに `false` を返せる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `fullPath` | 実際に開くパスを覚えるため | `ResolvePath()` が返した、開くパス | この手順で作る |
| 型 | `std::ifstream` | ファイルを読むため | ファイルを読む入力ストリーム。作ったときにファイルを開く。開けなかったときは `if (!file)` が `true` になる | C++ の標準ライブラリ（`<fstream>`。4-1 で読み込み済み） |
| 変数 | `file` | 開いたファイルを持つため | `fullPath` を開いた `std::ifstream` | この手順で作る |

### 5-3. 中身を JSON として読む

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **48行目**（`}` の下の空の行）の下、`return true;` の上に、10行足します（49〜58行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 49 -->
<table class="code-lines"><tr><td>

```cpp
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
		// 中身を読む。引数の意味は次のとおり。
		//   nullptr : 読みながら値を選り分ける関数（使わない）
		//   false   : 書き方が壊れていても例外を投げない。代わりに is_discarded() が true の値を返す
		//   true    : // と /* */ のコメントを許す（調整値の横に説明を書けるようにする）
		Json parsed = Json::parse(file, nullptr, false, true);
		if (parsed.is_discarded()) {
			LogError("JSON の書き方が壊れています: " + fullPath);
			return false;
		}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 53 | 3つ目の `false` で、書き方が壊れていても例外を投げない（代わりに「捨てた値」が返る）。4つ目の `true` で、`//` と `/* */` のコメントを許す |
| 54〜57 | 捨てた値なら、理由を出して失敗にする |

- **なぜ:** 標準の動きだと、書き方が壊れた JSON は例外で落ちる。`parse` の3つ目の引数を `false` にして、落とさずに「捨てた値」を返してもらう。4つ目の引数を `true` にして、調整値の横にコメントを書けるようにする。
- **効果:** 壊れた JSON を読んでも落ちない。JSON に `//` で説明を書ける。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `Json::parse()` | ファイルの中身を JSON として読むため | 文字を読んで `Json` にする。引数は 入力・コールバック・例外を投げるか・コメントを許すか | 外部ライブラリ（nlohmann/json）の [json.hpp](../../../../exlib/nlohmann/json.hpp) |
| 関数 | `is_discarded()` | 読み込みが失敗したか調べるため | `parse` が失敗したときの「捨てた値」なら `true` | 外部ライブラリ（nlohmann/json）の [json.hpp](../../../../exlib/nlohmann/json.hpp) |
| 変数 | `parsed` | 読んだ結果を、いったん入れておくため | `Json::parse()` の結果。成功したときだけ `out` に移す | この手順で作る |

### 5-4. 読めたら out に入れる

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **58行目**（`}` の下の空の行）の下、`return true;` の上に、2行足します（59〜60行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 59 -->
<table class="code-lines"><tr><td>

```cpp
59
60
```

</td><td>

```cpp
		// 読めたときだけ out を差し替える。move で中身をコピーせずに渡す。
		out = std::move(parsed);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 60 | `std::move` で、中身をコピーせずに渡す |

- **なぜ:** 失敗したときに `out` を壊さないよう、読めたと確かめてから `out` に入れる。`std::move` なら、大きな JSON でも中身をコピーせずに渡せる。
- **効果:** 読めたときだけ `out` に結果が入る。コピーの時間がかからない。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `std::move` | コピーせずに渡すため | 中身を、コピーせずに移せる状態にする | C++ の標準ライブラリ（`<utility>`。`json.hpp` が読み込み済み） |

**確かめ方:** ソリューションのビルドが通る。

---

## 手順6: 結果だけを返す Load() を書く

ふだんは読めたかどうかを気にせず、`value()` の既定値で動けば十分です。`TryLoad()` を呼んで、結果だけを返します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) | 追加（`Load()` の中身） |

### 6-1. Load() を書く

[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の **62行目** `}` の下、`}` の上に、9行足します（63〜71行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 63 -->
<table class="code-lines"><tr><td>

```cpp
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

	Json Load(const std::string& path)
	{
		// 成功・失敗は気にせず、結果だけを返す。
		// 失敗したときは TryLoad が result を空の {} にしているので、呼ぶ側は value() で既定値を取れる。
		Json result;
		TryLoad(path, result);
		return result;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 68〜70 | `TryLoad()` で読み、結果だけを返す。失敗したときは空の `{}` が返る |

- **なぜ:** ふだんは読めたかどうかを気にしない。`TryLoad()` が失敗したときも `result` を `{}` にしているので、それを返せば、1つの処理にまとめられる。
- **効果:** 呼ぶ側は `JsonFile::Load("player.json")` の1行で、JSON を受け取れる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `result` | `TryLoad()` の結果を受け取るため | 読んだ `Json`。失敗したときは空の `{}` | この手順で作る |

### 出来上がった JsonFile.cpp

<!-- full: Action/Game/Source/System/JsonFile.cpp -->
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
```

</td><td>

```cpp
#include "stdafx.h"
#include "JsonFile.h"

#include <fstream>

// JSON ファイルを読む係。設計は docs/ゲーム/システム/JSON読み込み/JSON読み込みの設計.md を参照。
// 中身の解釈は nlohmann/json に任せ、ここではパスの決め方・ファイルを開く・失敗したときの扱いだけを受け持つ。
namespace JsonFile
{
	// このファイルの中だけで使う関数。
	namespace
	{
		// 呼ぶ側が渡したパスを、実際に開くパスにする。
		// "Assets/"（または "Assets\"）で始まるならそのまま使う。
		// それ以外は Assets/data/ から数えたパスとみなし、DATA_DIR を頭に付ける。
		// 例: "player.json" → "Assets/data/player.json"
		std::string ResolvePath(const std::string& path)
		{
			// rfind(…, 0) は「先頭から始まっているか」を調べる書き方。
			if (path.rfind("Assets/", 0) == 0 || path.rfind("Assets\\", 0) == 0) {
				return path;
			}
			return std::string(DATA_DIR) + path;
		}

		// 失敗した理由を Visual Studio の出力ウィンドウに出す。
		// 頭に [JsonFile] を付けて、ほかのログと見分けられるようにする。
		void LogError(const std::string& message)
		{
			const std::string line = "[JsonFile] " + message + "\n";
			OutputDebugStringA(line.c_str());
		}
	}

	bool TryLoad(const std::string& path, Json& out)
	{
		// 先に空のオブジェクトを入れておく。途中で失敗して return しても、out は空の {} になる。
		// null ではなく {} にするのは、null に value() を使うと例外になるが、{} なら既定値が返るため。
		out = Json::object();

		// ファイルを開く。ないとき・開けないときはここで失敗にする。
		const std::string fullPath = ResolvePath(path);
		std::ifstream file(fullPath);
		if (!file) {
			LogError("ファイルを開けません: " + fullPath);
			return false;
		}

		// 中身を読む。引数の意味は次のとおり。
		//   nullptr : 読みながら値を選り分ける関数（使わない）
		//   false   : 書き方が壊れていても例外を投げない。代わりに is_discarded() が true の値を返す
		//   true    : // と /* */ のコメントを許す（調整値の横に説明を書けるようにする）
		Json parsed = Json::parse(file, nullptr, false, true);
		if (parsed.is_discarded()) {
			LogError("JSON の書き方が壊れています: " + fullPath);
			return false;
		}

		// 読めたときだけ out を差し替える。move で中身をコピーせずに渡す。
		out = std::move(parsed);
		return true;
	}

	Json Load(const std::string& path)
	{
		// 成功・失敗は気にせず、結果だけを返す。
		// 失敗したときは TryLoad が result を空の {} にしているので、呼ぶ側は value() で既定値を取れる。
		Json result;
		TryLoad(path, result);
		return result;
	}
}
```

</td></tr></table>

**確かめ方（これで出来上がり）:**

1. `Action/Game/Assets/data/` に `test.json` を作り、`{ "speed": 123 }` と書く（UTF-8）。
2. どこかの `.cpp`（例: [Game.cpp](../../../../Game/Source/Core/Game.cpp) の `Game::Start()`）に、仮に次を書く。

   ```cpp
   #include "System/JsonFile.h"   // ファイルの先頭の #include の並びに
   ...
   auto j = JsonFile::Load("test.json");
   float speed = j.value("speed", 0.0f);   // ブレークポイントを置いて 123 になっているか見る
   float none = j.value("none", 5.0f);     // キーがないので 5
   auto k = JsonFile::Load("nothing.json"); // 出力ウィンドウに [JsonFile] ファイルを開けません: ... と出る
   ```

3. 確かめたら、仮に書いたコードと `test.json` を消す。
