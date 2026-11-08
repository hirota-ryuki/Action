# JSON読み込みの手順

作成日: 2026-11-08
設計: [JSON読み込みの設計](JSON読み込みの設計.md)
経緯: [JSON読み込みの経緯](JSON読み込みの経緯.md)

nlohmann/json を入れて、ゲームから `JsonFile::Load("player.json")` で JSON ファイルを読めるようにするまでの手順です。上から順にやると、[JSON読み込みの設計](JSON読み込みの設計.md) の形になります。

**出来上がるもの:** `#include "System/JsonFile.h"` と書いた `.cpp` で、`Assets/data/` の JSON を読めます。ファイルがない・書き方が壊れているときも落ちず、`value(キー, 既定値)` で読めば既定値で動きます。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 使う nlohmann/json は **3.11.3**。
- ほかの手順書を先に済ませる必要はない。
- 作業を始める前に、main からブランチを切る。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | `json.hpp` を `exlib/nlohmann/` に置く | ファイルがある |
| 2 | Game から `exlib` を読めるようにする | ビルドが通る |
| 3 | `JsonFile.h` を作り、2つの関数を宣言する | ビルドが通る |
| 4 | `JsonFile.cpp` を作り、パスの決め方とログの出し方を書く | ビルドが通る |
| 5 | 読めたかどうかを返す `TryLoad()` を書く | ビルドが通る |
| 6 | 結果だけを返す `Load()` を書く | JSON の値が読める |

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [json.hpp](../../../exlib/nlohmann/json.hpp) | 新しく置く（公式のリリースからコピー） | 1 |
| [Game.vcxproj](../../../Game/Game.vcxproj) | 書き換え（インクルードパスに `$(ProjectDir)..\exlib` を足す）・追加（`JsonFile.h`・`JsonFile.cpp` の行） | 2・3・4 |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（`JsonFile.h`・`JsonFile.cpp` を `Source\System` に入れる行） | 3・4 |
| [JsonFile.h](../../../Game/Source/System/JsonFile.h) | 新しく作る | 3 |
| [JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) | 新しく作る → 関数を追加 | 4 → 5・6 |

消すファイル・コメントアウトする行はありません。`stdafx.h` も変えません（理由は手順3）。

`Game.vcxproj` と `Game.vcxproj.filters` は、Visual Studio の画面で操作すると自動で書き換わります。手で書く必要はありませんが、何が変わるかがわかるように diff を載せています。

---

## 手順1: json.hpp を exlib に置く

JSON の中身を読むのは nlohmann/json に任せます。1ファイル（`json.hpp`）だけで使えるライブラリなので、ImGui などと同じ `Action/exlib/` に置きます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| `Action/exlib/nlohmann/json.hpp` | 新しく置く |

<!-- copy: ../../../exlib/nlohmann/json.hpp -->

1. https://github.com/nlohmann/json/releases/tag/v3.11.3 を開き、「Assets」の `json.hpp` をダウンロードする。
2. `Action/exlib/nlohmann/` フォルダーを作り、`json.hpp` を置く。中身は変えない。

**確かめ方:** `Action/exlib/nlohmann/json.hpp` があり、先頭の方に `version 3.11.3` と書いてある。

---

## 手順2: Game から exlib を読めるようにする

`#include <nlohmann/json.hpp>` と書けるように、Game のインクルードパスに `Action/exlib` を足します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [Game.vcxproj](../../../Game/Game.vcxproj) | 書き換え（`IncludePath` の2つ目に `$(ProjectDir)..\exlib;` を足す） |

1. ソリューションエクスプローラーで `Game` を右クリック →「プロパティ」。
2. 上の「構成」を「すべての構成」、「プラットフォーム」を「すべてのプラットフォーム」にする。
3. 「構成プロパティ → VC++ ディレクトリ → インクルード ディレクトリ」で、先頭の `$(ProjectDir)Source;` のすぐ後ろに `$(ProjectDir)..\exlib;` を足して「OK」。

[Game.vcxproj](../../../Game/Game.vcxproj) は、次のように**書き換え**られます（1行が長いので、変わるのは `Source;` の後ろだけです）。

```diff
   <PropertyGroup>
-    <IncludePath>$(ProjectDir)Source;$(ProjectDir)Source\App;$(ProjectDir)Source\Core;$(ProjectDir)Source\Objects;$(ProjectDir)Source\System;$(ProjectDir)Source\Actor;$(ProjectDir)Source\Character;$(ProjectDir)Source\Character\Input;$(ProjectDir)Source\Character\Components;$(ProjectDir)Source\Character\StateMachine;$(ProjectDir)Source\Player;$(ProjectDir)Source\Player\State;$(ProjectDir)Source\Enemy;$(ProjectDir)Source\Enemy\State;$(ProjectDir)Source\Magic;$(ProjectDir)Source\UI;$(ProjectDir)Source\UI\Hud;$(ProjectDir)Source\UI\Elements;$(IncludePath)</IncludePath>
+    <IncludePath>$(ProjectDir)Source;$(ProjectDir)..\exlib;$(ProjectDir)Source\App;$(ProjectDir)Source\Core;$(ProjectDir)Source\Objects;$(ProjectDir)Source\System;$(ProjectDir)Source\Actor;$(ProjectDir)Source\Character;$(ProjectDir)Source\Character\Input;$(ProjectDir)Source\Character\Components;$(ProjectDir)Source\Character\StateMachine;$(ProjectDir)Source\Player;$(ProjectDir)Source\Player\State;$(ProjectDir)Source\Enemy;$(ProjectDir)Source\Enemy\State;$(ProjectDir)Source\Magic;$(ProjectDir)Source\UI;$(ProjectDir)Source\UI\Hud;$(ProjectDir)Source\UI\Elements;$(IncludePath)</IncludePath>
   </PropertyGroup>
```

**確かめ方:** ソリューションのビルドが通る（まだどこからも読んでいないので、動きは変わらない）。

---

## 手順3: JsonFile.h を作り、2つの関数を宣言する

ゲームのコードからは、nlohmann/json を直接さわらずに `JsonFile` を通して読むようにします。まずは「結果だけ返す `Load()`」と「読めたかも返す `TryLoad()`」の2つを宣言します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.h](../../../Game/Source/System/JsonFile.h) | 新しく作る |
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（`ClInclude` の1行。Visual Studio で足すと自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（`Source\System` に入れる3行。自動で入る） |

### 3-1. ファイルを作る

ソリューションエクスプローラーの `Game` の `Source\System` フィルターを右クリック →「追加 → 新しい項目」で、`JsonFile.h` を作ります。**場所は `Action/Game/Source/System/` にする。** 保存は UTF-8（BOM 付き）。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の1行が**追加**されます。

```diff
     <ClInclude Include="Source\App\stdafx.h" />
+    <ClInclude Include="Source\System\JsonFile.h" />
     <ClInclude Include="Source\System\system.h" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、次の3行が**追加**されます。

```diff
     <ClInclude Include="Source\App\stdafx.h">
       <Filter>Source\App</Filter>
     </ClInclude>
+    <ClInclude Include="Source\System\JsonFile.h">
+      <Filter>Source\System</Filter>
+    </ClInclude>
     <ClInclude Include="Source\System\system.h">
```

### 3-2. 宣言を書く

`JsonFile.h` に次を書きます。

<!-- new: ../../../Game/Source/System/JsonFile.h -->

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

- **`stdafx.h` には入れない。** `json.hpp` は約2.5万行あり、全部の `.cpp` で読むとビルドが遅くなります。JSON を使う `.cpp` だけで `JsonFile.h` を読みます。
- **クラスではなく namespace にする。** 持っておく状態がないので、インスタンスを作る必要がありません。
- `using Json = nlohmann::json;` で、呼ぶ側は `JsonFile::Json` と書けます。

**確かめ方:** ソリューションのビルドが通る（まだどの `.cpp` からも読んでいない）。

---

## 手順4: JsonFile.cpp を作り、パスの決め方とログの出し方を書く

`Load()` と `TryLoad()` の中で使う、2つの小さな関数を先に作ります。どちらも `JsonFile.cpp` の中だけで使うので、無名の namespace に入れます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) | 新しく作る（`ResolvePath()`・`LogError()`） |
| [Game.vcxproj](../../../Game/Game.vcxproj) | 追加（`ClCompile` の1行。自動で入る） |
| [Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) | 追加（`Source\System` に入れる3行。自動で入る） |

### 4-1. ファイルを作る

手順3と同じように、`Source\System` フィルターに `JsonFile.cpp` を作ります。場所は `Action/Game/Source/System/`、保存は UTF-8（BOM 付き）。

[Game.vcxproj](../../../Game/Game.vcxproj) には、次の1行が**追加**されます。

```diff
     </ClCompile>
+    <ClCompile Include="Source\System\JsonFile.cpp" />
     <ClCompile Include="Source\System\system.cpp" />
```

[Game.vcxproj.filters](../../../Game/Game.vcxproj.filters) には、次の3行が**追加**されます。

```diff
     <ClCompile Include="Source\App\stdafx.cpp">
       <Filter>Source\App</Filter>
     </ClCompile>
+    <ClCompile Include="Source\System\JsonFile.cpp">
+      <Filter>Source\System</Filter>
+    </ClCompile>
     <ClCompile Include="Source\System\system.cpp">
```

### 4-2. パスの決め方とログの出し方を書く

`JsonFile.cpp` に次を書きます。

<!-- new: ../../../Game/Source/System/JsonFile.cpp -->

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
}
```

- **ゲームの JSON はほぼ `Assets/data/` に置くので、呼ぶ側はファイル名だけ書けばよいようにする。** ほかのフォルダーのファイルも、`Assets/` から書けば読めます。
- **ログは `OutputDebugStringA()` で出す。** Visual Studio の出力ウィンドウに出ます。

**確かめ方:** ソリューションのビルドが通る（2つの関数はまだ使っていない）。

---

## 手順5: 読めたかどうかを返す TryLoad() を書く

ファイルを開いて JSON として読み、読めたら `true` を返します。読めなかったときは、例外を投げずに空のオブジェクトを入れて `false` を返します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) | 追加（`TryLoad()` の中身） |

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の無名の namespace の閉じかっこの下（`namespace JsonFile` の閉じかっこの上）に**追加**します。

```diff
 			OutputDebugStringA(line.c_str());
 		}
 	}
+
+	bool TryLoad(const std::string& path, Json& out)
+	{
+		// 先に空のオブジェクトを入れておく。途中で失敗して return しても、out は空の {} になる。
+		// null ではなく {} にするのは、null に value() を使うと例外になるが、{} なら既定値が返るため。
+		out = Json::object();
+
+		// ファイルを開く。ないとき・開けないときはここで失敗にする。
+		const std::string fullPath = ResolvePath(path);
+		std::ifstream file(fullPath);
+		if (!file) {
+			LogError("ファイルを開けません: " + fullPath);
+			return false;
+		}
+
+		// 中身を読む。引数の意味は次のとおり。
+		//   nullptr : 読みながら値を選り分ける関数（使わない）
+		//   false   : 書き方が壊れていても例外を投げない。代わりに is_discarded() が true の値を返す
+		//   true    : // と /* */ のコメントを許す（調整値の横に説明を書けるようにする）
+		Json parsed = Json::parse(file, nullptr, false, true);
+		if (parsed.is_discarded()) {
+			LogError("JSON の書き方が壊れています: " + fullPath);
+			return false;
+		}
+
+		// 読めたときだけ out を差し替える。move で中身をコピーせずに渡す。
+		out = std::move(parsed);
+		return true;
+	}
 }
```

- **`Json::parse()` の3つ目の引数を `false` にして、例外を使わない。** 書き方が壊れていると「捨てた値」（`is_discarded()` が true）が返ります。ゲームの中に例外を受け止める場所を作らずに済みます。
- **失敗したときは `null` ではなく空の `{}` にする。** nlohmann/json の `value()` は、オブジェクトでない値に使うと例外を投げますが、空のオブジェクトなら既定値を返します。

**確かめ方:** ソリューションのビルドが通る。

---

## 手順6: 結果だけを返す Load() を書く

ふだんは読めたかどうかを気にせず、`value()` の既定値で動けば十分です。`TryLoad()` を呼んで、結果だけを返す `Load()` を作ります。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) | 追加（`Load()` の中身） |

[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp) の `TryLoad()` の下（`namespace JsonFile` の閉じかっこの上）に**追加**します。

```diff
 		out = std::move(parsed);
 		return true;
 	}
+
+	Json Load(const std::string& path)
+	{
+		// 成功・失敗は気にせず、結果だけを返す。
+		// 失敗したときは TryLoad が result を空の {} にしているので、呼ぶ側は value() で既定値を取れる。
+		Json result;
+		TryLoad(path, result);
+		return result;
+	}
 }
```

**確かめ方（これで出来上がり）:**

1. `Action/Game/Assets/data/` に `test.json` を作り、`{ "speed": 123 }` と書く（UTF-8）。
2. どこかの `.cpp`（例: [Game.cpp](../../../Game/Source/Core/Game.cpp) の `Game::Start()`）に、仮に次を書く。

   <!-- skip -->

   ```cpp
   #include "System/JsonFile.h"   // ファイルの先頭の #include の並びに
   ...
   auto j = JsonFile::Load("test.json");
   float speed = j.value("speed", 0.0f);   // ブレークポイントを置いて 123 になっているか見る
   float none = j.value("none", 5.0f);     // キーがないので 5
   auto k = JsonFile::Load("nothing.json"); // 出力ウィンドウに [JsonFile] ファイルを開けません: ... と出る
   ```

3. 確かめたら、仮に書いたコードと `test.json` を消す。
