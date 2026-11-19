#include "stdafx.h"
#include "System/system.h"

#include<InitGUID.h>
#include<dxgidebug.h>

#include "Game.h"
#include "FirstPersonArmsPoser.h"



// デバッグビルド終了時にD3D12リソースのリークを検出してログ出力する。
// dxgidebug.dll は実行時ロードが必須（静的リンク不可）。
void ReportLiveObjects()
{
	IDXGIDebug* pDxgiDebug;

	typedef HRESULT(__stdcall* fPtr)(const IID&, void**);
	HMODULE hDll = GetModuleHandleW(L"dxgidebug.dll");
	fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress(hDll, "DXGIGetDebugInterface");

	DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDxgiDebug);

	// 出力。
	pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
}

///////////////////////////////////////////////////////////////////
// ウィンドウプログラムのメイン関数。
///////////////////////////////////////////////////////////////////
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	//ゲームの初期化。
	InitGame(hInstance, hPrevInstance, lpCmdLine, nCmdShow, TEXT("Game"));
	//////////////////////////////////////
	// ここから初期化を行うコードを記述する。
	//////////////////////////////////////

#ifdef _DEBUG
	// 一人称の腕の形を焼き込む（-bakeFirstPersonArms）・焼き込んだクリップを確かめる（-verifyFirstPersonArms）ときは、
	// ゲームを始めずに、それだけをして終わる。結果は tools/FirstPerson/BakedPose/ のログに書く。開発用なので Debug ビルドだけ。
	if (wcsstr(lpCmdLine, L"-bakeFirstPersonArms") != nullptr || wcsstr(lpCmdLine, L"-verifyFirstPersonArms") != nullptr) {
		const bool isBake = wcsstr(lpCmdLine, L"-bakeFirstPersonArms") != nullptr;
		const bool isOk = isBake
			? FirstPersonArmsPoser::BakeAll("../tools/FirstPerson/BakedPose/bake_log.txt")
			: FirstPersonArmsPoser::VerifyAll("../tools/FirstPerson/BakedPose/verify_log.txt");
		K2Engine::DeleteInstance();
		return isOk ? 0 : 1;
	}
#endif

	//Gameクラスのオブジェクトを作成。
	NewGO<Game>(0, "game");

	//////////////////////////////////////
	// 初期化を行うコードを書くのはここまで！！！
	//////////////////////////////////////
	
	// ここからゲームループ。
	while (DispatchWindowMessage())
	{
		K2Engine::GetInstance()->Execute();
	}

	K2Engine::DeleteInstance();

#ifdef _DEBUG
	ReportLiveObjects();
#endif // _DEBUG
	return 0;
}

