# 法線スムージングとBSPの設計

作成日: 2026-11-07（モデル読み込みの設計から分けた）
対象: [TkmFile.cpp](../../../k2EngineLow/tkFile/TkmFile.cpp) の法線の計算、[BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp)
経緯: [起動時のモデルロード高速化の経緯](../../最適化/起動/起動時のモデルロード高速化/起動時のモデルロード高速化の経緯.md)
関連: [法線のキャッシュの設計](../../最適化/起動/法線のキャッシュ/法線のキャッシュの設計.md)（計算した結果を次の起動で使い回す）
手順: [法線スムージングとBSPの手順](法線スムージングとBSPの手順.md)（BSP の探索を速くするまで）

tkm を読み込むときに法線を計算し直し、同じ座標の頂点どうしで平均して滑らかにする仕組みと、そのための BSP（二分空間分割）のページです。読み込み全体は [モデル読み込みの設計](../モデル読み込み/モデル読み込みの設計.md) にあります。

- **ファイルの法線は使わず、計算し直す。** 計算した結果は `.tkmcache` に書いておき、2回目の起動からはそれを読む（[法線のキャッシュの設計](../../最適化/起動/法線のキャッシュ/法線のキャッシュの設計.md)）。
- **同じ座標の頂点は BSP で探す。** 探索が速いことが、起動時間の前提になっている。

**用語:**

| 用語 | 何のことか |
|---|---|
| tkm | k2Engine のモデルのファイル（`.tkm`）。頂点・面・マテリアルが入っている |
| 法線 | 面や頂点が向いている方向（長さ 1 の `Vector3`）。光の当たり方の計算に使う |
| `.tkmcache` | このプロジェクトで足した、計算し直した法線などを保存しておくファイル。`.tkm` の隣に作る |
| BSP | Binary Space Partitioning（二分空間分割）。空間を平面で2つに分けることを繰り返して木にし、点を速く探せるようにする方法 |

---

## 1. 法線スムージング

`TkmFile::Load()` は、ファイルに入っている法線を捨てて（`vertex.normal = g_vec3Zero`）計算し直す。

| 手順 | 内容 | 関数 |
|---|---|---|
| 1. 面の法線を足し込む | 三角形ごとの面の法線を、その3頂点に足して正規化する。インデックスで共有されている頂点は周りの面の平均になる | `NormalSmoothing::Execute()` |
| 2. 同じ座標の頂点どうしで平均する | 座標がまったく同じで、向きが近い（内積が正の）頂点の法線を足して平均する。UV の継ぎ目で分かれた頂点も滑らかにつながる。`isFlatShading == false` のときだけ | `BuildTangentAndBiNormal()` |

**用語:**

| 用語 | 何のことか |
|---|---|
| 正規化 | ベクトルの向きはそのままで、長さを 1 にすること（`Vector3::Normalize()`） |
| インデックス | 三角形が「何番目の頂点を使うか」を並べた番号の配列。同じ頂点を、いくつもの三角形で共有できる |
| UV の継ぎ目 | テクスチャの貼り方（UV 座標）が切り替わる所。同じ座標でも UV が違うので、頂点が2つに分かれて入っている |
| フラットシェーディング | 法線を平均せず、面ごとにかくかくした陰影にする描き方 |

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| メンバー変数 | `isFlatShading` | 法線を平均するかを決めるため | 0 以外ならフラットシェーディング（平均しない）。tkm のファイルの頭に入っている | k2EngineLow（エンジン）の [TkmFile.cpp](../../../k2EngineLow/tkFile/TkmFile.cpp) の `SHeader` |

---

## 2. BSP

点の集まりを平面で2つに分けることを繰り返し、木にしたものです。「この座標にある頂点はどれか」を、全部の頂点を調べずに探せます。

エンジン内での使い道は次の1つです。

- `TkmFile`: 法線スムージングで、同じ座標の頂点を探す（1章）

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

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `TkmFile::BuildTangentAndBiNormal()` | 法線を同じ座標どうしで平均するため | 頂点の法線を、同じ座標の頂点どうしで平均して書き戻す | k2EngineLow（エンジン）の [TkmFile.cpp](../../../k2EngineLow/tkFile/TkmFile.cpp) |
| 変数 | `smoothVertex` | スレッドごとに、書き込む先を分けるため | 頂点ごとの「新しい法線」と「頂点へのポインター」の組（`SSmoothVertex`）の配列 | k2EngineLow（エンジン）の `TkmFile.cpp`（`SSmoothVertex` は同じファイルの名前のない `namespace` にある） |
| メンバー変数 | `m_bpsOnVertexPosition` | 同じ座標の頂点を速く探すため | 全部の頂点の座標を入れた BSP の木 | k2EngineLow（エンジン）の [TkmFile.h](../../../k2EngineLow/tkFile/TkmFile.h) |
| 関数 | `WalkTree()` | 木を全部たどらずに、近い葉だけを調べるため | 座標が入る側の子だけを下り、着いた葉ごとに、渡した関数を呼ぶ | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |
| 型 | `BSP::SLeaf` | 木の末端の1点を表すため | 座標（`position`）と、付けておくデータ（`extraData`）を持つ | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |
| メンバー変数 | `extraData` | 葉に、何でも付けておけるようにするため | 型を決めないポインター（`void*`）。TkmFile では法線を指すので、`static_cast<Vector3*>` で戻して使う | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |
| 関数 | `Dot()`（`Vector3` のメンバー） | 2つの向きが近いかを調べるため | 内積を返す。正なら、2つの向きのなす角は 90度より小さい | k2EngineLow（エンジン）の [Vector.h](../../../k2EngineLow/math/Vector.h) |
| メンバー変数 | `newNormal` | 平均した法線を、元の法線と分けてためるため | 同じ座標の頂点の法線を足していく先。最後に正規化して書き戻す | k2EngineLow（エンジン）の `TkmFile.cpp` の `SSmoothVertex` |

**用語:**

| 用語 | 何のことか |
|---|---|
| スレッド | 1つのプログラムの中で、同時に進む処理の流れ。ここでは C++ の標準ライブラリ（`<thread>`）の `std::thread` を4つ作り、頂点を4つに分けて同時に処理する |
| join | スレッドが終わるまで待つこと（`std::thread::join()`） |
| ロック | いくつものスレッドが同じデータを同時に書き換えないよう、順番待ちさせる仕組み。ここでは書く場所が分かれているので使わない |

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

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `BSP::SEntityPtr` | 木の要素（ノード・葉・末端の配列）を、同じ型で持つため | `std::shared_ptr<SEntity>` の別名 | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |
| 型 | `std::shared_ptr` | いくつもの場所から同じものを指し、誰も指さなくなったら消すため | 指している数を数えるスマートポインター。0 になると中身を `delete` する | C++ の標準ライブラリ（`<memory>`。[k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h) で読み込み済み） |
| 関数 | `front()` | 配列の最初の要素を取り出すため | `std::vector` の先頭の要素を返す | C++ の標準ライブラリ（`std::vector` のメンバー） |
| 関数 | `CreateBSPTreeEntity()` | 点の集まりから木を作るため | 点を2つに分け、左右それぞれで自分をまた呼ぶ（再帰）。1点になったら、その葉を返す | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) |
| 関数 | `CalcCenterPositionFromLeafList()` | 分ける平面を、点の集まりの真ん中に置くため | 点の座標の平均を返す | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) |
| 関数 | `CalcCovarianceMatrixFromLeafNodeList()` | 点がどの向きに散らばっているかを調べるため | 共分散行列（3×3）を作る | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) |
| 関数 | `CreateBSPTreeEntity_LeafList()` | ほぼ同じ場所の点を、それ以上分けずにまとめるため | 点をまとめて1つの末端（`SLeafList`）にする | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) |
| 関数 | `CalcSplitPlaneFromCovarianceMatrix()` | 点の集まりを、いちばん散らばっている向きで分ける平面を決めるため | ヤコビ法（`EigenJacobiMethod()`）で固有ベクトルを求め、固有値がいちばん大きいものを平面の法線にする | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) |
| 関数 | `SplitLeafArray()` | 点を、平面の表側と裏側に分けるため | 平面のどちら側にあるかで、左の配列と右の配列に分ける | k2EngineLow（エンジン）の [BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) |
| メンバー変数 | `plane` | ノードが点を分ける平面を覚えるため | 法線（`normal`）と、原点からの距離（`distance`）を持つ `SPlane` | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) の `SNode` |
| メンバー変数 | `leftEntity`・`rightEntity` | 平面の裏側・表側の点を、子として持つため | 左の子・右の子（どちらも `SEntityPtr`） | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) の `SNode` |

**用語:**

| 用語 | 何のことか |
|---|---|
| 共分散行列 | 点の集まりが、x・y・z のどの向きに、どれだけ散らばっているかを表す 3×3 の行列 |
| 固有ベクトル・固有値 | 行列をかけても向きが変わらないベクトルが固有ベクトル、そのときに長さが何倍になるかが固有値。共分散行列では、固有値がいちばん大きい固有ベクトルが「点がいちばん散らばっている向き」になる |
| ヤコビ法 | 固有値と固有ベクトルを求める計算の方法の1つ（下の「ヤコビ法の流れ」） |
| 再帰 | 関数の中で、同じ関数を呼ぶこと。ここでは、分けた左右それぞれで、また分けることを繰り返す |

**ヤコビ法の流れ**（[BSP.cpp](../../../k2EngineLow/geometry/BSP.cpp) の `EigenJacobiMethod()`。`#if 1` の側が使われている）

1. 固有ベクトルを入れる行列を、単位行列（対角線が 1、ほかは 0）にしておく。
2. 共分散行列の、対角線の外でいちばん大きい値（i 行 j 列）を探す。`1.0e-10` 以下なら終わる。
3. i 軸と j 軸だけを回す回転を、その値がちょうど 0 になる角度で、共分散行列の両側からかける。同じ回転を、固有ベクトルの行列にもかける。
4. 2 に戻る（最大 100 回）。

終わると、共分散行列の対角線に固有値が並び、固有ベクトルの行列の各列が、それぞれの固有値の固有ベクトルになります。回転はほかの対角線の外の値も少し変えますが、繰り返すと全部 0 に近づきます。

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

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 型 | `std::function` | 「葉に着いたら何をするか」を、呼ぶ側が渡せるようにするため | 関数やラムダ式を入れておける型。`<void(SLeaf* leaf)>` は「`SLeaf*` を受け取り、何も返さない」の意味で、ここには `BuildTangentAndBiNormal()` の「同じ座標なら法線を足す」ラムダ式が入る | C++ の標準ライブラリ（`<functional>`。[k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h) で読み込み済み） |
| 値 | `enEntityType_Node`・`enEntityType_Leaf`・`enEntityType_LeafList` | 要素がノード・葉・末端の配列のどれかを見分けるため | 要素の種類の番号。`SEntity` の `type` に入っている | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |
| 型 | `SNode` | 木の途中の、点を分ける所を表すため | 分割平面と、左右の子と、中心の座標を持つ | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) |
| 関数 | `get()` | `std::shared_ptr` が指しているものを、ふつうのポインターで取り出すため | 中身へのポインターを返す（数えている数は変えない） | C++ の標準ライブラリ（`std::shared_ptr` のメンバー） |
| 関数 | `Dot()`（引数が2つ） | 点が平面の表側か裏側かを調べるため | 2つの `Vector3` の内積を返す。点と法線の内積が `distance` より小さければ裏側 | k2EngineLow（エンジン）の [Vector.h](../../../k2EngineLow/math/Vector.h)（`nsK2EngineLow` の関数） |
| メンバー変数 | `normal`・`distance` | 平面を2つの値で表すため | 平面の法線と、原点から平面までの距離 | k2EngineLow（エンジン）の [BSP.h](../../../k2EngineLow/geometry/BSP.h) の `SPlane` |

1回の探索で通るノードは木の深さぶん（12万点で約17個）なので、全頂点を探索しても **O(N log N)** で済みます。

**用語:**

| 用語 | 何のことか |
|---|---|
| O(N log N)・O(N²) | 点の数 N が増えたときに、計算の回数がどれくらい増えるかの目安。O(N log N) は「N × 木の深さ」くらい、O(N²) は「N × N」くらいで、N が大きいとけた違いに遅い |

---

## 4. 使い方（BSP を書き換えるとき）

- `WalkTree()` は頂点の数だけ呼ばれます。**探索の中に、ノードが持つ点の数に比例する処理を入れないでください。** 全体が O(N²) になります。
- `WalkTree()` は再帰関数なので、引数（`shared_ptr` と `std::function`）は const 参照で受け取っています。値渡しに戻すと、ノードを1つ下りるたびにコピーが発生します。
- `toolsProj/nvmExporter/nvmExporter/bsp/` に同じ BSP の古いコピーがあります。こちらは修正前のままです（空ループが残っています）。
