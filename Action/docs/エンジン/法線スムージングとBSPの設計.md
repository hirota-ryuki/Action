# 法線スムージングとBSPの設計

作成日: 2026-11-07（モデル読み込みの設計から分けた）
対象: [TkmFile.cpp](../../k2EngineLow/tkFile/TkmFile.cpp) の法線の計算、[BSP.cpp](../../k2EngineLow/geometry/BSP.cpp)
経緯: [起動時のモデルロード高速化の経緯](起動時のモデルロード高速化の経緯.md)
手順: [法線スムージングとBSPの手順](法線スムージングとBSPの手順.md)（BSP の探索を速くするまで）

tkm を読み込むときに法線を計算し直し、同じ座標の頂点どうしで平均して滑らかにする仕組みと、そのための BSP（二分空間分割）のページです。読み込み全体は [モデル読み込みの設計](モデル読み込みの設計.md) にあります。

- **ファイルの法線は使わず、読み込むたびに計算し直す。**
- **同じ座標の頂点は BSP で探す。** 探索が速いことが、起動時間の前提になっている。

---

## 1. 法線スムージング

`TkmFile::Load()` は、ファイルに入っている法線を捨てて（`vertex.normal = g_vec3Zero`）計算し直す。

| 手順 | 内容 | 関数 |
|---|---|---|
| 1. 面の法線を足し込む | 三角形ごとの面の法線を、その3頂点に足して正規化する。インデックスで共有されている頂点は周りの面の平均になる | `NormalSmoothing::Execute()` |
| 2. 同じ座標の頂点どうしで平均する | 座標がまったく同じで、向きが近い（内積が正の）頂点の法線を足して平均する。UV の継ぎ目で分かれた頂点も滑らかにつながる。`isFlatShading == false` のときだけ | `BuildTangentAndBiNormal()` |

---

## 2. BSP

点の集まりを平面で2つに分けることを繰り返し、木にしたものです。「この座標にある頂点はどれか」を、全部の頂点を調べずに探せます。

エンジン内での使い道は次の2つです。

- `TkmFile`: 法線スムージングで、同じ座標の頂点を探す（1章）
- `NaviMesh`: セルの中心座標から近いセルを探す（[NaviMesh.cpp](../../k2EngineLow/AI/PathFinding/NaviMesh.cpp)）

### 木の要素

| 要素 | 持っているもの |
|---|---|
| `SNode` | 分割平面、左右の子、中心座標 |
| `SLeaf` | 座標と、付加データ（TkmFile では法線へのポインタ） |
| `SLeafList` | ほぼ同じ場所にある `SLeaf` の配列（木の末端） |

---

## 3. 実装方法

### 法線スムージング：頂点ごとに BSP で同じ座標を探す

```cpp
// TkmFile::BuildTangentAndBiNormal()
auto smoothFunc = [&](int startIndex, int endIndex) {
    for (int i = startIndex; i < endIndex; i++) {
        auto& va = smoothVertex[i];
        m_bpsOnVertexPosition.WalkTree(va.vertex->pos, [&](BSP::SLeaf* leaf) {
            if (va.vertex->pos.x == leaf->position.x && ...) {          // 座標がまったく同じ
                auto* normal = static_cast<Vector3*>(leaf->extraData);
                if (va.vertex->normal.Dot(*normal) > 0.0f) {            // 向きが近い
                    va.newNormal += *normal;
                }
            }
        });
    }
};
// 頂点を4等分して4スレッドで smoothFunc を実行し、join してから正規化して書き戻す
```

各スレッドは自分の担当の `newNormal` にだけ書き、元の法線は読むだけなので、ロックなしで並列にできます。

### BSP の構築：一番散らばっている方向で2つに分けることを繰り返す

```cpp
BSP::SEntityPtr BSP::CreateBSPTreeEntity(const std::vector<SEntityPtr>& leafArray)
{
    if (leafArray.size() == 1) { return leafArray.front(); }
    Vector3 centerPos = CalcCenterPositionFromLeafList(leafArray);
    float covarianceMatrix[3][3];
    CalcCovarianceMatrixFromLeafNodeList(covarianceMatrix, leafArray, centerPos);   // 共分散行列
    if (どの行も長さ 0.1 未満) {
        return CreateBSPTreeEntity_LeafList(leafArray);   // ほぼ同じ場所の点だけ → 末端
    }
    CalcSplitPlaneFromCovarianceMatrix(newNode->plane, ...);   // ヤコビ法で固有ベクトル → 分割平面
    SplitLeafArray(leftLeafArray, rightLeafArray, newNode->plane, leafArray);
    newNode->leftEntity = CreateBSPTreeEntity(leftLeafArray);   // 左右それぞれで繰り返す
    newNode->rightEntity = CreateBSPTreeEntity(rightLeafArray);
    return newNodePtr;
}
```

片方が空になったときも末端にして、再帰が終わらなくなるのを防いでいます。

### BSP の探索：平面のどちら側かで片方の子だけに進む

```cpp
void BSP::WalkTree(const SEntityPtr& entityPtr, const Vector3& pos,
                   const std::function<void(SLeaf* leaf)>& onEndWalk) const   // 引数は const 参照
{
    if (entityPtr->type == enEntityType_Node) {
        SNode* node = static_cast<SNode*>(entityPtr.get());
        if (Dot(pos, node->plane.normal) < node->plane.distance) {
            WalkTree(node->leftEntity, pos, onEndWalk);
        } else {
            WalkTree(node->rightEntity, pos, onEndWalk);
        }
    }
    else if (entityPtr->type == enEntityType_Leaf) { onEndWalk(葉); }
    else if (entityPtr->type == enEntityType_LeafList) { for (葉 : 末端の配列) onEndWalk(葉); }
}
```

1回の探索で通るノードは木の深さぶん（12万点で約17個）なので、全頂点を探索しても **O(N log N)** で済みます。

---

## 4. 使い方（BSP を書き換えるとき）

- `WalkTree()` は頂点の数だけ呼ばれます。**探索の中に、ノードが持つ点の数に比例する処理を入れないでください。** 全体が O(N²) になります。
- `WalkTree()` は再帰関数なので、引数（`shared_ptr` と `std::function`）は const 参照で受け取っています。値渡しに戻すと、ノードを1つ下りるたびにコピーが発生します。
- `toolsProj/nvmExporter/nvmExporter/bsp/` に同じ BSP の古いコピーがあります。こちらは修正前のままです（空ループが残っています）。
