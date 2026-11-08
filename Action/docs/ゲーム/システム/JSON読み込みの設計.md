# JSON読み込みの設計

作成日: 2026-11-08
対象: [JsonFile.h](../../../Game/Source/System/JsonFile.h)、[JsonFile.cpp](../../../Game/Source/System/JsonFile.cpp)、[json.hpp](../../../exlib/nlohmann/json.hpp)
経緯: [JSON読み込みの経緯](JSON読み込みの経緯.md)

ゲームで JSON ファイルを読むための仕組みのページです。調整値やステージの配置などを、コードの外に出したいときに使います。使っている例は [プレイヤーの調整値の設計](../プレイヤー/プレイヤーの調整値の設計.md) にあります。

- **JSON の中身は nlohmann/json（`json.hpp`）で扱う。** ファイルを開く・失敗したときの扱いだけを `JsonFile` が受け持つ。
- **読み込みに失敗しても落とさない。** デバッグ出力に理由を出し、空のオブジェクトを返す。呼ぶ側は `value(キー, 既定値)` で読めば、失敗しても既定値で動く。
- **`json.hpp` は `stdafx.h` に入れない。** 約2.5万行あるので、JSON を使う `.cpp` だけで `JsonFile.h` を読み込む。

---

## 1. 全体像

```
呼ぶ側の .cpp
   │  JsonFile::Load("player.json")
   ▼
JsonFile（Game/Source/System）
   │  パスを Assets/data/player.json にする → ファイルを開く → 読む
   ▼
nlohmann::json（exlib/nlohmann/json.hpp）
   │  文字列から値の木を作る
   ▼
JsonFile::Json（= nlohmann::json）を返す
```

| ファイル | 置き場所 | 役割 |
|---|---|---|
| `json.hpp` | `Action/exlib/nlohmann/` | nlohmann/json 3.11.3（MIT ライセンス）。公式リリースの1ファイルをそのまま置いている |
| `JsonFile.h` / `.cpp` | `Action/Game/Source/System/` | パスの決め方、ファイルを開く、失敗したときの扱い |
| JSON ファイル | `Action/Game/Assets/data/` | 読み込むデータ |

Game.vcxproj のインクルードパスに `$(ProjectDir)..\exlib` を足してあるので、`#include <nlohmann/json.hpp>` で読み込める。

## 2. 実装方法

### パスの決め方

`"Assets/"`（または `"Assets\"`）で始まるパスはそのまま使い、それ以外は `DATA_DIR`（`"Assets/data/"`）の後ろにつなぐ。ゲームの JSON はほぼ `Assets/data/` に置くので、呼ぶ側はファイル名だけ書けばよい。ほかのフォルダーのファイルも、`Assets/` から書けば読める。

### 例外を使わずに読む

```cpp
Json parsed = Json::parse(file, nullptr, false, true /* コメントを許す */);
if (parsed.is_discarded()) { /* 書き方が壊れている */ }
```

- 3つ目の引数 `false` で、書き方が壊れていても例外を投げずに「捨てた値」（`is_discarded()` が true）を返させる。ゲームの中で例外を受け止める場所を作らずに済む。
- 4つ目の引数 `true` で `//` と `/* */` のコメントを許す。調整値の横に説明を書けるようにするため。

### 失敗したときは空のオブジェクトを返す

失敗したときは `null` ではなく `Json::object()`（空の `{}`）を返す。nlohmann/json の `value()` は、オブジェクトでない値に使うと例外を投げるが、空のオブジェクトなら既定値を返すため。

失敗した理由はデバッグ出力（Visual Studio の出力ウィンドウ）に `[JsonFile] ファイルを開けません: Assets/data/xxx.json` のように出す。

## 3. 使い方

### 読む

```cpp
#include "System/JsonFile.h"

auto j = JsonFile::Load("player.json");            // Assets/data/player.json
float speed = j.value("runSpeed", 400.0f);         // キーがなければ 400
std::string name = j.value("name", std::string("Player"));
for (auto& v : j["list"]) { int n = v.get<int>(); }  // 配列
bool flag = j["nest"].value("a", false);           // 入れ子
```

- `j["キー"]` はキーがないと（const でない値では）その場で null を作る。キーが必ずあるとは限らない値は `value()` で読む。
- `get<int>()` は型が違うと例外を投げる。手で書いた JSON を読むときは `value()` の方が安全。

### 読めたかどうかで処理を分けたいとき

```cpp
JsonFile::Json j;
if (!JsonFile::TryLoad("stage1.json", j)) {
    // 読めなかったときの処理
}
```

### JSON ファイルを足す

1. `Action/Game/Assets/data/` に `.json` を置く（フォルダーがなければ作る）。文字コードは UTF-8。
2. 使う `.cpp` で `JsonFile.h` を読み込み、`JsonFile::Load("ファイル名.json")` を呼ぶ。

## 4. 決めたこと

- **ライブラリは nlohmann/json にした。** 1ファイルで入り、配列・入れ子・書き出しまで使え、使っている人が多いため。自前のパーサーは保守が増えるのでやめた。
- **`JsonFile` は namespace にした。** 持っておく状態がないため、クラスにしてインスタンスを作る必要がない。

## 5. 既知の課題

- 書き出し（保存）の関数はまだない。必要になったら `JsonFile::Save` を足す（中身は `j.dump(4)` をファイルに書くだけ）。
- 読み込みは呼ぶたびにファイルを開く。同じファイルを何度も読むようになったら、読んだ結果を取っておく形を考える。
