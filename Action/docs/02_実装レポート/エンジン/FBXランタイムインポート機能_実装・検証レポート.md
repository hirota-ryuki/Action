# FBXランタイムインポート機能 実装・検証レポート

作成日: 2026-07-08
対象リポジトリ: `d:\02_git\01_my\01_Action`(DirectX12自作エンジン `k2Engine` / `k2EngineLow`)

このドキュメントは、別セッション・別PCのClaudeが読んでも経緯と現状を追えるように、
背景・設計判断・実装内容・検証結果・既知の課題を独立した記録としてまとめたものです。

---

## 1. 背景・要望

- 元の要望: 「モデルレンダーのInitでFBXを指定したら、指定したFBXと同じフォルダにあるテクスチャの画像を自動で割り当ててモデルをInitできるようにしたい」
- 既存のパイプラインは、3ds Max上でFBXを独自バイナリ `.tkm`(メッシュ+マテリアル)/`.tks`(スケルトン)/`.tka`(アニメーション)にオフライン変換してから使う方式で、テクスチャは `.tkm` と同じフォルダにある `<拡張子なしファイル名>.dds` を探して読み込む仕組みだった(`Action\k2EngineLow\tkFile\TkmFile.cpp` の `BuildMaterial`)。
- 今回、これとは別に **`ModelRender::Init()` に直接 `.fbx` パスを渡すと、実行時にFBXを解析して `.tkm` を自動生成し、FBXと同じフォルダ(または `Textures` サブフォルダ)にあるテクスチャを自動で割り当てる**、新しいランタイムインポート機能を追加した。

### 技術選定(ユーザーとの合意事項)

1. FBXパーサーは Autodesk FBX SDK ではなく、MITライセンスの単一ヘッダ+実装ファイルC/C++ライブラリ **[ufbx](https://github.com/ufbx/ufbx)** を採用し、`Action\k2EngineLow\ExEngine\ufbx\` にベンダリング(ImGuiと同じ方式)。
   - 理由: Autodesk FBX SDKは未インストールで、ダウンロードにEULA同意が必要なため自動化できない。
2. テクスチャの紐付けは命名サフィックス規約に頼らない。FBXマテリアルが参照しているファイル名をFBXと同じフォルダ内で探すことを優先し、見つからなければフォルダ内の画像ファイルをフォールバックとして使う。
3. テクスチャは `<fbxのフォルダ>\Textures\` サブフォルダを優先的に探し、無ければfbx自体のフォルダを探す(ユーザーからの追加要望)。
4. 変換は「実行時にTKM変換して」既存の `.tkm` ロードパイプラインに乗せる方式(3dsMax等の外部ツールを都度起動する方式ではない)。

### スコープ(フェーズ1)

**静的メッシュ+マテリアル+テクスチャの自動割り当てのみ対応。スキン(ボーン変形)/スケルトン/アニメーションのFBXからの変換は非対応。**

根拠:
- `ModelRender::InitSkeleton()`(`Action\k2Engine\graphics\ModelRender.cpp`)は `.tkm` を `.tks` に文字列置換して `Skeleton::Init()` を呼ぶだけで、戻り値を一切チェックしない。`.tks` が存在しなくてもエラーにならず、静かにスケルトン無しとして処理が続行される。
- `MeshParts.cpp` は `skinWeights.x > 0.0f` で頂点単位にスキン有無を自動判定するため、スキンウェイトを全て0で書き込めば非スキンモデルとして正しく扱われる。
- スキン/アニメーションの完全変換(ボーン階層・バインドポーズ・アニメーションカーブの内部表現への精密変換)は独立した大きな作業であり、今回のスコープ外とした。
- 既存の `.tks`/`.tka`(3dsMax由来)は、fbxと同名で手動配置すれば今まで通り読み込まれ、共存可能。

---

## 2. 実装したファイル

### 新規追加

| ファイル | 役割 |
|---|---|
| `Action\k2EngineLow\ExEngine\ufbx\ufbx.h` / `ufbx.c` | ufbx本体(GitHub `ufbx/ufbx` からベンダリング、MITライセンス) |
| `Action\k2EngineLow\ExEngine\ufbx\LICENSE.txt` | ufbxのライセンス表記 |
| `Action\k2EngineLow\tkFile\FbxRuntimeImporter.h` | FBX→tkmランタイム変換クラスの宣言 |
| `Action\k2EngineLow\tkFile\FbxRuntimeImporter.cpp` | 変換ロジック本体(下記3章で詳述) |

### 変更(既存ファイルへの最小限の追記)

| ファイル | 変更内容 |
|---|---|
| `Action\k2EngineLow\tkFile\TkmFile.h` | `TkmFile::SetMeshPartsForRuntimeImport(std::vector<SMesh>&&)` を追加。`Load()`を介さずランタイム構築したメッシュデータを`Save()`できるようにする最小限のセッター。 |
| `Action\k2Engine\graphics\ModelRender.cpp` | `Init()` と `IniTranslucent()` の先頭に2行追加: `filePath` が `.fbx` の場合、`FbxRuntimeImporter::ResolveToTkmFilePath()` で変換後の `.tkm` パスに差し替えてから、以降の処理(既存のまま)に流す。`.tkm` が渡された場合は即座に同じ文字列を返すだけで副作用・性能劣化なし。`InitForwardRendering()` はこの自動変換の対象外(コメントで明記)。 |
| `Action\k2EngineLow\k2EngineLow.vcxproj` / `.vcxproj.filters` | 上記新規ファイルを登録。`ufbx.c` はサードパーティコード扱いで、Debug/Release/Preview x64の3構成で `PrecompiledHeader` を `NotUsing` にオーバーライド(ImGuiと同じパターン)。 |

`Action\k2EngineLow\graphics\MeshParts.cpp` は検証中に一時的なデバッグログを追加したが、**最終的に完全に元の内容へ戻した**(空白・改行コードの差異のみ残り、機能的な変更は無い)。

---

## 3. `FbxRuntimeImporter` の設計詳細

公開インターフェース(`FbxRuntimeImporter.h`):

```cpp
namespace nsK2EngineLow {
    using MaterialTextureTable = std::map<std::string, std::map<std::string, std::string>>;

    class FbxRuntimeImporter : public Noncopyable {
    public:
        static std::string ResolveToTkmFilePath(const char* filePath, bool isOutputErrorCodeTTY = false);
        // ...private: 変換の各段階を担うstaticメソッド群
    };
}
```

### 処理フロー

1. `ResolveToTkmFilePath(filePath)`
   - 拡張子が `.fbx` でなければ、何もせず `filePath` をそのまま返す(既存呼び出し元への副作用ゼロ)。
   - `.fbx` の場合、同じフォルダの同名 `.tkm` パスを算出。
   - `.tkm` が存在せず、または `.fbx` の方が新しければ `ConvertFbxToTkm()` を実行してから、`.tkm` パスを返す。
2. `ConvertFbxToTkm()`
   - `ufbx_load_file()` でFBXシーンをロード。
   - `LoadMaterialTextureTable()` で、fbxと同じフォルダにある `MaterialTextureTable.txt`(存在すれば)を読み込む(後述)。
   - `BuildMeshParts()` でメッシュ・マテリアル・テクスチャを `TkmFile::SMesh` 形式に変換。
   - `TkmFile::SetMeshPartsForRuntimeImport()` → `TkmFile::Save()` で `.tkm` を書き出す。
3. `BuildMeshParts()`
   - `scene->meshes` を走査。各 `ufbx_mesh` について、ノードの `geometry_to_world` 行列を頂点にベイクする(フェーズ1は単一静的インスタンス想定。複数インスタンスがあれば警告を出し最初の1つのみ変換)。
   - `mesh->materials`(マテリアルごとの面グループ、`mesh->material_parts` と対応)ごとに `ufbx_triangulate_face()` で三角形化し、位置・法線・UVを取得して `TkmFile::SVertex` を構築。スキンウェイトは全て0。
   - **安全対策**: `material_parts.count` が `materials.count` を下回るケースに備え `numMaterialSlots` をクランプ。1枚もポリゴンが割り当たっていないマテリアルスロット、および結果的に頂点0のメッシュは `TkmFile::SMesh` に含めない(理由は5章のバグ3参照)。
4. `BuildMaterial()`(テクスチャ解決の優先順位)
   1. ufbxの標準プロパティ(`pbr.base_color` / `fbx.diffuse_color` など)
   2. 上記で見つからない場合、`material->textures` からシェーダー固有プロパティ名(`gltf_tex_diffuse` 等、"diffuse"/"basecolor"/"albedo" を含むもの)を走査
   3. それでも見つからない場合、`MaterialTextureTable.txt`(後述)から完全一致キー→部分一致キーの順で検索
   4. 最終フォールバックとして、`Textures` サブフォルダ→fbx自体のフォルダの順にある画像ファイルを1枚だけ拾ってアルベドとして採用(複数あれば警告を出しつつ最初の1つ)
5. テクスチャ実ファイルの検索先は `GetTextureSearchDirs()` が返す `{ <fbxDir>/Textures, <fbxDir> }` の順(`Textures` サブフォルダ優先)。
6. 見つかった画像が `.dds` でなければ `texconv.exe` を `CreateProcessA` で起動してDDSに変換(オプションは `Action\tools\3dsMaxScripts\makefile` の `-vflip -f BC7_UNORM -srgb -y` と同一)。変換先は生成される `.tkm` と同じフォルダ(`TkmFile::BuildMaterial` の仕様上、tkmと同じフォルダに `<拡張子なし>.dds` が必要なため)。

### `MaterialTextureTable.txt` フォールバックについて

検証に使った実アセット `SK_Player.FBX` は、mtoon/glTF系のシェーダー固有プロパティ(`gltf_tex_diffuse`, `mtoon_tex_Normal` 等)でテクスチャを保持しており、これらは **ufbxの標準的なテクスチャ接続の走査(`material->textures`)では検出できなかった**(検証ログで全マテリアルが `numTextures=0` と判明。詳細は5章)。
このアセットには元々3ds Max作業用に生成された `MaterialTextureTable.txt`(マテリアル名→プロパティ名→テクスチャファイル名の対応表、プレーンテキスト)が同じフォルダに置かれていたため、これをフォールバックとしてパースし活用する仕組みを追加した(`LoadMaterialTextureTable()` / `FindTextureNameInTable()`)。
このファイルが無いFBXでは単に空のテーブルとして扱われ、他のフォールバック(2階層目・4階層目)にそのまま進む。

---

## 4. 使い方

```cpp
// 従来: tkmファイルを指定
m_modelRender.Init("Assets/modelData/xxx/model.tkm");

// 新機能: fbxファイルを直接指定
m_modelRender.Init("Assets/modelData/xxx/model.fbx");
```

- `IniTranslucent()` も同様に対応。
- テクスチャは **`model.fbx` と同じフォルダの `Textures` サブフォルダ** に置くことを推奨(`Assets/modelData/xxx/Textures/*.png` 等)。無ければfbx自体のフォルダも探す。
- 初回Init時にFBXを解析し、同じフォルダに `<fbx名>.tkm` と、変換したテクスチャの `.dds` を自動生成する(裏で `texconv.exe` を呼び出す)。2回目以降は `.tkm` が `.fbx` より新しければ変換をスキップして高速にロードする。FBXを更新すれば自動的に再変換される。
- **静的メッシュのみ対応**。スキン(ボーン変形)やアニメーションはFBXから変換されない。アニメーション付きで使いたい場合は、これまで通り3ds Max側で `.tks`/`.tka` を作成し、FBXと同じ場所に配置しておけば、既存の仕組みがそれらを読み込む(この機能とは独立)。
- `InitForwardRendering()` 経由ではこの自動変換は効かない(`Init()`/`IniTranslucent()` 限定)。

---

## 5. 実機検証(`SK_Player.FBX` を使用)

### 検証対象アセット

`Action\Game\Assets\modelData\player\Model\SK_Player.FBX`(18マテリアルスロット、約124,521頂点、mtoon/glTF系シェーダープロパティを使用)。同フォルダに `MaterialTextureTable.txt`(全35テクスチャの対応表)と `Textures\` サブフォルダ(T__01.png〜T__21.png 等、実質的な使用テクスチャは20枚程度)が用意されていた。

このアセットを一時的に `Action\Game\Source\Objects\Player.cpp` の `Player::Start()` から

```cpp
m_modelRender.Init("Assets/modelData/player/Model/SK_Player.FBX", nullptr, 0, enModelUpAxisZ);
```

として呼び出し、実際にビルド・起動してスクリーンショットで見た目を確認しながら反復デバッグした。

### 発見して修正したバグ(4件、すべて `FbxRuntimeImporter.cpp` 内)

1. **パス区切り文字の混在によるテクスチャロード失敗**
   - `JoinPath()` が `\` を使って結合していたため、生成される `.tkm` パスが `Assets/modelData/player/Model\SK_Player.tkm` のように `/` と `\` が混在した。
   - `TkmFile::BuildMaterial`(既存コード、`Action\k2EngineLow\tkFile\TkmFile.cpp`)はテクスチャ名を組み立てる際、まず `/` の最後の位置を探し、無い場合のみ `\` を探す実装になっているため、混在パスだと `Model` フォルダの手前で区切り位置を誤検出し、生成されるテクスチャパスから `Model/` が丸ごと消えて `Assets/modelData/player/Good64x64TilingNoiseHighFreq.dds` のような誤ったパスになり、「テクスチャのロードに失敗しました」エラーが発生。
   - **修正**: `JoinPath()` の区切り文字を常に `/` に統一。
2. **texconvへの相対パス指定によるDDS書き込み失敗**
   - `ConvertTextureToDDS()` が `srcTexPath`/`outputDir` を相対パスのまま `texconv.exe` に渡していた。texconvは相対パスの入力ファイルを渡すと、出力ファイル名に入力の相対パスをそのまま連結してしまう挙動があり(`writing Assets/modelData/player/Model\Assets/modelData/player/Model/Textures/T__01.DDS FAILED (80070003)`)、書き込みに失敗する。**exitCodeは0(成功扱い)のまま**返るため、当初は気づきにくかった。
   - **修正**: `texconv.exe` に渡す入出力パスを `GetAbsolutePath()`(`GetFullPathNameA` ラッパー)で絶対パス化してから渡すよう変更。
3. **空メッシュ/空マテリアルグループによる範囲外アクセスクラッシュ**
   - `mesh->material_parts` に「実際には1枚もポリゴンが割り当てられていないマテリアルスロット」が含まれるケースがあり、その場合 `TkmFile::SMesh::indexBuffer32Array` に空のインデックスバッファが入ってしまう。
   - 既存の `MeshParts::CreateMeshFromTkmMesh`(`Action\k2EngineLow\graphics\MeshParts.cpp`、変更なし)は `tkIb.indices[0]` や `tkmMesh.vertexBuffer[0]` を無条件にアクセスするため、空だと `Debug Assertion Failed! ... vector subscript out of range` でクラッシュする。
   - **修正**: `BuildMeshParts()` で、インデックスが空のマテリアルグループはスキップし、`TkmFile::SMesh` に含めないようにした。同様に、結果的に頂点0になったメッシュ自体もスキップ。また `mesh->material_parts.count` が `mesh->materials.count` を下回る場合に備え、走査範囲をクランプする安全策も追加。
4. **mtoon/glTF系シェーダープロパティのテクスチャがufbxで検出できない**
   - `SK_Player.FBX` は全18マテリアルで `srcMat->textures.count == 0`(ufbxの汎用テクスチャ接続走査で1件もヒットしない)ことが判明。3ds Max用に別途用意されていた `MaterialTextureTable.txt` の存在から、この種のプロパティは通常のFBXテクスチャ接続(Texture→Materialプロパティの接続)ではなく、別形式で保持されていると推測される。
   - **修正**: 4章で述べた `MaterialTextureTable.txt` パーサーとフォールバックロジックを追加。

### 上記とは別に判明した問題(FBXインポート機能自体のバグではない)

- `SK_Player.FBX` にはスケルトンが無い(フェーズ1の仕様通り)。この状態で、旧「Sapphiart」モデル用に作られたアニメーションクリップ(`.tka`、ボーンインデックス参照)を `ModelRender::Init()` の `animationClips` 引数に渡すと、`Skeleton::GetBone(int)`(`Action\k2EngineLow\graphics\Skeleton.h`、**境界チェック無し**: `return m_bones[boneNo].get();`)がアニメーション更新処理(`Action\k2EngineLow\graphics\Animation.cpp` / `AnimationPlayController.cpp` の `m_skeleton->GetBone(boneNo)` 呼び出し)から範囲外アクセスされ、`vector subscript out of range` でクラッシュすることを確認した。
  - これは **Player.cpp側の呼び出しミス**(スケルトンの無いモデルにアニメーションクリップを渡した)であり、FBXインポート機能自体のバグではない。`Skeleton::GetBone()` に境界チェックが無いこと自体は既存コードの潜在的な脆さだが、今回は修正せず(スコープ外と判断)。
  - 対応: `Player::Start()` の呼び出しを `m_modelRender.Init(..., nullptr, 0, ...)` に変更し、アニメーションクリップを渡さないようにした。

### 検証結果

上記の修正後、`Game.exe`(Debug|x64)を起動し、`SK_Player.FBX` が

- クラッシュせず30FPS前後で安定動作
- `Textures` サブフォルダの画像(服の柄・髪色等)が正しく反映された状態でモデルが描画される
- 地面の上に(向きも含めて)正しく立った状態で表示される

ことをスクリーンショットで確認した。ただし、モデルのスケールは旧モデル用の値(3.0倍)のままだと画面いっぱいに映るほど大きすぎたため、表示確認のため一時的に `Vector3(0.03f, 0.03f, 0.03f)` に変更している(下記6章「現状のPlayer.cppの状態」参照)。

---

## 6. 現状のPlayer.cppの状態(要判断・引き継ぎ事項)

検証のため `Action\Game\Source\Objects\Player.cpp` の `Player::Start()` を一時的に書き換えた状態で止まっている。

```cpp
// 現状(検証用に変更済み):
m_modelRender.Init("Assets/modelData/player/Model/SK_Player.FBX", nullptr, 0, enModelUpAxisZ);
...
m_modelRender.SetScale(Vector3(0.03f, 0.03f, 0.03f)); // SK_Player.FBXは単位系が違うため暫定的に縮小(表示確認用)。
m_modelRender.Update();
```

- アニメーションクリップ(`m_animClips`、idle/walk/run/jump)は渡していない(渡すとクラッシュするため。5章参照)。
- スケールは暫定値(0.03倍)。正式な数値は未調整。
- 剣の右手ボーンへの追従(`InitSword()`)はスケルトンが無いため機能しない(`FindBoneID` が常に-1を返し、`UpdateSword()` は安全に何もしないだけで、クラッシュはしない)。

### 判断待ちの事項

1. 元の「Sapphiart」モデル(アニメーション付き)に戻すか、`SK_Player` モデルへの移行を進めるか。
   - 参考: `Action\Game\Assets\modelData\player\` フォルダは、今回の検証開始前後で**ユーザー側によって再編されている**ことを確認した。旧モデルの実体(`player.tkm` / `player.tks` / `Sapphi_*.DDS` / `master/` サブフォルダ)は現在 `Action\Game\Assets\modelData\player2\` に移動されており、`player\` フォルダには新しい `Model\`(`SK_Player.FBX` 等)と `Animation\`(`ThirdPersonRun.fbx`)のみが残っている。これは筆者(Claude)が行った操作ではなく、作業と並行してユーザー側で行われたものと見られる。旧モデルに戻す場合はパスを `Assets/modelData/player2/player.tkm` に修正する必要がある。
2. `SK_Player` モデルを正式採用する場合、今後の課題:
   - スケールの正式な調整(現在は暫定値0.03)。
   - スケルトン/アニメーション対応(フェーズ1のスコープ外。`Action\Game\Assets\modelData\player\Animation\ThirdPersonRun.fbx` が用意されていることから、将来的にモーションも移行する想定と見られる)。
   - `Skeleton::GetBone(int)` に境界チェックを追加するかどうか(現状は呼び出し側で誤用しない限り安全)。

---

## 7. ビルド確認

- `k2EngineLow.vcxproj` → `k2Engine.vcxproj` → `Game.sln` の順にクリーンビルドし、`Game.exe` のリンクまで成功することを確認済み。
- 新規追加した `.cpp`/`.h` ファイルは **UTF-8 with BOM**(CRLF)で保存する必要がある(このプロジェクトはBOM無しUTF-8だとMSVCがコードページ932[Shift-JIS]と誤認し、日本語コメント内のマルチバイト文字が原因で文字列リテラルの解析が崩れ、大量のコンパイルエラーを引き起こす)。Write/Editツールで新規作成したファイルはBOM無しになるため、`printf '\xEF\xBB\xBF' > file.bom && cat file >> file.bom && mv file.bom file` のような手順でBOMを付与する必要があった。
- ビルド構成: `k2Engine.vcxproj` の Librarian(Lib)設定が `AdditionalDependencies: k2EngineLow.lib` を指定しており、`k2EngineLow.lib` の内容を `k2Engine.lib` にマージする構成になっている。このため、`k2EngineLow` 側のプリコンパイル済みヘッダ(PCH)が変化するような変更(今回のように新規ファイル追加+PCH再生成を伴う変更)を行った後は、**`k2EngineLow` → `k2Engine`(両方ともフル)の順にリビルドしてからでないと**、リンク時に `LNK2011: プリコンパイル済みオブジェクトはリンクされていません` という不可解なエラーが再現することがある(インクリメンタルビルドの残骸によるもので、コード自体のバグではない)。

---

## 8. 参考: ufbxの主要API(実際に使用した範囲)

`Action\k2EngineLow\ExEngine\ufbx\ufbx.h` より、今回利用したAPIの要点(バージョンにより変わる可能性があるため、実装時は必ずヘッダを再確認すること):

- `ufbx_scene* ufbx_load_file(const char* filename, const ufbx_load_opts* opts, ufbx_error* error)` / `void ufbx_free_scene(ufbx_scene*)`
- `scene->meshes`(`ufbx_mesh_list`、要素は `ufbx_mesh*`)
- `ufbx_mesh::instances`(`ufbx_node_list`、そのメッシュを使うノード一覧)、`ufbx_node::geometry_to_world`(`ufbx_matrix`)
- `ufbx_mesh::materials`(`ufbx_material_list`、要素は `ufbx_material*`)と `ufbx_mesh::material_parts`(`ufbx_mesh_part_list`、マテリアルごとの面グループ。`materials.count==0` でも1件存在する)
- `ufbx_mesh_part::face_indices` → `mesh->faces.data[faceIx]`(`ufbx_face`)→ `ufbx_triangulate_face(uint32_t* indices, size_t num_indices, const ufbx_mesh* mesh, ufbx_face face)`
- `ufbx_get_vertex_vec3/vec2(&mesh->vertex_position, corner)` 等(コーナー単位の頂点属性取得)
- `ufbx_transform_position/direction(&matrix, vec3)`
- `ufbx_material::pbr` / `ufbx_material::fbx`(ビルトインのPBR/legacy FBXマテリアルマップ)、`ufbx_material::textures`(`ufbx_material_texture_list`、`{material_prop; shader_prop; texture;}` の汎用リスト。今回の検証アセットではこれが空だった)
- `ufbx_texture::filename` / `relative_filename`(`ufbx_string`、`.data`/`.length`)

---

## 9. 関連ファイル一覧(このレポートの裏付け)

- `Action\k2EngineLow\ExEngine\ufbx\ufbx.h` / `ufbx.c` / `LICENSE.txt`
- `Action\k2EngineLow\tkFile\FbxRuntimeImporter.h` / `.cpp`
- `Action\k2EngineLow\tkFile\TkmFile.h`(`SetMeshPartsForRuntimeImport` 追加)
- `Action\k2Engine\graphics\ModelRender.cpp`(`Init`/`IniTranslucent` へのフック追加)
- `Action\k2EngineLow\k2EngineLow.vcxproj` / `.vcxproj.filters`
- `Action\Game\Source\Objects\Player.cpp`(検証用に一時変更、6章参照)
- `Action\Game\Assets\modelData\player\Model\SK_Player.FBX` / `MaterialTextureTable.txt` / `MaterialTextureTable_Simple.txt` / `Textures\*.png`(検証に使用した実アセット、ユーザー提供)
