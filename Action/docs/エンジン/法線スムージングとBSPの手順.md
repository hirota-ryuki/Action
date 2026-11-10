# 法線スムージングとBSPの手順

設計: [法線スムージングとBSPの設計](法線スムージングとBSPの設計.md)
経緯: [起動時のモデルロード高速化の経緯](起動時のモデルロード高速化の経緯.md)

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
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) | 削除・書き換え | 1・2・3 |
| [BSP.h](../../k2EngineLow/geometry/BSP.h) | 削除・書き換え | 2・3 |

---

**もとからあって、変えないもの:** 法線スムージングは、頂点ごとに `WalkTree()` で同じ座標の頂点を探します（[TkmFile.cpp](../../k2EngineLow/tkFile/TkmFile.cpp) の `BuildTangentAndBiNormal()`）。頂点の数だけ `WalkTree()` が呼ばれるので、`WalkTree()` の中の無駄が、そのまま読み込みの時間になっていました。

---

## 手順1: 探索の中の、何もしないループを消す

`WalkTree()` は、ノードに来るたびに、そのノードが持つ全部のリーフを回すループに入っていました。中身が空なので何もしませんが、Debug ビルドでは最適化されずに毎回回ります。根に近いノードほどリーフが多いので、これがいちばん重い所でした。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) | 削除 |

### 1-1. `WalkTree()` の中の行を消す

[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) の **372〜374行目**を消します。

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

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: ノードが持っていた、使っていないリストを消す

ノード（`SNode`）は、自分の下の全部のリーフのリスト `leafArray` を持っていました。手順1のループでしか使っておらず、作るたびにリストをまるごとコピーしていたので、消します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) | 削除 |
| [BSP.h](../../k2EngineLow/geometry/BSP.h) | 削除 |

### 2-1. `CreateBSPTreeEntity()` の中の行を消す

[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) の **336行目**を消します。

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

### 2-2. 行を消す

[BSP.h](../../k2EngineLow/geometry/BSP.h) の **45行目**を消します。

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

**確かめ方:** ソリューションのビルドが通る。

---

## 手順3: 探索の引数を参照で渡す

`WalkTree()` は、ノードの `shared_ptr` と、見つけたときに呼ぶ `std::function` を値で受け取っていたので、潜るたびにコピーしていました（参照カウントの増減と、関数オブジェクトのコピー）。どちらも `const` の参照で受け取るようにします。リーフのリストを回すループの変数も、参照にします。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) | 書き換え |
| [BSP.h](../../k2EngineLow/geometry/BSP.h) | 書き換え |

### 3-1. `WalkTree()` を書き換える

[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) の **365行目**を、次のように書き換えます。

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

### 3-2. `WalkTree()` の中を書き換える

[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) の **388行目**を、次のように書き換えます。

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

### 3-3. `WalkTree()` を書き換える

[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) の **393行目**を、次のように書き換えます。

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

### 3-4. 書き換える

[BSP.h](../../k2EngineLow/geometry/BSP.h) の **92行目**を、次のように書き換えます。

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

### 3-5. 書き換える

[BSP.h](../../k2EngineLow/geometry/BSP.h) の **145行目**を、次のように書き換えます。

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

**確かめ方（これで出来上がり）:** ソリューションのビルドが通る。Debug ビルドで起動すると、モデルのロードが前より速く終わる。モデルの見た目（陰影）は変わらない。
