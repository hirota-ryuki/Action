# デバッグUIの設計

作成日: 2026-11-08
対象: [DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h)、[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp)、[exlib/imgui](../../../exlib/imgui/)（ImGui v1.92.9b）
手順: [デバッグUIの手順](デバッグUIの手順.md)
経緯: [デバッグUIの経緯](デバッグUIの経緯.md)

ゲームの画面の上に、開発用の窓（スライダーやチェックボックス）を出す仕組みのページです。ライティングなどの調整値を、ゲームを動かしたまま変えるために使います。

> **状態:** [デバッグUIの手順](デバッグUIの手順.md) どおりに実装してあります。ビルドと起動・終了は確かめてありますが、F1 で窓が出ること・日本語・クリックは、画面ではまだ確かめていません。

- **ImGui をさわるのは `DebugUI` だけにする。** 起動・フレームの始めと終わり・終了・メッセージの受け渡しを、エンジンから決まった場所で呼ぶ。
- **ゲームのコードは、`DebugUI::IsVisible()` が true のときに `Update()` の中で ImGui の関数を呼ぶだけ。** 窓の登録などはいらない。
- **Debug・Preview ビルドだけで動く。** F1 で出す・隠すを切り替え、出している間はマウスを ImGui に使う。

**用語:**

| 用語 | 何のことか |
|---|---|
| メッセージ | Windows が、キー入力・マウス操作・窓の大きさの変更などを、プログラムに知らせる仕組み |

---

## 1. 全体像

```
K2Engine::Execute()（1フレーム）
  K2EngineLow::BeginFrame()
    GraphicsEngine::BeginRender()
    DebugUI::BeginFrame()      ← ImGui の受け付けを始める。F1 で出す・隠す。DebugUI 自身の窓
  ExecuteUpdate()              ← ゲームの Update()。ここで ImGui::Begin() などを呼べる
  ExecuteRender()・RenderingEngine::Execute()
                               ← ゲームの絵を描き、フレームバッファに写す
  K2EngineLow::EndFrame()
    DebugUI::Render()          ← ImGui の窓をフレームバッファの上に描く
    GraphicsEngine::EndRender()

MsgProc（Game/Source/System/system.cpp）
  DebugUI::HandleWindowMessage() ← キー・マウスのメッセージを ImGui に渡す
```

**用語:**

| 用語 | 何のことか |
|---|---|
| フレームバッファ | 画面に出す画像を入れておく置き場。ここに描くと、画面に出る |
| `MsgProc` | 窓に届いたメッセージを受け取る関数（ゲームの [system.cpp](../../../Game/Source/System/system.cpp) にある） |

| ファイル | 置き場所 | 役割 |
|---|---|---|
| ImGui 本体とバックエンド | `Action/exlib/imgui/` | 公式の v1.92.9b をそのまま置く。バックエンド（Win32・DX12）も同じフォルダーに置く |
| `DebugUI.h` / `.cpp` | `Action/k2EngineLow/dbg/` | ImGui の起動・終了・毎フレームの処理・メッセージの受け渡し |
| `k2EngineLow.cpp` | `Action/k2EngineLow/` | `DebugUI` の関数を、起動・フレームの始めと終わり・終了で呼ぶ |
| `system.cpp` | `Action/Game/Source/System/` | `MsgProc` から `DebugUI::HandleWindowMessage()` を呼ぶ |
| `GameCamera.cpp`・`PlayerInputSource.cpp` | `Action/Game/Source/...` | 出している間は、マウスでカメラを回さない・クリックで魔法を撃たない |

**用語:**

| 用語 | 何のことか |
|---|---|
| バックエンド | ImGui を、Windows や描画の仕組みとつなぐ部品。キー・マウスを受け取る Win32 用と、描く DX12 用がある |
| Win32 | Windows の、窓やキー・マウスの入力を扱う API |
| DX12 | DirectX 12。GPU に描かせるための Windows の API |

ImGui の `.cpp` は k2EngineLow のプロジェクトでビルドする（プリコンパイル済みヘッダーは使わない）。k2EngineLow のライブラリは k2Engine を通して Game にリンクされるので、Game からも ImGui の関数を呼べる。

**用語:**

| 用語 | 何のことか |
|---|---|
| プリコンパイル済みヘッダー | よく使うヘッダーを先に1回だけコンパイルしておき、ビルドを速くする仕組み（[k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h)） |
| リンク | コンパイルしたプログラムの部品を、1つにつなぐこと |

## 2. 実装方法

### namespace にして、状態は .cpp に隠す

`DebugUI` はクラスではなく namespace にした。ImGui 自体が1つだけのグローバルな状態（コンテキスト）を持つので、インスタンスを作っても1つにしかならないため。持っておく状態（ディスクリプタヒープ、出しているか）は `.cpp` の無名 namespace に置き、外からはさわれないようにしている。`k2EngineLowPreCompile.h` で読み込むので、エンジンからも Game からも `DebugUI::` で呼べる。

**用語:**

| 用語 | 何のことか |
|---|---|
| コンテキスト | ImGui が持つ、窓や入力の状態のまとまり。1つだけ作る |
| ディスクリプタヒープ | ディスクリプタを並べておく置き場 |
| 無名 namespace | 名前を付けない namespace。中に置いた名前は、その `.cpp` の外から見えない |

### ディスクリプタヒープは ImGui 専用に作る

ImGui は、文字の画像などのテクスチャをシェーダーに渡すためにディスクリプタを使う。v1.92 からは、文字の画像を途中で作り直すことがあるので、`AllocSrv` / `FreeSrv` の2つの関数で、ディスクリプタを借りて返す形になっている。64個のヒープを ImGui 専用に作り、空いている番号の一覧（`g_freeSrvIndices`）から貸す。エンジンのヒープと分けたので、エンジン側の番号の管理には手を入れていない。

**用語:**

| 用語 | 何のことか |
|---|---|
| ディスクリプタ | テクスチャなどの場所と形を、GPU に教える小さなデータ |
| SRV | Shader Resource View。シェーダーがテクスチャを読むためのディスクリプタ |

### 描く場所はフレームの最後

`DebugUI::Render()` は `EndRender()` の直前に呼ぶ。この時点で、レンダリングエンジンはゲームの絵をフレームバッファに写し終わっていて、フレームバッファは描ける状態になっている。描く先をフレームバッファに設定し直し、ヒープを ImGui 専用のものに差し替えてから描く。

```cpp
commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
ID3D12DescriptorHeap* heaps[] = { g_srvHeap };
commandList->SetDescriptorHeaps(1, heaps);
ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
```

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 変数 | `commandList` | GPU への命令を並べて入れるため | 描く命令を溜めておく入れ物（コマンドリスト）。`GraphicsEngine::GetCommandList()` で取る | k2EngineLow（エンジン）の [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の `Render()` の中 |
| 関数 | `OMSetRenderTargets()` | 描く先を決めるため | 描く先（レンダーターゲット）と、深度の書き込み先を決める。最初の 1 は描く先の数 | Windows の API（DirectX 12。<d3d12.h>。[k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h) で読み込み済み）の命令 |
| 変数 | `rtv` | 描く先を持つため | 今のフレームバッファの、描く先を指すハンドル | k2EngineLow（エンジン）の [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の `Render()` の中 |
| マクロ | `FALSE` | 「描く先のハンドルが、連続して並んでいるか」を偽にするため | 0。描く先は1つなので、どちらでも同じ | Windows の API（<windows.h>） |
| 変数 | `dsv` | 深度の書き込み先を持つため | 今のフレームバッファの、深度の書き込み先を指すハンドル | k2EngineLow（エンジン）の [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の `Render()` の中 |
| 変数 | `heaps` | 使うヒープを、配列で渡すため | ImGui 専用のディスクリプタヒープ（`g_srvHeap`）が1つ入っている | k2EngineLow（エンジン）の [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の `Render()` の中 |
| 変数 | `g_srvHeap` | ImGui 専用のヒープを持つため | ImGui のテクスチャのディスクリプタを入れるヒープ。64個ぶんある | k2EngineLow（エンジン）の [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の無名 namespace |
| 関数 | `SetDescriptorHeaps()` | これから描くときに使うヒープを、GPU に伝えるため | 渡したヒープに切り替える。エンジンのヒープから、ImGui 専用のものに差し替える | Windows の API（DirectX 12。<d3d12.h>。[k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h) で読み込み済み）の命令 |
| 関数 | `ImGui_ImplDX12_RenderDrawData()` | ImGui の窓を、DX12 で描くため | 描く内容を、コマンドリストへ積む | 外部ライブラリ（ImGui）の [imgui_impl_dx12.h](../../../exlib/imgui/imgui_impl_dx12.h) |
| 関数 | `ImGui::GetDrawData()` | 今のフレームで描く内容を取り出すため | 窓の頂点・三角形など、描く内容を返す。`ImGui::Render()` のあとに呼ぶ | 外部ライブラリ（ImGui）の [imgui.h](../../../exlib/imgui/imgui.h) |

**用語:**

| 用語 | 何のことか |
|---|---|
| コマンドリスト | GPU への命令を、並べて溜めておく入れ物。あとでまとめて GPU に渡す |
| レンダーターゲット | 描く先の画像 |

隠しているときも `ImGui::Render()` と `RenderDrawData()` は毎フレーム通す。窓がないので何も描かれないが、テクスチャを作る・消す処理がここで行われるため。

### 出している間はマウスを ImGui に使う

`GameCamera` はマウスでカメラを回すために、毎フレームカーソルを画面の真ん中に戻し、カーソルを隠している。このままでは窓をクリックできないので、`DebugUI::IsVisible()` の間はマウスルックを終わらせる（`EndMouseLook()` でカーソルが出る）。魔法のクリックも `PlayerInputSource::CanUseMouse()` で止める。キーボードは止めていないので、窓を出したまま移動できる。

**用語:**

| 用語 | 何のことか |
|---|---|
| マウスルック | マウスを動かして、カメラの向きを回すこと。`GameCamera` は、毎フレームカーソルを画面の真ん中に戻して、これを実現している |

### 終了は GPU を待ってから

ImGui の頂点バッファは GPU が描き終わるまで使われている。`Finalize()` では `GraphicsEngine::WaitDraw()` で GPU を待ってから ImGui を終了する。そのため `WaitDraw()` を `public` にした。

**用語:**

| 用語 | 何のことか |
|---|---|
| 頂点バッファ | 描く図形の頂点を入れておく、GPU が読むための置き場 |

## 3. 使い方

### 調整用の窓を足す

ゲームオブジェクトの `Update()` で、`IsVisible()` のときだけ ImGui の関数を呼ぶ。

```cpp
#include <imgui/imgui.h>

void Game::Update()
{
	if (DebugUI::IsVisible()) {
		ImGui::Begin(u8"ライト");
		ImGui::SliderFloat(u8"明るさ", &m_lightPower, 0.0f, 10.0f);
		ImGui::ColorEdit3(u8"色", &m_lightColor.x);
		ImGui::End();
	}
}
```

**名前:**

| ジャンル | 名前 | 何のためのもの | 何をするためのもの | どこから来たもの |
|---|---|---|---|---|
| 関数 | `ImGui::Begin()` | 窓を1つ開くため | `ImGui::End()` までの間に置いた部品が、この窓に入る。同じ名前を渡すと、同じ窓に足される | 外部ライブラリ（ImGui）の [imgui.h](../../../exlib/imgui/imgui.h) |
| 関数 | `ImGui::SliderFloat()` | 数値をスライダーで変えるため | スライダーを出し、動かすと渡した `float` を書き換える。引数は、名前・変数のアドレス・最小・最大 | 外部ライブラリ（ImGui）の [imgui.h](../../../exlib/imgui/imgui.h) |
| メンバー変数 | `m_lightPower` | 調整する値を持つため | 例として使う `float` の変数 | 使う側が決める（この見本のコード） |
| 関数 | `ImGui::ColorEdit3()` | 色を変えるため | 色の見本と、r・g・b の入力欄を出す。3つの `float` の先頭のアドレスを渡す | 外部ライブラリ（ImGui）の [imgui.h](../../../exlib/imgui/imgui.h) |
| メンバー変数 | `m_lightColor` | 調整する色を持つため | 例として使う色の変数。`.x`・`.y`・`.z` が r・g・b として並んでいる | 使う側が決める（この見本のコード） |
| 関数 | `ImGui::End()` | 窓を閉じるため | `ImGui::Begin()` で開いた窓を終える | 外部ライブラリ（ImGui）の [imgui.h](../../../exlib/imgui/imgui.h) |

- **`IsVisible()` を必ず見る。** Release ビルドでは ImGui を起動しないので、見ないで呼ぶと落ちる。
- **日本語の文字列は `u8"..."` で書く。** ImGui は UTF-8 の文字列を受け取るが、このプロジェクトは `u8` を付けないと Shift-JIS になる。
- `ImGui::Begin()` に同じ名前を渡すと、同じ窓に足される。
- ImGui でできること（ボタン・グラフ・ツリーなど）は、DebugUI の「ImGui のデモを出す」で見られる。見たい部品のコードは `exlib/imgui/imgui_demo.cpp` にある。

**用語:**

| 用語 | 何のことか |
|---|---|
| UTF-8・Shift-JIS | 文字を数値にして持つ決まりの名前。UTF-8 は世界中の文字を扱える。Shift-JIS は、日本語の Windows で昔から使われている |

### 決めた値を残す

ImGui は値を変えるだけで、保存はしない。決まった値は、コードの定数か JSON（[JSON読み込みの設計](../../ゲーム/システム/JSON読み込み/JSON読み込みの設計.md)）に書き写す。

## 4. 調整用の数値

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の無名 namespace にある。

| 名前 | 値 | 意味 |
|---|---|---|
| `SRV_HEAP_SIZE` | 64 | ImGui に貸すディスクリプタの数。テクスチャ（画像）を自分で出すようになって足りなくなったら増やす |
| `NUM_FRAMES_IN_FLIGHT` | 2 | フレームバッファの数（`FrameBuffer::FRAME_BUFFER_COUNT`）と同じにする |
| `FONT_SIZE` | 18.0 | 文字の大きさ（ピクセル） |
| `FONT_FILE_PATH` | `C:\Windows\Fonts\meiryo.ttc` | 日本語のフォント。ないときは ImGui の標準のフォント（英数字だけ） |

出す・隠すのキー（F1）は `BeginFrame()` の `ImGuiKey_F1`。

## 5. 決めたこと

- **ImGui は v1.92.9b にした。** 2026-11-08 時点で、いちばん新しい v1.93.0 は出てから1か月たっていないので、その前の安定版にした。
- **imgui.ini は作らない。** 窓の位置は起動のたびに戻るが、ゲームのフォルダーにファイルが増えない。
- **窓の登録の仕組みは作らない。** ImGui はその場で呼べばよい作りなので、`Update()` で直接呼ぶ形にした。

**用語:**

| 用語 | 何のことか |
|---|---|
| `imgui.ini` | ImGui が、窓の位置や大きさを保存しておくファイル |

## 6. 既知の課題

- 窓を出している間も、キーボードの入力はゲームにも届く。ImGui の入力欄に文字を打つと、キャラクターも動く。
- 窓の位置・大きさは保存されない。
- Release ビルドでの動き（何も出ないこと）は、まだ確かめていない。

## 7. 関連するページ

- [JSON読み込みの設計](../../ゲーム/システム/JSON読み込み/JSON読み込みの設計.md) — 決めた値をファイルに出すとき
- [FPSの表示の設計](../../ゲーム/UI/FPSの表示/FPSの表示の設計.md) — 画面の右上に出している FPS
