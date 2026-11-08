# 法線スムージングとBSPの手順

作成日: 2026-11-08
設計: [法線スムージングとBSPの設計](法線スムージングとBSPの設計.md)
経緯: [起動時のモデルロード高速化の経緯](起動時のモデルロード高速化の経緯.md)

tkm を読み込むときの法線スムージングで使う BSP の探索を、速くするまでの手順です。上から順にやると、[法線スムージングとBSPの設計](法線スムージングとBSPの設計.md) の形になります。

**出来上がるもの:** Debug ビルドで、モデルを読み込む時間（起動の時間）が短くなります。見た目は変わりません。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 始める前は、エンジンにもとからある形になっている。法線スムージング（`TkmFile::BuildTangentAndBiNormal()`）と、BSP の作り方（`BSP::CreateBSPTreeEntity()` など）は、もとからある（変えない）。変えるのは、BSP の探索（`WalkTree()`）と、ノードが持っていた使っていないリストだけ。
- ほかの手順書を先に済ませる必要はない。
- 作業を始める前に、main からブランチを切る。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | 探索の中の、何もしないループを消す | ビルドが通る |
| 2 | ノードが持っていた、使っていないリストを消す | ビルドが通る |
| 3 | 探索の引数を参照で渡す（毎回コピーしない） | 起動が速くなる |

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| [BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) | 削除（何もしないループ、リストのコピー）・書き換え（`WalkTree()` の引数・ループの変数） | 1・2・3 |
| [BSP.h](../../k2EngineLow/geometry/BSP.h) | 削除（`SNode::leafArray`）・書き換え（`WalkTree()` の宣言2つ） | 2・3 |

新しく作るファイル・コメントアウトする行はありません。

**もとからあって、変えないもの:** 法線スムージングは、頂点ごとに `WalkTree()` で同じ座標の頂点を探します（[TkmFile.cpp](../../k2EngineLow/tkFile/TkmFile.cpp) の `BuildTangentAndBiNormal()`）。頂点の数だけ `WalkTree()` が呼ばれるので、`WalkTree()` の中の無駄が、そのまま読み込みの時間になっていました。

---

## 手順1: 探索の中の、何もしないループを消す

`WalkTree()` は、ノードに来るたびに、そのノードが持つ全部のリーフを回すループに入っていました。中身が空なので何もしませんが、Debug ビルドでは最適化されずに毎回回ります。根に近いノードほどリーフが多いので、これがいちばん重い所でした。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) | 削除（`WalkTree()` の中の空の `for` 3行） |

[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) の `BSP::WalkTree()`（ノードを受け取る方）で、空の `for` を**削除**します。

<!-- file: ../../k2EngineLow/geometry/BSP.cpp -->

```diff
             // 左に潜る？右に潜る？
             SNode* node = static_cast<SNode*>(entityPtr.get());
-            for (SEntityPtr& entity : node->leafArray) {
-                 
-            }
             float t = Dot(pos, node->plane.normal);
```

**確かめ方:** ソリューションのビルドが通る。

---

## 手順2: ノードが持っていた、使っていないリストを消す

ノード（`SNode`）は、自分の下の全部のリーフのリスト `leafArray` を持っていました。手順1のループでしか使っておらず、作るたびにリストをまるごとコピーしていたので、消します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.h](../../k2EngineLow/geometry/BSP.h) | 削除（`SNode` のメンバー `leafArray`） |
| [BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) | 削除（`CreateBSPTreeEntity()` でリストをコピーする1行） |

[BSP.h](../../k2EngineLow/geometry/BSP.h) の `SNode` から**削除**します。

<!-- file: ../../k2EngineLow/geometry/BSP.h -->

```diff
 			SEntityPtr rightEntity;		// 右の要素
 			SEntityPtr leftEntity;		// 左の要素。
 			Vector3 centerPos;			// 中心座標。
-			std::vector<SEntityPtr> leafArray;	// ノードが内包しているリーフ。
 		};
 		/// <summary>
 		/// リーフ
```

[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) の `CreateBSPTreeEntity()` から**削除**します。

<!-- file: ../../k2EngineLow/geometry/BSP.cpp -->

```diff
         auto newNodePtr = std::make_shared<SNode>();
         newNodePtr->type = enEntityType_Node;
         newNodePtr->centerPos = centerPos;
-        newNodePtr->leafArray = leafArray;
         auto newNode = static_cast<SNode*>(newNodePtr.get());
 
         // 分散しているので、共分散行列を利用して
```

**確かめ方:** ソリューションのビルドが通る。

---

## 手順3: 探索の引数を参照で渡す

`WalkTree()` は、ノードの `shared_ptr` と、見つけたときに呼ぶ `std::function` を値で受け取っていたので、潜るたびにコピーしていました（参照カウントの増減と、関数オブジェクトのコピー）。どちらも `const` の参照で受け取るようにします。リーフのリストを回すループの変数も、参照にします。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [BSP.h](../../k2EngineLow/geometry/BSP.h) | 書き換え（`WalkTree()` の宣言2つの引数） |
| [BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) | 書き換え（`WalkTree()` の定義2つの引数、ループの変数） |

[BSP.h](../../k2EngineLow/geometry/BSP.h) の宣言2つを**書き換え**ます。

<!-- file: ../../k2EngineLow/geometry/BSP.h -->

```diff
 		/// </remark>
 		/// <param name="pos">座標</param>
 		/// <param name="onEndWalk">探索が終了した時に呼ばれる処理</param>
-		void WalkTree(const Vector3& pos, std::function<void(SLeaf* leaf)> onEndWalk) const;
+		void WalkTree(const Vector3& pos, const std::function<void(SLeaf* leaf)>& onEndWalk) const;
 	private:
 		/// <summary>
 		/// 平面でリーフノードを分割していく
```

<!-- file: ../../k2EngineLow/geometry/BSP.h -->

```diff
 		/// <returns></returns>
 		SEntityPtr CreateBSPTreeEntity_LeafList(const std::vector<SEntityPtr>& leafArray);
 	
-		void WalkTree(SEntityPtr entity, const Vector3& pos, std::function<void(SLeaf* leaf)> onEndWalk ) const;
+		void WalkTree(const SEntityPtr& entity, const Vector3& pos, const std::function<void(SLeaf* leaf)>& onEndWalk) const;
 	};
 }
```

[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp) の定義2つと、ループの変数を**書き換え**ます。まず、ノードを受け取る方の `WalkTree()` の1行目です。

<!-- file: ../../k2EngineLow/geometry/BSP.cpp -->

```diff
-    void BSP::WalkTree(SEntityPtr entityPtr, const Vector3& pos, std::function<void(SLeaf* leaf)> onEndWalk) const
+    void BSP::WalkTree(const SEntityPtr& entityPtr, const Vector3& pos, const std::function<void(SLeaf* leaf)>& onEndWalk) const
     {
         if (entityPtr->type == enEntityType_Node) {
```

次に、リーフのリストを回すループの変数と、位置だけを受け取る方の `WalkTree()` です。

<!-- file: ../../k2EngineLow/geometry/BSP.cpp -->

```diff
         else if (entityPtr->type == enEntityType_LeafList) {
             // リーフのリストに到達した。
             auto leafList = static_cast<SLeafList*>(entityPtr.get());
-            for (auto leaf : leafList->leafList) {
+            for (const auto& leaf : leafList->leafList) {
                 onEndWalk(static_cast<SLeaf*>(leaf.get()));
             }
         }
     }
-    void BSP::WalkTree(const Vector3& pos, std::function<void(SLeaf* leaf)> onEndWalk) const
+    void BSP::WalkTree(const Vector3& pos, const std::function<void(SLeaf* leaf)>& onEndWalk) const
     {
         WalkTree(m_rootNode, pos, onEndWalk);
     }
```

**確かめ方（これで出来上がり）:** ソリューションのビルドが通る。Debug ビルドで起動すると、モデルのロードが前より速く終わる。モデルの見た目（陰影）は変わらない。
