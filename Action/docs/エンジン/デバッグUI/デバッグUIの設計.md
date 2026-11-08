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

| ファイル | 置き場所 | 役割 |
|---|---|---|
| ImGui 本体とバックエンド | `Action/exlib/imgui/` | 公式の v1.92.9b をそのまま置く。バックエンド（Win32・DX12）も同じフォルダーに置く |
| `DebugUI.h` / `.cpp` | `Action/k2EngineLow/dbg/` | ImGui の起動・終了・毎フレームの処理・メッセージの受け渡し |
| `k2EngineLow.cpp` | `Action/k2EngineLow/` | `DebugUI` の関数を、起動・フレームの始めと終わり・終了で呼ぶ |
| `system.cpp` | `Action/Game/Source/System/` | `MsgProc` から `DebugUI::HandleWindowMessage()` を呼ぶ |
| `GameCamera.cpp`・`PlayerInputSource.cpp` | `Action/Game/Source/...` | 出している間は、マウスでカメラを回さない・クリックで魔法を撃たない |

ImGui の `.cpp` は k2EngineLow のプロジェクトでビルドする（プリコンパイル済みヘッダーは使わない）。k2EngineLow のライブラリは k2Engine を通して Game にリンクされるので、Game からも ImGui の関数を呼べる。

## 2. 実装方法

### namespace にして、状態は .cpp に隠す

`DebugUI` はクラスではなく namespace にした。ImGui 自体が1つだけのグローバルな状態（コンテキスト）を持つので、インスタンスを作っても1つにしかならないため。持っておく状態（ディスクリプタヒープ、出しているか）は `.cpp` の無名 namespace に置き、外からはさわれないようにしている。`k2EngineLowPreCompile.h` で読み込むので、エンジンからも Game からも `DebugUI::` で呼べる。

### ディスクリプタヒープは ImGui 専用に作る

ImGui は、文字の画像などのテクスチャをシェーダーに渡すためにディスクリプタを使う。v1.92 からは、文字の画像を途中で作り直すことがあるので、`AllocSrv` / `FreeSrv` の2つの関数で、ディスクリプタを借りて返す形になっている。64個のヒープを ImGui 専用に作り、空いている番号の一覧（`g_freeSrvIndices`）から貸す。エンジンのヒープと分けたので、エンジン側の番号の管理には手を入れていない。

### 描く場所はフレームの最後

`DebugUI::Render()` は `EndRender()` の直前に呼ぶ。この時点で、レンダリングエンジンはゲームの絵をフレームバッファに写し終わっていて、フレームバッファは描ける状態になっている。描く先をフレームバッファに設定し直し、ヒープを ImGui 専用のものに差し替えてから描く。

```cpp
commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
ID3D12DescriptorHeap* heaps[] = { g_srvHeap };
commandList->SetDescriptorHeaps(1, heaps);
ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
```

隠しているときも `ImGui::Render()` と `RenderDrawData()` は毎フレーム通す。窓がないので何も描かれないが、テクスチャを作る・消す処理がここで行われるため。

### 出している間はマウスを ImGui に使う

`GameCamera` はマウスでカメラを回すために、毎フレームカーソルを画面の真ん中に戻し、カーソルを隠している。このままでは窓をクリックできないので、`DebugUI::IsVisible()` の間はマウスルックを終わらせる（`EndMouseLook()` でカーソルが出る）。魔法のクリックも `PlayerInputSource::CanUseMouse()` で止める。キーボードは止めていないので、窓を出したまま移動できる。

### 終了は GPU を待ってから

ImGui の頂点バッファは GPU が描き終わるまで使われている。`Finalize()` では `GraphicsEngine::WaitDraw()` で GPU を待ってから ImGui を終了する。そのため `WaitDraw()` を `public` にした。

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

- **`IsVisible()` を必ず見る。** Release ビルドでは ImGui を起動しないので、見ないで呼ぶと落ちる。
- **日本語の文字列は `u8"..."` で書く。** ImGui は UTF-8 の文字列を受け取るが、このプロジェクトは `u8` を付けないと Shift-JIS になる。
- `ImGui::Begin()` に同じ名前を渡すと、同じ窓に足される。
- ImGui でできること（ボタン・グラフ・ツリーなど）は、DebugUI の「ImGui のデモを出す」で見られる。見たい部品のコードは `exlib/imgui/imgui_demo.cpp` にある。

### 決めた値を残す

ImGui は値を変えるだけで、保存はしない。決まった値は、コードの定数か JSON（[JSON読み込みの設計](../../ゲーム/システム/JSON読み込みの設計.md)）に書き写す。

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

## 6. 既知の課題

- 窓を出している間も、キーボードの入力はゲームにも届く。ImGui の入力欄に文字を打つと、キャラクターも動く。
- 窓の位置・大きさは保存されない。
- Release ビルドでの動き（何も出ないこと）は、まだ確かめていない。

## 7. 関連するページ

- [JSON読み込みの設計](../../ゲーム/システム/JSON読み込みの設計.md) — 決めた値をファイルに出すとき
- [FPSの表示の設計](../../ゲーム/UI/FPSの表示の設計.md) — 画面の右上に出している FPS
