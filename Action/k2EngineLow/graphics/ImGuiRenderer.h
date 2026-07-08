#pragma once

#include <d3d12.h>

namespace nsK2EngineLow {
	/// <summary>
	/// ImGuiの初期化・更新・描画をまとめて管理するクラス。
	/// </summary>
	class ImGuiRenderer : public Noncopyable {
	public:
		~ImGuiRenderer();
		/// <summary>
		/// 初期化。GraphicsEngine::Init()内で、デバイス作成後に呼び出す。
		/// </summary>
		/// <param name="hwnd">ウィンドウハンドル。</param>
		/// <param name="device">D3D12デバイス。</param>
		/// <param name="numFrameInFlight">フレームバッファの数。</param>
		/// <param name="rtvFormat">レンダーターゲットのフォーマット。</param>
		bool Init(HWND hwnd, ID3D12Device* device, int numFrameInFlight, DXGI_FORMAT rtvFormat);
		/// <summary>
		/// 破棄。
		/// </summary>
		void Shutdown();
		/// <summary>
		/// フレーム開始時に呼び出す。
		/// </summary>
		void NewFrame();
		/// <summary>
		/// フレーム終了時に呼び出す。
		/// </summary>
		/// <remarks>
		/// GraphicsEngine::EndRender()でレンダーターゲットがPRESENT状態へ遷移する前、
		/// つまりEndFrame()を呼び出すより前に呼び出す必要がある。
		/// </remarks>
		void Render(ID3D12GraphicsCommandList* commandList);
		/// <summary>
		/// Win32のWndProcから呼び出す。
		/// </summary>
		static LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	private:
		ID3D12DescriptorHeap* m_srvDescriptorHeap = nullptr;	//ImGui専用のSRVディスクリプタヒープ(フォントテクスチャ用)。
		bool m_isInited = false;
	};
}
