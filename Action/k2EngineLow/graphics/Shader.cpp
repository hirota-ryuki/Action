#include "k2EngineLowPreCompile.h"
#include "Shader.h"
#include <stierr.h>
#include <sstream>
#include <fstream>
#include <atlbase.h>

namespace nsK2EngineLow {


	namespace {
		const char* g_vsShaderModelName = "vs_5_0";	//頂点シェーダーのシェーダーモデル名。
		const char* g_psShaderModelName = "ps_5_0";	//ピクセルシェーダーのシェーダモデル名。
		const char* g_csShaderModelName = "cs_5_0";	//コンピュートシェーダーのシェーダーモデル名。
	}
	Shader::~Shader()
	{
		Release();
	}
	void Shader::Release()
	{
		ReleaseD3D12Object(m_blob);
	}
	void Shader::Load(const char* filePath, const char* entryFuncName, const char* shaderModel)
	{
		Release();
		ID3DBlob* errorBlob;
#ifdef K2_DEBUG
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		wchar_t wfxFilePath[256] = { L"" };
		mbstowcs(wfxFilePath, filePath, 256);

		auto hr = D3DCompileFromFile(wfxFilePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryFuncName, shaderModel, compileFlags, 0, &m_blob, &errorBlob);

		if (FAILED(hr)) {
			if (hr == STIERR_OBJECTNOTFOUND) {
				std::wstring errorMessage;
				errorMessage = L"指定されたfxファイルが開けませんでした。";
				errorMessage += wfxFilePath;
				MessageBoxW(nullptr, errorMessage.c_str(), L"エラー", MB_OK);
			}
			if (errorBlob) {
				static char errorMessage[10 * 1024];
				sprintf_s(errorMessage, "filePath : %ws, %s", wfxFilePath, (char*)errorBlob->GetBufferPointer());
				MessageBoxA(NULL, errorMessage, "シェーダーコンパイルエラー", MB_OK);
				return;
			}
		}
		m_isInited = true;
	}
	void Shader::LoadPS(const char* filePath, const char* entryFuncName)
	{
		Load(filePath, entryFuncName, g_psShaderModelName);
	}
	void Shader::LoadVS(const char* filePath, const char* entryFuncName)
	{
		Load(filePath, entryFuncName, g_vsShaderModelName);
	}
	void Shader::LoadCS(const char* filePath, const char* entryFuncName)
	{
		Load(filePath, entryFuncName, g_csShaderModelName);
	}
}
