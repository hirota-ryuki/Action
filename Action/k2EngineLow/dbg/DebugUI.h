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
