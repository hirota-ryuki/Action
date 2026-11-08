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
