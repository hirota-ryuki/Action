# JSON読み込みの手順

設計: [JSON読み込みの設計](JSON読み込みの設計.md)
経緯: [JSON読み込みの経緯](JSON読み込みの経緯.md)

nlohmann/json を入れて、ゲームから `JsonFile::Load("player.json")` で JSON ファイルを読めるようにするまでの手順です。

**出来上がるもの:** `#include "System/JsonFile.h"` と書いた `.cpp` で、`Assets/data/` の JSON を読めます。ファイルがない・書き方が壊れているときも落ちず、`value(キー, 既定値)` で読めば既定値で動きます。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 使う nlohmann/json は **3.11.3**。
- ゲームの手順書の流れでは、[FPSの表示の手順](../UI/FPSの表示の手順.md) のあと、[プレイヤーの調整値の手順](../プレイヤー/プレイヤーの調整値の手順.md) の前に済ませる（行番号はこの順で数えている）。

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
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| `Action/exlib/nlohmann/json.hpp` | 新しく置く（公式のリリースからコピー） | 1 |
| Game のプロジェクトのプロパティ | インクルード ディレクトリに `$(ProjectDir)..\exlib` を足す | 2 |
| [JsonFile.h](../../../Game/Source/System/JsonFile.h) | 新しく作る | 3 |
| [JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) | 新しく作る・追加 | 4・5・6 |

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
| [JsonFile.h](../../../Game/Source/System/JsonFile.h) | 新しく作る |

`Source\System` フィルターに `JsonFile.h` を作ります。ファイルの場所は `Action/Game/Source/System/` です。

### 3-1. nlohmann/json を読み込み、名前空間を作る

[JsonFile.h](../../../Game/Source/System/JsonFile.h) を新しく作り、次の8行を書きます（1〜8行目）。

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

### 3-2. JSON の型と、置き場所を決める

[JsonFile.h](../../../Game/Source/System/JsonFile.h) の **7行目** `{` の下、`}` の上に、4行足します（8〜11行目になります）。

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

### 3-3. 結果だけを返す関数を宣言する

[JsonFile.h](../../../Game/Source/System/JsonFile.h) の **11行目** `constexpr const char* DATA_DIR = "Assets/data/";` の下に、5行足します（12〜16行目になります）。

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

### 3-4. 読めたかも返す関数を宣言する

[JsonFile.h](../../../Game/Source/System/JsonFile.h) の **16行目** `Json Load(const std::string& path);` の下に、3行足します（17〜19行目になります）。

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

### 3-5. 使い方のコメントを書く

[JsonFile.h](../../../Game/Source/System/JsonFile.h) の **5行目**（`#include <nlohmann/json.hpp>` の下の空の行）の下、`namespace JsonFile` の上に、5行足します（6〜10行目になります）。

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
| [JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) | 新しく作る（パスを決める関数、ログを出す関数） |

`Source\System` フィルターに `JsonFile.cpp` を作ります。

### 4-1. JsonFile.cpp を作り、名前空間を作る

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) を新しく作り、次の8行を書きます（1〜8行目）。

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

### 4-2. このファイルの中だけで使う関数を置く場所を作る

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **7行目** `{` の下、`}` の上に、4行足します（8〜11行目になります）。

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

### 4-3. パスを決める関数を書く

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **10行目** `{` の下、`}` の上に、9行足します（11〜19行目になります）。

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

### 4-4. "Assets/" で始まるパスはそのまま使う

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **16行目** `{` の下、`return std::string(DATA_DIR) + path;` の上に、4行足します（17〜20行目になります）。

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

### 4-5. 失敗した理由を出す関数を書く

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **23行目**（`}` の下の空の行）の下、`}` の上に、7行足します（24〜30行目になります）。

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

### 4-6. コメントを書く

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **5行目**（`#include <fstream>` の下の空の行）の下、`namespace JsonFile` の上に、2行足します（6〜7行目になります）。

<!-- add: Action/Game/Source/System/JsonFile.cpp 6 -->
<table class="code-lines"><tr><td>

```cpp
6
7
```

</td><td>

```cpp
// JSON ファイルを読む係。設計は docs/ゲーム/システム/JSON読み込みの設計.md を参照。
// 中身の解釈は nlohmann/json に任せ、ここではパスの決め方・ファイルを開く・失敗したときの扱いだけを受け持つ。
```

</td></tr></table>

**確かめ方:** ソリューションのビルドが通る。

---

## 手順5: 読めたかどうかを返す TryLoad() を書く

ファイルを開いて JSON として読み、読めたら `true` を返します。読めなかったときは、例外を投げずに空のオブジェクトを入れて `false` を返します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) | 追加（`TryLoad()` の中身） |

### 5-1. TryLoad() の形を書く

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **33行目** `}` の下、`}` の上に、9行足します（34〜42行目になります）。

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

### 5-2. ファイルを開く

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **40行目**（`out = Json::object();` の下の空の行）の下、`return true;` の上に、8行足します（41〜48行目になります）。

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

### 5-3. 中身を JSON として読む

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **48行目**（`}` の下の空の行）の下、`return true;` の上に、10行足します（49〜58行目になります）。

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

### 5-4. 読めたら out に入れる

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **58行目**（`}` の下の空の行）の下、`return true;` の上に、2行足します（59〜60行目になります）。

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

**確かめ方:** ソリューションのビルドが通る。

---

## 手順6: 結果だけを返す Load() を書く

ふだんは読めたかどうかを気にせず、`value()` の既定値で動けば十分です。`TryLoad()` を呼んで、結果だけを返します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) | 追加（`Load()` の中身） |

### 6-1. Load() を書く

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の **62行目** `}` の下、`}` の上に、9行足します（63〜71行目になります）。

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

// JSON ファイルを読む係。設計は docs/ゲーム/システム/JSON読み込みの設計.md を参照。
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
2. どこかの `.cpp`（例: [Game.cpp](../../../Game/Source/Core/Game.cpp) の `Game::Start()`）に、仮に次を書く。

   ```cpp
   #include "System/JsonFile.h"   // ファイルの先頭の #include の並びに
   ...
   auto j = JsonFile::Load("test.json");
   float speed = j.value("speed", 0.0f);   // ブレークポイントを置いて 123 になっているか見る
   float none = j.value("none", 5.0f);     // キーがないので 5
   auto k = JsonFile::Load("nothing.json"); // 出力ウィンドウに [JsonFile] ファイルを開けません: ... と出る
   ```

3. 確かめたら、仮に書いたコードと `test.json` を消す。
