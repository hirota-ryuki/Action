# JSON読み込みの設計

作成日: 2026-11-08
対象: [JsonFile.h](../../../../Game/Source/System/JsonFile.h)、[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp)、[json.hpp](../../../../exlib/nlohmann/json.hpp)
経緯: [JSON読み込みの経緯](JSON読み込みの経緯.md)
手順: [JSON読み込みの手順](JSON読み込みの手順.md)（どのファイルに何を足したか）

ゲームで JSON ファイルを読むための仕組みのページです。調整値やステージの配置などを、コードの外に出したいときに使います。使っている例は [プレイヤーの調整値の設計](../../プレイヤー/プレイヤーの調整値/プレイヤーの調整値の設計.md) にあります。

- **JSON の中身は nlohmann/json（`json.hpp`）で扱う。** ファイルを開く・失敗したときの扱いだけを `JsonFile` が受け持つ。
- **読み込みに失敗しても落とさない。** デバッグ出力に理由を出し、空のオブジェクトを返す。呼ぶ側は `value(キー, 既定値)` で読めば、失敗しても既定値で動く。
- **`json.hpp` は `stdafx.h` に入れない。** 約2.5万行あるので、JSON を使う `.cpp` だけで `JsonFile.h` を読み込む。

**用語:**

| 用語 | 何のことか |
|---|---|
| nlohmann/json | C++ で JSON を扱う外部ライブラリ。`json.hpp` 1ファイルだけで使える |
| 例外 | 処理を続けられない失敗を、呼んだ側へ知らせる C++ の仕組み（`throw`）。受け止める場所（`try`・`catch`）がないと、プログラムが止まる |

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

**用語:**

| 用語 | 何のことか |
|---|---|
| MIT ライセンス | 自由に使ってよいが、著作権の表示を残す、というソフトの使い方の決まり |
| インクルードパス | `#include` が、ファイルを探しに行くフォルダーの一覧。Visual Studio のプロジェクトの設定で足す |

## 2. 実装方法

### パスの決め方

`"Assets/"`（または `"Assets\"`）で始まるパスはそのまま使い、それ以外は `DATA_DIR`（`"Assets/data/"`）の後ろにつなぐ。ゲームの JSON はほぼ `Assets/data/` に置くので、呼ぶ側はファイル名だけ書けばよい。ほかのフォルダーのファイルも、`Assets/` から書けば読める。

### 例外を使わずに読む

```cpp
Json parsed = Json::parse(file, nullptr, false, true /* コメントを許す */);
if (parsed.is_discarded()) { /* 書き方が壊れている */ }
```

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `Json` | `nlohmann::json` を短く書くため | JSON の値（オブジェクト・配列・数値など）を入れる型 | ゲームの [JsonFile.h](../../../../Game/Source/System/JsonFile.h) の `JsonFile`（`using Json = nlohmann::json;`） |
| 関数 | `Json::parse()` | 文字から JSON の値を作るため | ファイルなどから読み、`Json` を返す。3つ目・4つ目の引数で、失敗したときとコメントの扱いを決める | 外部ライブラリ（nlohmann/json。[json.hpp](../../../../exlib/nlohmann/json.hpp)） |
| 変数 | `file` | 開いたファイルを渡すため | 開いた `std::ifstream`。ファイルの中身を、文字の流れとして読める | C++ の標準ライブラリ（`<fstream>`。[JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) で読み込む） |
| 変数 | `parsed` | 読めた結果を、いったん受けるため | 読んだ `Json`。壊れていたら「捨てた値」になる | ゲームの [JsonFile.cpp](../../../../Game/Source/System/JsonFile.cpp) の `TryLoad()` の中 |
| 関数 | `is_discarded()` | 読めたかを調べるため | 書き方が壊れていて読めなかった値なら `true` を返す | 外部ライブラリ（nlohmann/json。[json.hpp](../../../../exlib/nlohmann/json.hpp)） |

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

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `JsonFile::Load()` | JSON のファイルを読むため | パスを受け取り、読んだ `Json` を返す。失敗したら空の `{}` を返す | ゲームの [JsonFile.h](../../../../Game/Source/System/JsonFile.h) |
| 変数 | `j` | 読んだ JSON を持つため | 読んだ `Json`（`auto`）。`j["キー"]` で中の値を取り出せる | `auto j = JsonFile::Load(...)` で作る |
| 関数 | `get<int>()` | JSON の値を、`int` などの型で取り出すため | 値を `int` にして返す。型が違うと例外を投げる | 外部ライブラリ（nlohmann/json。[json.hpp](../../../../exlib/nlohmann/json.hpp)） |

- `j["キー"]` はキーがないと（const でない値では）その場で null を作る。キーが必ずあるとは限らない値は `value()` で読む。
- `get<int>()` は型が違うと例外を投げる。手で書いた JSON を読むときは `value()` の方が安全。

### 読めたかどうかで処理を分けたいとき

```cpp
JsonFile::Json j;
if (!JsonFile::TryLoad("stage1.json", j)) {
    // 読めなかったときの処理
}
```

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `JsonFile::TryLoad()` | 読めたかどうかで、処理を分けるため | 読めたら `true` を返し、結果を第2引数へ入れる。失敗なら `false` で、第2引数は空の `{}` | ゲームの [JsonFile.h](../../../../Game/Source/System/JsonFile.h) |
| 型 | `JsonFile::Json` | 読んだ値を入れる変数の型を書くため | `nlohmann::json` の別名 | ゲームの [JsonFile.h](../../../../Game/Source/System/JsonFile.h) |

### JSON ファイルを足す

1. `Action/Game/Assets/data/` に `.json` を置く（フォルダーがなければ作る）。文字コードは UTF-8。
2. 使う `.cpp` で `JsonFile.h` を読み込み、`JsonFile::Load("ファイル名.json")` を呼ぶ。

## 4. 決めたこと

- **ライブラリは nlohmann/json にした。** 1ファイルで入り、配列・入れ子・書き出しまで使え、使っている人が多いため。自前のパーサーは保守が増えるのでやめた。

**用語:**

| 用語 | 何のことか |
|---|---|
| パーサー | 文字で書かれたもの（ここでは JSON）を読んで、プログラムで使える値にするもの |
- **`JsonFile` は namespace にした。** 持っておく状態がないため、クラスにしてインスタンスを作る必要がない。

## 5. 既知の課題

- 書き出し（保存）の関数はまだない。必要になったら `JsonFile::Save` を足す（中身は `j.dump(4)` をファイルに書くだけ）。
- 読み込みは呼ぶたびにファイルを開く。同じファイルを何度も読むようになったら、読んだ結果を取っておく形を考える。
