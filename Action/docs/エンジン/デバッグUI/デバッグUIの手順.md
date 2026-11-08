# デバッグUIの手順

作成日: 2026-11-08
設計: [デバッグUIの設計](デバッグUIの設計.md)
経緯: [デバッグUIの経緯](デバッグUIの経緯.md)

ImGui を入れて、F1 キーで開発用の画面（デバッグUI）を出せるようにするまでの手順です。上から順にやると、[デバッグUIの設計](デバッグUIの設計.md) の形になります。

**出来上がるもの:** Debug・Preview ビルドで F1 を押すと、ゲームの絵の上に「DebugUI」の窓（FPS とデモの切り替え）が出ます。出している間はマウスが ImGui に使われ、カメラが回ったり魔法が出たりしません。ゲームのコードからは、`Update()` の中で `ImGui::Begin()` などを呼んで窓を足せます。

**前提:**

- Visual Studio 2022 で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 使う ImGui は **v1.92.9b**。版が違うと、手順4の `ImGui_ImplDX12_InitInfo` の中身が違うことがある。
- 作業を始める前に、main からブランチを切る。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | ImGui のファイルを `exlib/imgui/` に置く | ファイルが16個ある |
| 2 | k2EngineLow で ImGui をビルドする | k2EngineLow のビルドが通る |
| 3 | GPU を待つ関数を外から呼べるようにする | ビルドが通る |
| 4 | DebugUI を作り、起動と終了を入れる | 起動・終了で落ちない |
| 5 | ウィンドウのメッセージを ImGui に渡す | ビルドが通る |
| 6 | 毎フレーム ImGui を受け付けて描く | F1 で窓が出る |
| 7 | 窓を出している間はマウスを ImGui に使う | 窓をクリックで操作できる |

---

## 新しいファイルを作るときの注意（全手順共通）

新しく作る `.h` / `.cpp` は、**UTF-8（BOM 付き）**で保存する。BOM がないと、コンパイラーが Shift-JIS として読み、`u8"日本語"` の文字が化ける。

- Visual Studio では「ファイル → 名前を付けて保存」→ 保存ボタンの右の ▼ →「エンコード付きで保存」→「Unicode (UTF-8 シグネチャ付き) - コードページ 65001」を選ぶ。

---

## 手順1: ImGui のファイルを exlib に置く

ImGui 本体と、Windows（Win32）・DirectX12 用のつなぎのファイル（バックエンド）を、JSON の `nlohmann` と同じ `Action/exlib/` に置きます。

1. https://github.com/ocornut/imgui/releases/tag/v1.92.9b を開き、「Source code (zip)」をダウンロードして展開する。
2. `Action/exlib/imgui/` フォルダーを作り、次のファイルをコピーする。**`backends` の中の4つも、フォルダーを作らずに同じ場所へ置く。** バックエンドのファイルは `#include "imgui.h"` と書いてあるので、同じフォルダーにあればそのまま見つかる。

| 展開したフォルダーのどこから | ファイル |
|---|---|
| 一番上 | `imgui.h`、`imgui.cpp`、`imgui_draw.cpp`、`imgui_tables.cpp`、`imgui_widgets.cpp`、`imgui_demo.cpp`、`imgui_internal.h`、`imconfig.h`、`imstb_rectpack.h`、`imstb_textedit.h`、`imstb_truetype.h`、`LICENSE.txt` |
| `backends/` | `imgui_impl_win32.h`、`imgui_impl_win32.cpp`、`imgui_impl_dx12.h`、`imgui_impl_dx12.cpp` |

**確かめ方:** `Action/exlib/imgui/` にファイルが16個ある。

---

## 手順2: k2EngineLow で ImGui をビルドする

ImGui の描画はエンジンのフレームの最後で行うので、ImGui の `.cpp` は k2EngineLow のプロジェクトでビルドします。k2EngineLow のライブラリは k2Engine を通して Game にリンクされるので、Game からも ImGui の関数を呼べるようになります。

1. **インクルードパスを足す。**
   - ソリューションエクスプローラーで `k2EngineLow` を右クリック →「プロパティ」。
   - 上の「構成」を「すべての構成」、「プラットフォーム」を「x64」にする。
   - 「構成プロパティ → VC++ ディレクトリ → インクルード ディレクトリ」の先頭に `$(ProjectDir)..\exlib;` を足して「OK」。
   - これで `#include <imgui/imgui.h>` と書ける（Game にはすでに同じパスが入っている）。
2. **ファイルをプロジェクトに足す。**
   - `k2EngineLow` を右クリック →「追加 → 新しいフィルター」で `imgui` を作る。
   - `imgui` フィルターを右クリック →「追加 → 既存の項目」で、`Action/exlib/imgui/` の `.cpp` 7つ（`imgui.cpp`、`imgui_demo.cpp`、`imgui_draw.cpp`、`imgui_tables.cpp`、`imgui_widgets.cpp`、`imgui_impl_dx12.cpp`、`imgui_impl_win32.cpp`）を選んで足す。`.h` も足しておくと、ソリューションエクスプローラーから開ける。
3. **ImGui の `.cpp` だけ、プリコンパイル済みヘッダーを使わないようにする。** k2EngineLow の `.cpp` は先頭で `k2EngineLowPreCompile.h` を読む決まりだが、ImGui のファイルは読まないため、このままだと「プリコンパイル済みヘッダーが見つからない」エラーになる。
   - 足した `.cpp` 7つを選んで右クリック →「プロパティ」。
   - 「構成」を「すべての構成」、「プラットフォーム」を「x64」にする。
   - 「C/C++ → プリコンパイル済みヘッダー → プリコンパイル済みヘッダー」を「プリコンパイル済みヘッダーを使用しない」にして「OK」。

**確かめ方:** `k2EngineLow` を右クリック →「ビルド」。出力に `imgui.cpp` などが出て、エラーが0。

---

## 手順3: GPU を待つ関数を外から呼べるようにする

終了するときに、GPU が ImGui のバッファを使い終わるのを待ってから消す必要があります。待つ関数 `GraphicsEngine::WaitDraw()` はすでにありますが `private` なので、`public` に移します。

[GraphicsEngine.h](../../../k2EngineLow/graphics/GraphicsEngine.h) で、次の4行を `private:` の下から消します。

```cpp
		/// <summary>
		/// 描画の完了待ち。
		/// </summary>
		void WaitDraw();
```

同じ4行を、`EndGPUEvent()` の `#endif` のすぐ下、最初の `private:` の上に書きます。

```cpp
#endif
		/// <summary>
		/// 描画の完了待ち。
		/// </summary>
		void WaitDraw();
	private:
		/// <summary>
		/// D3Dデバイスの作成。
```

**確かめ方:** `k2EngineLow` のビルドが通る。

---

## 手順4: DebugUI を作り、起動と終了を入れる

ImGui を直接さわるのは `DebugUI` だけにします。まずは起動（`Init`）と終了（`Finalize`）だけを作り、エンジンの起動と終了から呼びます。

### 4-1. ファイルを作る

`k2EngineLow` の `dbg` フィルターを右クリック →「追加 → 新しい項目」で、`DebugUI.h` と `DebugUI.cpp` を作ります。**場所は `Action/k2EngineLow/dbg/` にする**（何もしないと `k2EngineLow` の直下に作られる）。保存は UTF-8（BOM 付き）。

### 4-2. DebugUI.h に起動と終了を宣言する

`DebugUI.h` に次を書きます。ゲームのどこからでも使えるように、クラスではなく namespace にしています（持っておく状態は `.cpp` の中に隠す）。

```cpp
#pragma once

namespace nsK2EngineLow {
	/// <summary>
	/// ImGui を使った開発用の画面（Debug・Preview ビルドだけ）。F1 キーで出す・隠すを切り替える。
	/// </summary>
	/// <remarks>
	/// ImGui の関数（ImGui::Begin など）は、IsVisible() が true のときに、
	/// ゲームオブジェクトの Update() の中で呼ぶ。BeginFrame() と Render() の間なので、そのまま描かれる。
	/// </remarks>
	namespace DebugUI {
		/// <summary>
		/// ImGui を起動する。グラフィックエンジンを作ったあとに1回だけ呼ぶ。
		/// </summary>
		void Init(HWND hwnd);
		/// <summary>
		/// ImGui を終了する。グラフィックエンジンを消す前に呼ぶ。
		/// </summary>
		void Finalize();
	}
}
```

### 4-3. エンジン全体から見えるようにする

[k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h) の `#include "dbg/MyLog.h"` の下に1行足します。Game の `stdafx.h` もこのファイルを読んでいるので、Game からも `DebugUI::` と書けるようになります。

```cpp
#include "dbg/MyLog.h"
#include "dbg/DebugUI.h"
```

### 4-4. DebugUI.cpp に起動と終了を書く

`DebugUI.cpp` に次を書きます。

```cpp
#include "k2EngineLowPreCompile.h"
#include "dbg/DebugUI.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

namespace nsK2EngineLow {
	namespace DebugUI {
		namespace {
#ifdef K2_DEBUG
			constexpr bool IS_ENABLED = true;			// Debug・Preview ビルドだけ ImGui を動かす。
#else
			constexpr bool IS_ENABLED = false;
#endif
			constexpr int SRV_HEAP_SIZE = 64;			// ImGui のテクスチャ（文字の画像など）に使うディスクリプタの数。
			constexpr int NUM_FRAMES_IN_FLIGHT = 2;		// フレームバッファの数（FrameBuffer::FRAME_BUFFER_COUNT）と同じにする。
			constexpr float FONT_SIZE = 18.0f;			// 文字の大きさ（ピクセル）。
			constexpr const char* FONT_FILE_PATH = "C:\\Windows\\Fonts\\meiryo.ttc";	// 日本語を出すためのフォント。

			bool g_isInitialized = false;				// Init() が成功したか。
			ID3D12DescriptorHeap* g_srvHeap = nullptr;	// ImGui 専用のディスクリプタヒープ。
			UINT g_srvIncrementSize = 0;				// ディスクリプタ1つ分の大きさ。
			std::vector<int> g_freeSrvIndices;			// 空いているディスクリプタの番号。

			// ImGui がテクスチャを作るときに呼ばれ、空いているディスクリプタを1つ渡す。
			void AllocSrv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
			{
				IM_ASSERT(!g_freeSrvIndices.empty());
				const int index = g_freeSrvIndices.back();
				g_freeSrvIndices.pop_back();
				outCpuHandle->ptr = g_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<SIZE_T>(index) * g_srvIncrementSize;
				outGpuHandle->ptr = g_srvHeap->GetGPUDescriptorHandleForHeapStart().ptr + static_cast<UINT64>(index) * g_srvIncrementSize;
			}

			// ImGui がテクスチャを消すときに呼ばれ、ディスクリプタを空きに戻す。
			void FreeSrv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE)
			{
				const SIZE_T offset = cpuHandle.ptr - g_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
				g_freeSrvIndices.push_back(static_cast<int>(offset / g_srvIncrementSize));
			}
		}

		void Init(HWND hwnd)
		{
			if (!IS_ENABLED || g_graphicsEngine == nullptr) {
				return;
			}
			ID3D12Device* device = g_graphicsEngine->GetD3DDevice();

			// ImGui 専用のディスクリプタヒープを作る。エンジンのヒープとは分けて、互いに番号を気にしなくて済むようにする。
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.NumDescriptors = SRV_HEAP_SIZE;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_srvHeap)))) {
				return;
			}
			g_srvIncrementSize = device->GetDescriptorHandleIncrementSize(heapDesc.Type);
			for (int i = SRV_HEAP_SIZE - 1; i >= 0; i--) {
				g_freeSrvIndices.push_back(i);
			}

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			// imgui.ini（ウィンドウの位置の記録）を作らない。ゲームのフォルダーにファイルが増えないようにするため。
			io.IniFilename = nullptr;
			// 日本語のフォントがあれば使う。なければ ImGui の標準のフォント（英数字だけ）になる。
			if (GetFileAttributesA(FONT_FILE_PATH) != INVALID_FILE_ATTRIBUTES) {
				io.Fonts->AddFontFromFileTTF(FONT_FILE_PATH, FONT_SIZE);
			}
			ImGui::StyleColorsDark();

			ImGui_ImplWin32_Init(hwnd);

			ImGui_ImplDX12_InitInfo initInfo;
			initInfo.Device = device;
			initInfo.CommandQueue = g_graphicsEngine->GetCommandQueue();
			initInfo.NumFramesInFlight = NUM_FRAMES_IN_FLIGHT;
			initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;	// フレームバッファの色の形式（FrameBuffer.cpp）。
			initInfo.DSVFormat = DXGI_FORMAT_D32_FLOAT;		// フレームバッファの深度の形式（FrameBuffer.cpp）。
			initInfo.SrvDescriptorHeap = g_srvHeap;
			initInfo.SrvDescriptorAllocFn = AllocSrv;
			initInfo.SrvDescriptorFreeFn = FreeSrv;
			ImGui_ImplDX12_Init(&initInfo);

			g_isInitialized = true;
		}

		void Finalize()
		{
			if (!g_isInitialized) {
				return;
			}
			// GPU が ImGui のバッファを使い終わるまで待ってから消す。
			g_graphicsEngine->WaitDraw();
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			g_srvHeap->Release();
			g_srvHeap = nullptr;
			g_freeSrvIndices.clear();
			g_isInitialized = false;
		}
	}
}
```

- **ディスクリプタヒープは ImGui 専用に作る。** v1.92 から、ImGui は文字の画像などのテクスチャを途中で作り直すことがあり、そのたびに `AllocSrv` / `FreeSrv` でディスクリプタを借りて返す。エンジンのヒープと分けておけば、番号がぶつからない。
- **`IS_ENABLED` が false（Release ビルド）のときは何もしない。** `g_isInitialized` が false のままなので、このあと足す関数もすべて何もしなくなる。

### 4-5. エンジンの起動と終了から呼ぶ

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の2か所に足します。

`K2EngineLow::Init()` の中、`m_graphicsEngine->Init(...)` の呼び出しのすぐ下（`if (hwnd) {` の閉じかっこの前）:

```cpp
				raytracingInitData
			);
			// 開発用の画面（ImGui）を起動する。
			DebugUI::Init(hwnd);
		}
```

`K2EngineLow::~K2EngineLow()` の先頭（`g_graphicsEngine = nullptr;` より前。`Finalize()` の中で `g_graphicsEngine` を使うため）:

```cpp
	K2EngineLow::~K2EngineLow()
	{
		// ImGui はグラフィックエンジンを使っているので、先に終了する。
		DebugUI::Finalize();
		// グローバルなアクセスポイントにnullptrを代入。
```

**確かめ方:**

- ソリューションをビルドしてエラーが0。
- ゲームを起動して、いつもどおり動く。ウィンドウを閉じても落ちない（まだ画面には何も出ない）。
- Game のビルドで `'DebugUI': 識別子がクラス名でも名前空間名でもありません` と出たら、Game のプリコンパイル済みヘッダーが作り直されていない。`Game` を右クリック →「リビルド」する。

---

## 手順5: ウィンドウのメッセージを ImGui に渡す

ImGui はキーやマウスの入力を、ウィンドウのメッセージから受け取ります。ゲームのメッセージ処理（`MsgProc`）から `DebugUI` に渡します。

### 5-1. DebugUI.h に宣言を足す

`Finalize()` の宣言の下に足します。

```cpp
		/// <summary>
		/// ウィンドウのメッセージを ImGui に渡す。true が返ったら、ImGui が使ったメッセージ。
		/// </summary>
		bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
```

### 5-2. DebugUI.cpp に書く

まず、`#include <imgui/imgui_impl_dx12.h>` の下に、ImGui のメッセージ処理の宣言を足します。`imgui_impl_win32.h` では、`windows.h` に頼らないためにこの宣言が `#if 0` で外してあるので、使う側で書く決まりです。

```cpp
#include <imgui/imgui_impl_dx12.h>

// imgui_impl_win32.h では、windows.h に頼らないために宣言が #if 0 で外してあるので、ここで宣言する。
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
```

次に、`Finalize()` の下（`namespace DebugUI` の閉じかっこの前）に足します。

```cpp
		bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (!g_isInitialized) {
				return false;
			}
			return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) != 0;
		}
```

### 5-3. MsgProc から呼ぶ

[system.cpp](../../../Game/Source/System/system.cpp) の `MsgProc()` の先頭（`switch (msg)` より前）に足します。

```cpp
LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// 開発用の画面（ImGui）が使ったメッセージは、ゲームでは使わない。
	if (DebugUI::HandleWindowMessage(hWnd, msg, wParam, lParam)) {
		return true;
	}
	//送られてきたメッセージで処理を分岐させる。
```

**確かめ方:** ソリューションのビルドが通る（画面はまだ変わらない）。

---

## 手順6: 毎フレーム ImGui を受け付けて描く

フレームの始めに ImGui の受け付けを始め（`BeginFrame`）、フレームの終わりにゲームの絵の上へ描きます（`Render`）。始めと終わりの間にゲームの `Update()` が呼ばれるので、ゲームからも ImGui の関数を呼べます。F1 で出す・隠すを切り替え、出しているときは DebugUI 自身の窓を出します。

### 6-1. DebugUI.h に宣言を足す

`Finalize()` の宣言の下（`HandleWindowMessage()` の上）に足します。

```cpp
		/// <summary>
		/// フレームの始めに呼ぶ。ImGui の受け付けを始め、F1 で出す・隠すを切り替える。
		/// </summary>
		void BeginFrame();
		/// <summary>
		/// フレームの終わり（画面に出す直前）に呼ぶ。ImGui の画面をフレームバッファに描く。
		/// </summary>
		void Render();
		/// <summary>
		/// 開発用の画面を出しているか。出している間は、マウスを ImGui に使う。
		/// </summary>
		bool IsVisible();
```

### 6-2. DebugUI.cpp に書く

無名の `namespace { }` の中、`bool g_isInitialized = false;` の下に、出しているかどうかの変数を2つ足します。

```cpp
			bool g_isInitialized = false;				// Init() が成功したか。
			bool g_isVisible = false;					// 開発用の画面を出しているか。
			bool g_isShowDemoWindow = false;			// ImGui のデモの画面を出しているか。
```

同じ無名の `namespace { }` の中、`FreeSrv()` の下に、DebugUI 自身の窓を描く関数を足します。

```cpp
			// DebugUI 自身の画面。開いていることがわかるように、FPS とデモの切り替えを出す。
			void DrawDebugUIWindow()
			{
				ImGui::Begin("DebugUI");
				ImGui::Text(u8"F1 で閉じる");
				ImGui::Text("FPS %.1f", ImGui::GetIO().Framerate);
				ImGui::Checkbox(u8"ImGui のデモを出す", &g_isShowDemoWindow);
				ImGui::End();
				if (g_isShowDemoWindow) {
					ImGui::ShowDemoWindow(&g_isShowDemoWindow);
				}
			}
```

`Finalize()` の下（`HandleWindowMessage()` の上）に、3つの関数を足します。

```cpp
		void BeginFrame()
		{
			if (!g_isInitialized) {
				return;
			}
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
				g_isVisible = !g_isVisible;
			}
			if (g_isVisible) {
				DrawDebugUIWindow();
			}
		}

		void Render()
		{
			if (!g_isInitialized) {
				return;
			}
			// 隠しているときも毎フレーム描く処理を通す。何も描かれないが、ImGui がテクスチャを作る・消す処理がここで行われるため。
			ImGui::Render();

			ID3D12GraphicsCommandList* commandList = g_graphicsEngine->GetCommandList();
			const D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_graphicsEngine->GetCurrentFrameBuffuerRTV();
			const D3D12_CPU_DESCRIPTOR_HANDLE dsv = g_graphicsEngine->GetCurrentFrameBuffuerDSV();
			commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
			ID3D12DescriptorHeap* heaps[] = { g_srvHeap };
			commandList->SetDescriptorHeaps(1, heaps);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
		}

		bool IsVisible()
		{
			return g_isInitialized && g_isVisible;
		}
```

- **`Render()` は、描く先をフレームバッファに設定し直してから描く。** この時点のフレームバッファには、レンダリングエンジンがゲームの絵を写し終わっている。
- **ディスクリプタヒープを ImGui 専用のものに差し替える。** 次のフレームでは、エンジンが描くたびに自分のヒープを設定し直すので、戻さなくてよい。
- **F1 は `ImGui::IsKeyPressed()` で見る。** 手順5でメッセージを渡しているので、ImGui がキーを知っている。

### 6-3. エンジンのフレームの始めと終わりから呼ぶ

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の2か所に足します。

`K2EngineLow::BeginFrame()` の `m_graphicsEngine->BeginRender();` の下:

```cpp
		m_graphicsEngine->BeginRender();
		// ImGui の受け付けを始める。ここから EndFrame() までの間、ゲームの Update で ImGui の関数を呼べる。
		DebugUI::BeginFrame();
```

`K2EngineLow::EndFrame()` の `m_graphicsEngine->EndRender();` の上:

```cpp
		// ImGui の画面を、ゲームの絵の上に描く。
		DebugUI::Render();
		m_graphicsEngine->EndRender();
```

**確かめ方:**

- ゲームを起動して F1 を押すと、左上のあたりに「DebugUI」の窓が出て、FPS の数字が動く。もう一度 F1 で消える。
- 日本語（「F1 で閉じる」）が正しく出る。化けるときは、`DebugUI.cpp` が UTF-8（BOM 付き）で保存されていない。
- この時点では、マウスがカメラの操作に使われるので、カーソルが出ず、窓をクリックできない。次の手順で直す。

---

## 手順7: 窓を出している間はマウスを ImGui に使う

カメラは、毎フレームカーソルを画面の真ん中に戻して、動いた量で向きを変えています（カーソルも隠す）。また、左右クリックで魔法を撃ちます。DebugUI を出している間は、この2つを止めます。

### 7-1. カメラを回さない

[GameCamera.cpp](../../../Game/Source/Objects/GameCamera.cpp) の `GameCamera::TryGetMouseRotationInput()` で、「ウィンドウが非アクティブなら強制的にマウスルックを終了する」の `if` の下に足します。`EndMouseLook()` がカーソルを出し直します。

```cpp
	// ウィンドウが非アクティブなら強制的にマウスルックを終了する。
	if (g_hWnd == nullptr || GetForegroundWindow() != g_hWnd) {
		EndMouseLook();
		return false;
	}

	// 開発用の画面（ImGui）を出している間は、マウスを ImGui に使うので、カメラは回さない。
	if (DebugUI::IsVisible()) {
		EndMouseLook();
		return false;
	}
```

### 7-2. クリックで魔法を撃たない

[PlayerInputSource.h](../../../Game/Source/Character/Input/PlayerInputSource.h) の `static bool IsGameWindowActive();` の下に宣言を足します。

```cpp
	static bool IsGameWindowActive();
	// マウスのボタンをゲームの操作に使ってよいか（ウィンドウが前にあり、開発用の画面を出していない）。
	static bool CanUseMouse();
```

[PlayerInputSource.cpp](../../../Game/Source/Character/Input/PlayerInputSource.cpp) の `IsGameWindowActive()` の下（`IsKeyPressed()` の上）に足します。

```cpp
bool PlayerInputSource::CanUseMouse()
{
	// 開発用の画面（ImGui）を出している間は、クリックを ImGui に使うので、魔法を撃たない。
	return IsGameWindowActive() && !DebugUI::IsVisible();
}
```

同じファイルの `Update()` で、左右クリックを見ている2行の `IsGameWindowActive()` を `CanUseMouse()` に変えます。

```cpp
	input.fire.Update((CanUseMouse() && IsKeyPressed(VK_LBUTTON)) || pad.IsPress(enButtonY));
	// パッド未接続時は右トリガー(RB2)が8キーに割り当てられている。
	input.subFire.Update((CanUseMouse() && IsKeyPressed(VK_RBUTTON)) || pad.IsPress(enButtonRB2));
```

**確かめ方（これで出来上がり）:**

- F1 を押すと、カーソルが出て、カメラが止まる。
- 「ImGui のデモを出す」をクリックすると、デモの窓が出る。クリックしても魔法は出ない。
- キーボードの操作（移動・ジャンプなど）はそのまま効く（キーボードは止めていない）。
- もう一度 F1 で窓が消え、マウスでカメラが回り、クリックで魔法が出る。
- ウィンドウを閉じても落ちない。

---

## ゲームから窓を足すとき

ここまでで仕組みは出来上がりです。調整用の窓を足す方法は、[デバッグUIの設計](デバッグUIの設計.md) の「3. 使い方」を見てください。
