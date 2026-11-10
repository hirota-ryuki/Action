# デバッグUIの手順

設計: [デバッグUIの設計](デバッグUIの設計.md)
経緯: [デバッグUIの経緯](デバッグUIの経緯.md)

ImGui を入れて、F1 キーで開発用の画面（デバッグUI）を出せるようにするまでの手順です。

**出来上がるもの:** Debug・Preview ビルドで F1 を押すと、ゲームの絵の上に「DebugUI」の窓（FPS とデモの切り替え）が出ます。出している間はマウスが ImGui に使われ、カメラが回ったり魔法が出たりしません。ゲームのコードからは、`Update()` の中で `ImGui::Begin()` などを呼んで窓を足せます。

**前提:**

- Visual Studio で `Action/Game/Game.sln` を開き、構成は **Debug / x64** でビルドする。
- 使う ImGui は **v1.92.9b**。版が違うと、手順4の `ImGui_ImplDX12_InitInfo` の中身が違うことがある。
- ゲームの手順書の流れでは、[プレイヤーの調整値の手順](../../ゲーム/プレイヤー/プレイヤーの調整値の手順.md) のあとに進める（行番号はこの順で数えている）。

| 手順 | 足すもの | 確かめ方 |
|---|---|---|
| 1 | ImGui のファイルを `exlib/imgui/` に置く | ファイルが16個ある |
| 2 | k2EngineLow で ImGui をビルドする | k2EngineLow のビルドが通る |
| 3 | GPU を待つ関数を外から呼べるようにする | ビルドが通る |
| 4 | DebugUI を作り、起動と終了を入れる | 起動・終了で落ちない |
| 5 | ウィンドウのメッセージを ImGui に渡す | ビルドが通る |
| 6 | 毎フレーム ImGui を受け付けて描く | F1 で窓が出る |
| 7 | 窓を出している間はマウスを ImGui に使う | 窓をクリックで操作できる |

**読み方:**

- 足す場所は「〇行目 `その行に書いてあること` の下」で示します。行番号は、それより前の小手順を済ませたときの番号です。
- 枠の左の数字は、足したあとの行番号です。右の枠のコードを、そのまま書きます。
- 消すところは、消す行を枠で示します（番号は消す前の番号）。書き換えるところは、「前」と「後」の枠を並べています。
- VSCode で `Ctrl+Shift+V` を押してプレビューで読みます。

---

## 触るファイルの一覧

| ファイル | 何をするか | 手順 |
|---|---|---|
| `Action/exlib/imgui/`（16個） | 新しく置く（ImGui のファイルをコピー） | 1 |
| `k2EngineLow` のプロジェクトの設定 | インクルードパスを足す、ImGui の `.cpp` 7つを足して、プリコンパイル済みヘッダーを使わない | 2 |
| [GraphicsEngine.h](../../../k2EngineLow/graphics/GraphicsEngine.h) | 削除・追加（`WaitDraw()` の宣言を `public` へ移す） | 3 |
| [DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) | 新しく作る・追加 | 4・5・6 |
| [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) | 新しく作る・追加 | 4・5・6 |
| [k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h) | 追加 | 4 |
| [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) | 追加 | 4・6 |
| [system.cpp](../../../Game/Source/System/system.cpp) | 追加 | 5 |
| [GameCamera.cpp](../../../Game/Source/Objects/GameCamera.cpp) | 追加 | 7 |
| [PlayerInputSource.h](../../../Game/Source/Character/Input/PlayerInputSource.h) | 追加 | 7 |
| [PlayerInputSource.cpp](../../../Game/Source/Character/Input/PlayerInputSource.cpp) | 追加・書き換え | 7 |

新しく作る `.h` / `.cpp` は、**UTF-8（BOM 付き）**で保存します。BOM がないと、コンパイラーが Shift-JIS として読み、`u8"日本語"` の文字が化けます（Visual Studio では「ファイル → 名前を付けて保存」→ 保存ボタンの右の ▼ →「エンコード付きで保存」→「Unicode (UTF-8 シグネチャ付き) - コードページ 65001」）。

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

終了するときに、GPU が ImGui のバッファを使い終わるのを待ってから消す必要があります。待つ関数 `GraphicsEngine::WaitDraw()` はすでにありますが `private` なので、`public` に移します。関数の中身（`GraphicsEngine.cpp`）は変えません。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GraphicsEngine.h](../../../k2EngineLow/graphics/GraphicsEngine.h) | 削除（`private:` の中の `WaitDraw()` の宣言4行）・追加（同じ4行を `public` の最後へ） |

### 3-1. private の中の WaitDraw() の宣言を消す

`CreateSynchronizationWithGPUObject()` の下にあります。

[GraphicsEngine.h](../../../k2EngineLow/graphics/GraphicsEngine.h) の **359〜362行目**を消します。

<!-- del: Action/k2EngineLow/graphics/GraphicsEngine.h 359 -->
<table class="code-lines"><tr><td>

```cpp
359
360
361
362
```

</td><td>

```cpp
		/// <summary>
		/// 描画の完了待ち。
		/// </summary>
		void WaitDraw();
```

</td></tr></table>

### 3-2. 同じ宣言を public の最後に足す

`EndGPUEvent()` の `#endif` のすぐ下、最初の `private:` の上です。

[GraphicsEngine.h](../../../k2EngineLow/graphics/GraphicsEngine.h) の **311行目** `#endif` の下に、4行足します（312〜315行目になります）。

<!-- add: Action/k2EngineLow/graphics/GraphicsEngine.h 312 -->
<table class="code-lines"><tr><td>

```cpp
312
313
314
315
```

</td><td>

```cpp
		/// <summary>
		/// 描画の完了待ち。
		/// </summary>
		void WaitDraw();
```

</td></tr></table>

**確かめ方:** `k2EngineLow` のビルドが通る。

---

## 手順4: DebugUI を作り、起動と終了を入れる

ImGui を直接さわるのは `DebugUI` だけにします。まずは起動（`Init`）と終了（`Finalize`）だけを作り、エンジンの起動と終了から呼びます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) | 新しく作る（`Init()`・`Finalize()` の宣言） |
| [k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h) | 追加（`#include` 1行） |
| [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) | 新しく作る（`Init()`・`Finalize()` の中身） |
| [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) | 追加（起動と終了から呼ぶ） |

`k2EngineLow` の `dbg` フィルターを右クリック →「追加 → 新しい項目」で、`DebugUI.h` と `DebugUI.cpp` を作ります。**場所は `Action/k2EngineLow/dbg/` にします**（何もしないと `k2EngineLow` の直下に作られる）。

### 4-1. DebugUI の名前空間を作る

ゲームのどこからでも使えるように、クラスではなく名前空間にします（持っておく状態は `.cpp` の中に隠す）。

[DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) を新しく作り、次の6行を書きます（1〜6行目）。

<!-- new: Action/k2EngineLow/dbg/DebugUI.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
```

</td><td>

```cpp
#pragma once

namespace nsK2EngineLow {
	namespace DebugUI {
	}
}
```

</td></tr></table>

### 4-2. 起動と終了を宣言する

[DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) の **4行目** `namespace DebugUI {` の下に、8行足します（5〜12行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.h 5 -->
<table class="code-lines"><tr><td>

```cpp
5
6
7
8
9
10
11
12
```

</td><td>

```cpp
		/// <summary>
		/// ImGui を起動する。グラフィックエンジンを作ったあとに1回だけ呼ぶ。
		/// </summary>
		void Init(HWND hwnd);
		/// <summary>
		/// ImGui を終了する。グラフィックエンジンを消す前に呼ぶ。
		/// </summary>
		void Finalize();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 8 | ImGui を起動する。グラフィックエンジンを作ったあとに1回だけ呼ぶ |
| 12 | ImGui を終了する。グラフィックエンジンを消す前に呼ぶ |

### 4-3. 使い方のコメントを書く

[DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) の **3行目** `namespace nsK2EngineLow {` の下に、7行足します（4〜10行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.h 4 -->
<table class="code-lines"><tr><td>

```cpp
4
5
6
7
8
9
10
```

</td><td>

```cpp
	/// <summary>
	/// ImGui を使った開発用の画面（Debug・Preview ビルドだけ）。F1 キーで出す・隠すを切り替える。
	/// </summary>
	/// <remarks>
	/// ImGui の関数（ImGui::Begin など）は、IsVisible() が true のときに、
	/// ゲームオブジェクトの Update() の中で呼ぶ。BeginFrame() と Render() の間なので、そのまま描かれる。
	/// </remarks>
```

</td></tr></table>

### 4-4. エンジン全体から見えるようにする

Game の `stdafx.h` もこのファイルを読んでいるので、Game からも `DebugUI::` と書けるようになります。

[k2EngineLowPreCompile.h](../../../k2EngineLow/k2EngineLowPreCompile.h) の **49行目** `#include "dbg/MyLog.h"` の下に、1行足します（50〜50行目になります）。

<!-- add: Action/k2EngineLow/k2EngineLowPreCompile.h 50 -->
<table class="code-lines"><tr><td>

```cpp
50
```

</td><td>

```cpp
#include "dbg/DebugUI.h"
```

</td></tr></table>

### 4-5. DebugUI.cpp を作り、ImGui を読み込む

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) を新しく作り、次の13行を書きます（1〜13行目）。

<!-- new: Action/k2EngineLow/dbg/DebugUI.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
```

</td><td>

```cpp
#include "k2EngineLowPreCompile.h"
#include "dbg/DebugUI.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

namespace nsK2EngineLow {
	namespace DebugUI {
		namespace {
		}

	}
}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 3〜5 | ImGui 本体と、Win32・DirectX12 のバックエンド |
| 9 | 名前のない `namespace` の中は、このファイルの中でだけ使える |

### 4-6. 使う値を決める

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **9行目** `namespace {` の下に、10行足します（10〜19行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 10 -->
<table class="code-lines"><tr><td>

```cpp
10
11
12
13
14
15
16
17
18
19
```

</td><td>

```cpp
#ifdef K2_DEBUG
			constexpr bool IS_ENABLED = true;			// Debug・Preview ビルドだけ ImGui を動かす。
#else
			constexpr bool IS_ENABLED = false;
#endif
			constexpr int SRV_HEAP_SIZE = 64;			// ImGui のテクスチャ（文字の画像など）に使うディスクリプタの数。
			constexpr int NUM_FRAMES_IN_FLIGHT = 2;		// フレームバッファの数（FrameBuffer::FRAME_BUFFER_COUNT）と同じにする。
			constexpr float FONT_SIZE = 18.0f;			// 文字の大きさ（ピクセル）。
			constexpr const char* FONT_FILE_PATH = "C:\\Windows\\Fonts\\meiryo.ttc";	// 日本語を出すためのフォント。

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 10〜14 | Debug・Preview ビルドだけ ImGui を動かす |
| 15 | ImGui のテクスチャ（文字の画像など）に使うディスクリプタの数 |
| 16 | フレームバッファの数と同じにする |
| 17〜18 | 日本語を出すためのフォントと、文字の大きさ |

### 4-7. 持っておく状態を足す

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **19行目**（`constexpr const char* FONT_FILE_PATH = "C:\\Windows\\Fonts\\meiryo.ttc";	// 日本語を出すためのフォント。` の下の空の行）の下、`}` の上に、7行足します（20〜26行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 20 -->
<table class="code-lines"><tr><td>

```cpp
20
21
22
23
24
25
26
```

</td><td>

```cpp
			bool g_isInitialized = false;				// Init() が成功したか。
			bool g_isVisible = false;					// 開発用の画面を出しているか。
			bool g_isShowDemoWindow = false;			// ImGui のデモの画面を出しているか。
			ID3D12DescriptorHeap* g_srvHeap = nullptr;	// ImGui 専用のディスクリプタヒープ。
			UINT g_srvIncrementSize = 0;				// ディスクリプタ1つ分の大きさ。
			std::vector<int> g_freeSrvIndices;			// 空いているディスクリプタの番号。

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 20 | `Init()` が成功したか |
| 21〜22 | 開発用の画面・ImGui のデモを出しているか（手順6で使う） |
| 23〜25 | ImGui 専用のディスクリプタヒープと、その空き |

### 4-8. ImGui にディスクリプタを渡す関数を書く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **26行目**（`std::vector<int> g_freeSrvIndices;			// 空いているディスクリプタの番号。` の下の空の行）の下、`}` の上に、10行足します（27〜36行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 27 -->
<table class="code-lines"><tr><td>

```cpp
27
28
29
30
31
32
33
34
35
36
```

</td><td>

```cpp
			// ImGui がテクスチャを作るときに呼ばれ、空いているディスクリプタを1つ渡す。
			void AllocSrv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
			{
				IM_ASSERT(!g_freeSrvIndices.empty());
				const int index = g_freeSrvIndices.back();
				g_freeSrvIndices.pop_back();
				outCpuHandle->ptr = g_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<SIZE_T>(index) * g_srvIncrementSize;
				outGpuHandle->ptr = g_srvHeap->GetGPUDescriptorHandleForHeapStart().ptr + static_cast<UINT64>(index) * g_srvIncrementSize;
			}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 30〜32 | 空いている番号を1つ取り出す |
| 33〜34 | その番号の場所を、CPU・GPU から見たアドレスにして渡す |

### 4-9. ディスクリプタを空きに戻す関数を書く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **36行目**（`}` の下の空の行）の下、`}` の上に、7行足します（37〜43行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 37 -->
<table class="code-lines"><tr><td>

```cpp
37
38
39
40
41
42
43
```

</td><td>

```cpp
			// ImGui がテクスチャを消すときに呼ばれ、ディスクリプタを空きに戻す。
			void FreeSrv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE)
			{
				const SIZE_T offset = cpuHandle.ptr - g_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
				g_freeSrvIndices.push_back(static_cast<int>(offset / g_srvIncrementSize));
			}

```

</td></tr></table>

### 4-10. 起動する関数の形を書く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **45行目**（`}` の下の空の行）の下、`}` の上に、10行足します（46〜55行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 46 -->
<table class="code-lines"><tr><td>

```cpp
46
47
48
49
50
51
52
53
54
55
```

</td><td>

```cpp
		void Init(HWND hwnd)
		{
			if (!IS_ENABLED || g_graphicsEngine == nullptr) {
				return;
			}
			ID3D12Device* device = g_graphicsEngine->GetD3DDevice();

			g_isInitialized = true;
		}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 48〜50 | Release ビルドか、グラフィックエンジンがなければ何もしない |
| 51 | DirectX12 のデバイス |

### 4-11. ImGui 専用のディスクリプタヒープを作る

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **52行目**（`ID3D12Device* device = g_graphicsEngine->GetD3DDevice();` の下の空の行）の下、`g_isInitialized = true;` の上に、8行足します（53〜60行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 53 -->
<table class="code-lines"><tr><td>

```cpp
53
54
55
56
57
58
59
60
```

</td><td>

```cpp
			// ImGui 専用のディスクリプタヒープを作る。エンジンのヒープとは分けて、互いに番号を気にしなくて済むようにする。
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.NumDescriptors = SRV_HEAP_SIZE;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_srvHeap)))) {
				return;
			}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 53〜60 | エンジンのヒープとは分けて、お互いに番号を気にしなくて済むようにする |

### 4-12. 空いている番号の一覧を作る

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **60行目** `}` の下、`g_isInitialized = true;` の上に、5行足します（61〜65行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 61 -->
<table class="code-lines"><tr><td>

```cpp
61
62
63
64
65
```

</td><td>

```cpp
			g_srvIncrementSize = device->GetDescriptorHandleIncrementSize(heapDesc.Type);
			for (int i = SRV_HEAP_SIZE - 1; i >= 0; i--) {
				g_freeSrvIndices.push_back(i);
			}

```

</td></tr></table>

### 4-13. ImGui を作り、フォントと色を決める

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **65行目**（`}` の下の空の行）の下、`g_isInitialized = true;` の上に、11行足します（66〜76行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 66 -->
<table class="code-lines"><tr><td>

```cpp
66
67
68
69
70
71
72
73
74
75
76
```

</td><td>

```cpp
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

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 69〜70 | `imgui.ini`（窓の位置の記録）を作らない。ゲームのフォルダーにファイルが増えないように |
| 71〜74 | 日本語のフォントがあれば使う。なければ英数字だけになる |

### 4-14. Win32 のバックエンドと、DirectX12 の設定を始める

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **76行目**（`ImGui::StyleColorsDark();` の下の空の行）の下、`g_isInitialized = true;` の上に、7行足します（77〜83行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 77 -->
<table class="code-lines"><tr><td>

```cpp
77
78
79
80
81
82
83
```

</td><td>

```cpp
			ImGui_ImplWin32_Init(hwnd);

			ImGui_ImplDX12_InitInfo initInfo;
			initInfo.Device = device;
			initInfo.CommandQueue = g_graphicsEngine->GetCommandQueue();
			initInfo.NumFramesInFlight = NUM_FRAMES_IN_FLIGHT;
			initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;	// フレームバッファの色の形式（FrameBuffer.cpp）。
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 83 | フレームバッファの色の形式（`FrameBuffer.cpp` と同じ） |

### 4-15. DirectX12 のバックエンドを起動する

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **83行目** `initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;	// フレームバッファの色の形式（FrameBuffer.cpp）。` の下に、6行足します（84〜89行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 84 -->
<table class="code-lines"><tr><td>

```cpp
84
85
86
87
88
89
```

</td><td>

```cpp
			initInfo.DSVFormat = DXGI_FORMAT_D32_FLOAT;		// フレームバッファの深度の形式（FrameBuffer.cpp）。
			initInfo.SrvDescriptorHeap = g_srvHeap;
			initInfo.SrvDescriptorAllocFn = AllocSrv;
			initInfo.SrvDescriptorFreeFn = FreeSrv;
			ImGui_ImplDX12_Init(&initInfo);

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 84 | フレームバッファの深度の形式 |
| 85〜87 | 4-8・4-9 の関数で、ディスクリプタを受け渡す |

### 4-16. 終了する関数を書き、ImGui を止める

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **92行目**（`}` の下の空の行）の下、`}` の上に、10行足します（93〜102行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 93 -->
<table class="code-lines"><tr><td>

```cpp
93
94
95
96
97
98
99
100
101
102
```

</td><td>

```cpp
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
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 99 | GPU が ImGui のバッファを使い終わるまで待つ（手順3で `public` にした関数） |

### 4-17. ディスクリプタヒープを消す

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **102行目** `ImGui::DestroyContext();` の下に、6行足します（103〜108行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 103 -->
<table class="code-lines"><tr><td>

```cpp
103
104
105
106
107
108
```

</td><td>

```cpp
			g_srvHeap->Release();
			g_srvHeap = nullptr;
			g_freeSrvIndices.clear();
			g_isInitialized = false;
		}

```

</td></tr></table>

### 4-18. エンジンのデストラクターで終了する

ここから [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) です。

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の **10行目** `{` の下、`// グローバルなアクセスポイントにnullptrを代入。` の上に、2行足します（11〜12行目になります）。

<!-- add: Action/k2EngineLow/k2EngineLow.cpp 11 -->
<table class="code-lines"><tr><td>

```cpp
11
12
```

</td><td>

```cpp
		// ImGui はグラフィックエンジンを使っているので、先に終了する。
		DebugUI::Finalize();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 12 | ImGui はグラフィックエンジンを使っているので、先に終了する |

### 4-19. グラフィックエンジンを作ったあとに起動する

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の **41行目** `);` の下に、2行足します（42〜43行目になります）。

<!-- add: Action/k2EngineLow/k2EngineLow.cpp 42 -->
<table class="code-lines"><tr><td>

```cpp
42
43
```

</td><td>

```cpp
			// 開発用の画面（ImGui）を起動する。
			DebugUI::Init(hwnd);
```

</td></tr></table>

**確かめ方:** ゲームを起動して終了しても落ちない（まだ何も描かない）。

---

## 手順5: ウィンドウのメッセージを ImGui に渡す

ImGui がマウスやキーを受け取れるように、ウィンドウに来たメッセージを渡します。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) | 追加（`HandleWindowMessage()` の宣言） |
| [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) | 追加（宣言と、`HandleWindowMessage()` の中身） |
| [system.cpp](../../../Game/Source/System/system.cpp) | 追加（`MsgProc()` の先頭で渡す） |

### 5-1. メッセージを渡す関数を宣言する

[DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) の **19行目** `void Finalize();` の下に、4行足します（20〜23行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.h 20 -->
<table class="code-lines"><tr><td>

```cpp
20
21
22
23
```

</td><td>

```cpp
		/// <summary>
		/// ウィンドウのメッセージを ImGui に渡す。true が返ったら、ImGui が使ったメッセージ。
		/// </summary>
		bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
```

</td></tr></table>

### 5-2. ImGui のメッセージを受け取る関数を宣言する

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **6行目**（`#include <imgui/imgui_impl_dx12.h>` の下の空の行）の下、`namespace nsK2EngineLow {` の上に、3行足します（7〜9行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 7 -->
<table class="code-lines"><tr><td>

```cpp
7
8
9
```

</td><td>

```cpp
// imgui_impl_win32.h では、windows.h に頼らないために宣言が #if 0 で外してあるので、ここで宣言する。
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 7〜8 | `imgui_impl_win32.h` では、`windows.h` に頼らないために宣言が `#if 0` で外してあるので、ここで宣言する |

### 5-3. メッセージを渡す関数を書く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **111行目**（`}` の下の空の行）の下、`}` の上に、7行足します（112〜118行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 112 -->
<table class="code-lines"><tr><td>

```cpp
112
113
114
115
116
117
118
```

</td><td>

```cpp
		bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (!g_isInitialized) {
				return false;
			}
			return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) != 0;
		}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 117 | ImGui が使ったメッセージなら `true` |

### 5-4. ウィンドウのメッセージを ImGui に渡す

[system.cpp](../../../Game/Source/System/system.cpp) の `MsgProc()` の先頭（`switch (msg)` より前）に足します。

[system.cpp](../../../Game/Source/System/system.cpp) の **16行目** `{` の下、`//送られてきたメッセージで処理を分岐させる。` の上に、4行足します（17〜20行目になります）。

<!-- add: Action/Game/Source/System/system.cpp 17 -->
<table class="code-lines"><tr><td>

```cpp
17
18
19
20
```

</td><td>

```cpp
	// 開発用の画面（ImGui）が使ったメッセージは、ゲームでは使わない。
	if (DebugUI::HandleWindowMessage(hWnd, msg, wParam, lParam)) {
		return true;
	}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 18〜20 | ImGui が使ったメッセージは、ゲームでは使わない |

**確かめ方:** ソリューションのビルドが通る。

---

## 手順6: 毎フレーム ImGui を受け付けて描く

フレームの始めに ImGui の受け付けを始め、フレームの終わり（画面に出す直前）に描きます。その間、ゲームの `Update()` で `ImGui::Begin()` などを呼べます。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) | 追加（`BeginFrame()`・`Render()`・`IsVisible()` の宣言） |
| [DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) | 追加（DebugUI の窓、3つの関数の中身） |
| [k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) | 追加（フレームの始めと終わりで呼ぶ） |

### 6-1. フレームの始めと終わりの関数を宣言する

[DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) の **19行目** `void Finalize();` の下に、8行足します（20〜27行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.h 20 -->
<table class="code-lines"><tr><td>

```cpp
20
21
22
23
24
25
26
27
```

</td><td>

```cpp
		/// <summary>
		/// フレームの始めに呼ぶ。ImGui の受け付けを始め、F1 で出す・隠すを切り替える。
		/// </summary>
		void BeginFrame();
		/// <summary>
		/// フレームの終わり（画面に出す直前）に呼ぶ。ImGui の画面をフレームバッファに描く。
		/// </summary>
		void Render();
```

</td></tr></table>

### 6-2. 出しているかを返す関数を宣言する

[DebugUI.h](../../../k2EngineLow/dbg/DebugUI.h) の **27行目** `void Render();` の下に、4行足します（28〜31行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.h 28 -->
<table class="code-lines"><tr><td>

```cpp
28
29
30
31
```

</td><td>

```cpp
		/// <summary>
		/// 開発用の画面を出しているか。出している間は、マウスを ImGui に使う。
		/// </summary>
		bool IsVisible();
```

</td></tr></table>

### 6-3. DebugUI の窓を描く関数を書く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **46行目**（`}` の下の空の行）の下、`}` の上に、12行足します（47〜58行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 47 -->
<table class="code-lines"><tr><td>

```cpp
47
48
49
50
51
52
53
54
55
56
57
58
```

</td><td>

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

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 50〜54 | 「DebugUI」の窓に、閉じ方・FPS・デモを出すかのチェックを出す |
| 55〜57 | チェックが入っていれば、ImGui のデモの窓を出す |

### 6-4. フレームの始めの関数を書く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **123行目**（`}` の下の空の行）の下、`bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)` の上に、11行足します（124〜134行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 124 -->
<table class="code-lines"><tr><td>

```cpp
124
125
126
127
128
129
130
131
132
133
134
```

</td><td>

```cpp
		void BeginFrame()
		{
			if (!g_isInitialized) {
				return;
			}
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

		}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 129〜131 | DirectX12・Win32・ImGui の順に、新しいフレームを始める |

### 6-5. F1 で出す・隠すを切り替える

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **132行目**（`ImGui::NewFrame();` の下の空の行）の下、`}` の上に、6行足します（133〜138行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 133 -->
<table class="code-lines"><tr><td>

```cpp
133
134
135
136
137
138
```

</td><td>

```cpp
			if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
				g_isVisible = !g_isVisible;
			}
			if (g_isVisible) {
				DrawDebugUIWindow();
			}
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 133〜135 | F1 を押した瞬間に切り替える |
| 136〜138 | 出しているときは、6-3 の窓を描く |

### 6-6. フレームの終わりの関数を書く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **140行目**（`}` の下の空の行）の下、`bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)` の上に、9行足します（141〜149行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 141 -->
<table class="code-lines"><tr><td>

```cpp
141
142
143
144
145
146
147
148
149
```

</td><td>

```cpp
		void Render()
		{
			if (!g_isInitialized) {
				return;
			}
			// 隠しているときも毎フレーム描く処理を通す。何も描かれないが、ImGui がテクスチャを作る・消す処理がここで行われるため。
			ImGui::Render();
		}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 146〜147 | 隠しているときも、毎フレーム描く処理を通す。何も描かれないが、ImGui がテクスチャを片付けるため |

### 6-7. フレームバッファに描く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **147行目** `ImGui::Render();` の下に、8行足します（148〜155行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 148 -->
<table class="code-lines"><tr><td>

```cpp
148
149
150
151
152
153
154
155
```

</td><td>

```cpp

			ID3D12GraphicsCommandList* commandList = g_graphicsEngine->GetCommandList();
			const D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_graphicsEngine->GetCurrentFrameBuffuerRTV();
			const D3D12_CPU_DESCRIPTOR_HANDLE dsv = g_graphicsEngine->GetCurrentFrameBuffuerDSV();
			commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
			ID3D12DescriptorHeap* heaps[] = { g_srvHeap };
			commandList->SetDescriptorHeaps(1, heaps);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 150〜152 | 今のフレームバッファを描く先にする |
| 153〜154 | ImGui 専用のディスクリプタヒープを使う |
| 155 | ImGui の絵を描く |

### 6-8. 出しているかを返す関数を書く

[DebugUI.cpp](../../../k2EngineLow/dbg/DebugUI.cpp) の **157行目**（`}` の下の空の行）の下、`bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)` の上に、5行足します（158〜162行目になります）。

<!-- add: Action/k2EngineLow/dbg/DebugUI.cpp 158 -->
<table class="code-lines"><tr><td>

```cpp
158
159
160
161
162
```

</td><td>

```cpp
		bool IsVisible()
		{
			return g_isInitialized && g_isVisible;
		}

```

</td></tr></table>

### 6-9. エンジンのフレームの始めで受け付けを始める

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の **70行目** `m_graphicsEngine->BeginRender();` の下に、2行足します（71〜72行目になります）。

<!-- add: Action/k2EngineLow/k2EngineLow.cpp 71 -->
<table class="code-lines"><tr><td>

```cpp
71
72
```

</td><td>

```cpp
		// ImGui の受け付けを始める。ここから EndFrame() までの間、ゲームの Update で ImGui の関数を呼べる。
		DebugUI::BeginFrame();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 72 | ここから `EndFrame()` までの間、ゲームの `Update()` で ImGui の関数を呼べる |

### 6-10. エンジンのフレームの終わりで描く

[k2EngineLow.cpp](../../../k2EngineLow/k2EngineLow.cpp) の **96行目** `//#endif` の下に、2行足します（97〜98行目になります）。

<!-- add: Action/k2EngineLow/k2EngineLow.cpp 97 -->
<table class="code-lines"><tr><td>

```cpp
97
98
```

</td><td>

```cpp
		// ImGui の画面を、ゲームの絵の上に描く。
		DebugUI::Render();
```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 98 | ゲームの絵の上に描く |

### 出来上がった DebugUI.h

<!-- full: Action/k2EngineLow/dbg/DebugUI.h -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
```

</td><td>

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
		/// <summary>
		/// ウィンドウのメッセージを ImGui に渡す。true が返ったら、ImGui が使ったメッセージ。
		/// </summary>
		bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	}
}
```

</td></tr></table>

### 出来上がった DebugUI.cpp

<!-- full: Action/k2EngineLow/dbg/DebugUI.cpp -->
<table class="code-lines"><tr><td>

```cpp
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
50
51
52
53
54
55
56
57
58
59
60
61
62
63
64
65
66
67
68
69
70
71
72
73
74
75
76
77
78
79
80
81
82
83
84
85
86
87
88
89
90
91
92
93
94
95
96
97
98
99
100
101
102
103
104
105
106
107
108
109
110
111
112
113
114
115
116
117
118
119
120
121
122
123
124
125
126
127
128
129
130
131
132
133
134
135
136
137
138
139
140
141
142
143
144
145
146
147
148
149
150
151
152
153
154
155
156
157
158
159
160
161
162
163
164
165
166
167
168
169
170
171
```

</td><td>

```cpp
#include "k2EngineLowPreCompile.h"
#include "dbg/DebugUI.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

// imgui_impl_win32.h では、windows.h に頼らないために宣言が #if 0 で外してあるので、ここで宣言する。
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
			bool g_isVisible = false;					// 開発用の画面を出しているか。
			bool g_isShowDemoWindow = false;			// ImGui のデモの画面を出しているか。
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

		bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (!g_isInitialized) {
				return false;
			}
			return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) != 0;
		}
	}
}
```

</td></tr></table>

**確かめ方:** 起動して F1 を押すと、「DebugUI」の窓が出る。もう一度押すと消える。

---

## 手順7: 窓を出している間はマウスを ImGui に使う

窓を出している間は、マウスでカメラが回ったり、クリックで魔法が出たりしないようにします。

**この手順で触るファイル:**

| ファイル | 何をするか |
|---|---|
| [GameCamera.cpp](../../../Game/Source/Objects/GameCamera.cpp) | 追加（窓を出している間はマウスで回さない） |
| [PlayerInputSource.h](../../../Game/Source/Character/Input/PlayerInputSource.h) | 追加（マウスを使ってよいかの宣言） |
| [PlayerInputSource.cpp](../../../Game/Source/Character/Input/PlayerInputSource.cpp) | 追加（マウスを使ってよいか）・書き換え（クリックを見る2行） |

### 7-1. 窓を出している間は、マウスでカメラを回さない

[GameCamera.cpp](../../../Game/Source/Objects/GameCamera.cpp) の `TryGetMouseRotationInput()` で、ウィンドウが前にないときに止める `if` の下に足します。

[GameCamera.cpp](../../../Game/Source/Objects/GameCamera.cpp) の **258行目**（`}` の下の空の行）の下、`RECT clientRect;` の上に、6行足します（259〜264行目になります）。

<!-- add: Action/Game/Source/Objects/GameCamera.cpp 259 -->
<table class="code-lines"><tr><td>

```cpp
259
260
261
262
263
264
```

</td><td>

```cpp
	// 開発用の画面（ImGui）を出している間は、マウスを ImGui に使うので、カメラは回さない。
	if (DebugUI::IsVisible()) {
		EndMouseLook();
		return false;
	}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 260〜263 | 窓を出している間は、マウスで回すのをやめる（`EndMouseLook()` がカーソルを出し直す） |

### 7-2. マウスを使ってよいかの関数を宣言する

[PlayerInputSource.h](../../../Game/Source/Character/Input/PlayerInputSource.h) の **23行目** `static bool IsGameWindowActive();` の下に、2行足します（24〜25行目になります）。

<!-- add: Action/Game/Source/Character/Input/PlayerInputSource.h 24 -->
<table class="code-lines"><tr><td>

```cpp
24
25
```

</td><td>

```cpp
	// マウスのボタンをゲームの操作に使ってよいか（ウィンドウが前にあり、開発用の画面を出していない）。
	static bool CanUseMouse();
```

</td></tr></table>

### 7-3. マウスを使ってよいかの関数を書く

[PlayerInputSource.cpp](../../../Game/Source/Character/Input/PlayerInputSource.cpp) の **58行目**（`}` の下の空の行）の下、`bool PlayerInputSource::IsKeyPressed(int virtualKey)` の上に、6行足します（59〜64行目になります）。

<!-- add: Action/Game/Source/Character/Input/PlayerInputSource.cpp 59 -->
<table class="code-lines"><tr><td>

```cpp
59
60
61
62
63
64
```

</td><td>

```cpp
bool PlayerInputSource::CanUseMouse()
{
	// 開発用の画面（ImGui）を出している間は、クリックを ImGui に使うので、魔法を撃たない。
	return IsGameWindowActive() && !DebugUI::IsVisible();
}

```

</td></tr></table>

| 行 | 何をしているか |
|---|---|
| 62 | ゲームのウィンドウが前にあり、窓を出していなければ使ってよい |

### 7-4. 左クリックを見る行を書き換える

[PlayerInputSource.cpp](../../../Game/Source/Character/Input/PlayerInputSource.cpp) の **32行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/Game/Source/Character/Input/PlayerInputSource.cpp 32 -->
<table class="code-lines"><tr><td>

```cpp
32
```

</td><td>

```cpp
	input.fire.Update((IsGameWindowActive() && IsKeyPressed(VK_LBUTTON)) || pad.IsPress(enButtonY));
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/Game/Source/Character/Input/PlayerInputSource.cpp 32 -->
<table class="code-lines"><tr><td>

```cpp
32
```

</td><td>

```cpp
	input.fire.Update((CanUseMouse() && IsKeyPressed(VK_LBUTTON)) || pad.IsPress(enButtonY));
```

</td></tr></table>

### 7-5. 右クリックを見る行を書き換える

[PlayerInputSource.cpp](../../../Game/Source/Character/Input/PlayerInputSource.cpp) の **34行目**を、次のように書き換えます。

**前**

<!-- rewrite-before: Action/Game/Source/Character/Input/PlayerInputSource.cpp 34 -->
<table class="code-lines"><tr><td>

```cpp
34
```

</td><td>

```cpp
	input.subFire.Update((IsGameWindowActive() && IsKeyPressed(VK_RBUTTON)) || pad.IsPress(enButtonRB2));
```

</td></tr></table>

**後**

<!-- rewrite-after: Action/Game/Source/Character/Input/PlayerInputSource.cpp 34 -->
<table class="code-lines"><tr><td>

```cpp
34
```

</td><td>

```cpp
	input.subFire.Update((CanUseMouse() && IsKeyPressed(VK_RBUTTON)) || pad.IsPress(enButtonRB2));
```

</td></tr></table>

**確かめ方（これで出来上がり）:** F1 で窓を出している間は、マウスを動かしてもカメラが回らず、クリックしても魔法が出ない。窓のチェックボックスをクリックで操作できる。

## ゲームから窓を足すとき

ここまでで仕組みは出来上がりです。調整用の窓を足す方法は、[デバッグUIの設計](デバッグUIの設計.md) の「3. 使い方」を見てください。
