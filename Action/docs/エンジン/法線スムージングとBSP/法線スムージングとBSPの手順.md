# 法線スムージングとBSPの手順

設計: [法線スムージングとBSPの設計](法線スムージングとBSPの設計.md)
経緯: [起動時のモデルロード高速化の経緯](../../最適化/起動/起動時のモデルロード高速化/起動時のモデルロード高速化の経緯.md)

tkm を読み込むときの法線スムージングで使う BSP の探索を、速くするまでの手順です。上から順にやると、[法線スムージングとBSPの設計](法線スムージングとBSPの設計.md) の形になります。

**出来上がるもの:** Debug ビルドで、モデルを読み込む時間（起動の時間）が短くなります。見た目は変わりません。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 始める前は、エンジンにもとからある形になっている。法線スムージング（`TkmFile::BuildTangentAndBiNormal()`）と、BSP の作り方（`BSP::CreateBSPTreeEntity()` など）は、もとからある（変えない）。変えるのは、BSP の探索（`WalkTree()`）と、ノードが持っていた使っていないリストだけ。
- ほかの手順書を先に済ませる必要はない。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | 探索の中の、何もしないループを消す | ビルドが通る |
| 2 | ノードが持っていた、使っていないリストを消す | ビルドが通る |
| 3 | 探索の引数を参照で渡す（毎回コピーしない） | 起動が速くなる |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- 消すところは、消す行を枠で示します（番号は消す前の番号）。書き換えるところは、「前」と「後」の枠を並べています。
- 小手順の見出しは、足すコードのコメントか関数の名前です。何のためのコードかは、コードのコメントに書いてあります。
- コードの下に、3つを置きます。
  - 「行・何をしているか」の表: 1行ずつの動き。
  - 「なぜ」「効果」: その小手順でそう書く理由と、書くと何ができるようになるか。
  - 「初めて出てくる名前」の表: このページで初めて出てくる変数・引数・関数・型・マクロ・C++ の書き方を、コードに出てくる順に並べます。2回目からは載せないので、わからない名前はページ内検索（`Ctrl+F`）で最初に出てきた所を探してください。「どこから来たもの」で、このゲームで作ったもの・エンジン（k2Engine）の機能・C++ の標準ライブラリ・C++ の書き方のどれかがわかります。
- [前提知識](../../前提知識.md) で「よく知っている」にあるもの（参照・`const`・`auto`・範囲 for・ポインター・`std::vector` など）は、表に載せません。「まあまあ」「怪しい」「微妙」にあるもの（`std::function`）は載せます。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) | 削除・書き換え | 1・2・3 |
| [BSP.h](../../../k2EngineLow/geometry/BSP.h) | 削除・書き換え | 2・3 |

---

**もとからあって、変えないもの:** 法線スムージングは、頂点ごとに `WalkTree()` で同じ座標の頂点を探します（[TkmFile.cpp](../../../k2EngineLow/tkFile/TkmFile.cpp) の `BuildTangentAndBiNormal()`）。頂点の数だけ `WalkTree()` が呼ばれるので、`WalkTree()` の中の無駄が、そのまま読み込みの時間になっていました。

---

## 手順1: 探索の中の、何もしないループを消す

`WalkTree()` は、ノードに来るたびに、そのノードが持つ全部のリーフを回すループに入っていました。中身が空なので何もしませんが、Debug ビルドでは最適化されずに毎回回ります。根に近いノードほどリーフが多いので、これがいちばん重い所でした。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) | 削除 |

### 1-1. `WalkTree()` の中の行を消す

[BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) の **372〜374行目**を消します。

<!-- del: Action/k2EngineLow/geometry/BSP.cpp 372 -->
<table class="code-lines"><tr><td>

```cpp
372
373
374
```

</td><td>

```cpp
            for (SEntityPtr& entity : node->leafArray) {
                 
            }
```

</td></tr></table>

- **なぜ:** このループは、中身が空で何もしない。Debug ビルドでは最適化されないので、ノードに来るたびに全部のリーフを回ってしまい、時間だけがかかる。
- **効果:** 探索のたびに、リーフの数だけ回る無駄がなくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `SEntityPtr` | BSP の要素（ノード・リーフ）を、長い名前を書かずに持つため | `std::shared_ptr<SEntity>` の別名。`shared_ptr` は、何か所から持たれても、最後の1か所がなくなったときに1回だけ自動で消えるポインター | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |
| 変数 | `entity` | `leafArray` の要素を1つずつ取り出すため | このループで取り出した、リーフ1つ（`SEntityPtr` への参照）。中では使っていない | この行で作る（消す行） |
| 変数 | `node` | 今見ているノードを持つため | `WalkTree()` の中で、要素を `SNode*` にしたもの | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp)（もとからある） |
| メンバー変数 | `leafArray` | ノードの下にある全部のリーフを持つため | ノード（`SNode`）が持っていたリーフのリスト。このループでしか使っていなかった | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h)（手順2で消す） |

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: ノードが持っていた、使っていないリストを消す

ノード（`SNode`）は、自分の下の全部のリーフのリスト `leafArray` を持っていました。手順1のループでしか使っておらず、作るたびにリストをまるごとコピーしていたので、消します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) | 削除 |
| [BSP.h](../../../k2EngineLow/geometry/BSP.h) | 削除 |

### 2-1. `CreateBSPTreeEntity()` の中の行を消す

[BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) の **336行目**を消します。

<!-- del: Action/k2EngineLow/geometry/BSP.cpp 336 -->
<table class="code-lines"><tr><td>

```cpp
336
```

</td><td>

```cpp
        newNodePtr->leafArray = leafArray;
```

</td></tr></table>

- **なぜ:** `leafArray` は、手順1で消したループでしか使っていなかった。それなのに、ノードを作るたびにリストをまるごとコピーしていた。
- **効果:** ノードを作る時間と、使うメモリが減る。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `newNodePtr` | 新しく作るノードを持つため | `std::make_shared<SNode>()` で作ったノード。この行で、`leafArray` のコピーを入れていた | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp)（もとからある） |

### 2-2. 行を消す

[BSP.h](../../../k2EngineLow/geometry/BSP.h) の **45行目**を消します。

<!-- del: Action/k2EngineLow/geometry/BSP.h 45 -->
<table class="code-lines"><tr><td>

```cpp
45
```

</td><td>

```cpp
			std::vector<SEntityPtr> leafArray;	// ノードが内包しているリーフ。
```

</td></tr></table>

- **なぜ:** 2-1 で `leafArray` に入れるのをやめたので、使われないメンバー変数が残る。使わないものは消しておく。
- **効果:** `SNode` が小さくなり、「このリストは使うのか」と迷わなくなる。

**確かめ方:** ソリューションのビルドが通る。

---

## 手順3: 探索の引数を参照で渡す

`WalkTree()` は、ノードの `shared_ptr` と、見つけたときに呼ぶ `std::function` を値で受け取っていたので、潜るたびにコピーしていました（参照カウントの増減と、関数オブジェクトのコピー）。どちらも `const` の参照で受け取るようにします。リーフのリストを回すループの変数も、参照にします。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) | 書き換え |
| [BSP.h](../../../k2EngineLow/geometry/BSP.h) | 書き換え |

### 3-1. `WalkTree()` を書き換える

[BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) の **365行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/k2EngineLow/geometry/BSP.cpp 365 -->
<table class="code-lines"><tr><td>

```cpp
365
```

</td><td>

```cpp
    void BSP::WalkTree(SEntityPtr entityPtr, const Vector3& pos, std::function<void(SLeaf* leaf)> onEndWalk) const
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/geometry/BSP.cpp 365 -->
<table class="code-lines"><tr><td>

```cpp
365
```

</td><td>

```cpp
    void BSP::WalkTree(const SEntityPtr& entityPtr, const Vector3& pos, const std::function<void(SLeaf* leaf)>& onEndWalk) const
```

</td></tr></table>

- **なぜ:** 引数を値で受け取ると、呼ぶたびにコピーが作られる。`shared_ptr` のコピーは参照カウントを増減させ、`std::function` のコピーは関数オブジェクトの中身まで複製する。`const` 参照にすれば、コピーなしで読むだけにできる。
- **効果:** ノードに潜るたびに起きていた、コピーの時間がなくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `WalkTree` | 同じ座標のリーフを探すため | BSP の木をたどり、`pos` に近いリーフに着くたびに `onEndWalk` を呼ぶ | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp)（もとからある） |
| 引数 | `entityPtr` | どの要素から潜るかを受け取るため | 探し始める要素（最初はルートノード） | 呼ぶ側が決める |
| 引数 | `pos` | どの座標の近くを探すかを受け取るため | 探したい座標 | 呼ぶ側が決める |
| 引数 | `onEndWalk` | リーフに着いたときにしたいことを、呼ぶ側に決めさせるため | リーフに着くたびに呼ばれる関数。法線スムージングでは、同じ座標の頂点を見つけてまとめる処理を渡す | 呼ぶ側が決める |
| 型 | `std::function` | いろいろな関数を、同じ型の変数に入れて渡すため | 関数・ラムダ・メンバー関数を入れておける型。`<void(SLeaf* leaf)>` は「`SLeaf*` を受け取り、何も返さない関数」 | C++ の標準ライブラリ（`<functional>`。`k2EngineLowPreCompile.h` で読み込み済み） |
| 型 | `SLeaf` | BSP の末端（座標を1つ持つ要素）を表すため | 座標 `position` と、持ち主のデータ `extraData` を持つ構造体 | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |

### 3-2. `WalkTree()` の中を書き換える

[BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) の **388行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/k2EngineLow/geometry/BSP.cpp 388 -->
<table class="code-lines"><tr><td>

```cpp
388
```

</td><td>

```cpp
            for (auto leaf : leafList->leafList) {
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/geometry/BSP.cpp 388 -->
<table class="code-lines"><tr><td>

```cpp
388
```

</td><td>

```cpp
            for (const auto& leaf : leafList->leafList) {
```

</td></tr></table>

- **なぜ:** `for (auto leaf : ...)` は、要素を1つずつコピーする。`const auto&` にすれば、コピーせずに読むだけにできる。
- **効果:** リーフのリストの要素が、1つずつコピーされなくなる。

**初めて出てくる名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `leafList` | リーフのリストを持つ要素を扱うため | 要素を `SLeafList*` にしたもの | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp)（もとからある） |
| メンバー変数 | `leafList`（`SLeafList` の中） | リストの中のリーフを持つため | `SEntityPtr` の `std::vector`。末端にまとめて置かれたリーフが入る | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |
| 変数 | `leaf` | リストのリーフを1つずつ取り出すため | このループで取り出した、リーフ1つ（`const` 参照） | この行で作る |

### 3-3. `WalkTree()` を書き換える

[BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) の **393行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/k2EngineLow/geometry/BSP.cpp 393 -->
<table class="code-lines"><tr><td>

```cpp
393
```

</td><td>

```cpp
    void BSP::WalkTree(const Vector3& pos, std::function<void(SLeaf* leaf)> onEndWalk) const
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/geometry/BSP.cpp 393 -->
<table class="code-lines"><tr><td>

```cpp
393
```

</td><td>

```cpp
    void BSP::WalkTree(const Vector3& pos, const std::function<void(SLeaf* leaf)>& onEndWalk) const
```

</td></tr></table>

- **なぜ:** 3-1 の `WalkTree()` を呼ぶ側の `WalkTree(pos, onEndWalk)` も、同じ理由で `std::function` を `const` 参照で受け取る。
- **効果:** この `WalkTree()` を呼ぶときも、`std::function` がコピーされない。

### 3-4. 書き換える

[BSP.h](../../../k2EngineLow/geometry/BSP.h) の **92行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/k2EngineLow/geometry/BSP.h 92 -->
<table class="code-lines"><tr><td>

```cpp
92
```

</td><td>

```cpp
		void WalkTree(const Vector3& pos, std::function<void(SLeaf* leaf)> onEndWalk) const;
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/geometry/BSP.h 92 -->
<table class="code-lines"><tr><td>

```cpp
92
```

</td><td>

```cpp
		void WalkTree(const Vector3& pos, const std::function<void(SLeaf* leaf)>& onEndWalk) const;
```

</td></tr></table>

- **なぜ:** `.cpp` の関数の形を変えたら、`.h` の宣言も同じ形に変える。合っていないと、ビルドが通らない。
- **効果:** 3-3 の書き換えに、宣言が合う。

### 3-5. 書き換える

[BSP.h](../../../k2EngineLow/geometry/BSP.h) の **145行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/k2EngineLow/geometry/BSP.h 145 -->
<table class="code-lines"><tr><td>

```cpp
145
```

</td><td>

```cpp
		void WalkTree(SEntityPtr entity, const Vector3& pos, std::function<void(SLeaf* leaf)> onEndWalk ) const;
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/k2EngineLow/geometry/BSP.h 145 -->
<table class="code-lines"><tr><td>

```cpp
145
```

</td><td>

```cpp
		void WalkTree(const SEntityPtr& entity, const Vector3& pos, const std::function<void(SLeaf* leaf)>& onEndWalk) const;
```

</td></tr></table>

- **なぜ:** 3-4 と同じ。3-1 の `WalkTree()` の宣言も、`.cpp` の形に合わせる。
- **効果:** 3-1 の書き換えに、宣言が合う。

**確かめ方（これで出来上がり）:** ソリューションのビルドが通る。Debug ビルドで起動すると、モデルのロードが前より速く終わる。モデルの見た目（陰影）は変わらない。
