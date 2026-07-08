#include "k2EngineLowPreCompile.h"
#include "ImGuiRenderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

namespace nsK2EngineLow {
	ImGuiRenderer::~ImGuiRenderer()
	{
		Shutdown();
	}

	bool ImGuiRenderer::Init(HWND hwnd, ID3D12Device* device, int numFrameInFlight, DXGI_FORMAT rtvFormat)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvDescriptorHeap)))) {
			return false;
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		if (!ImGui_ImplWin32_Init(hwnd)) {
			return false;
		}
		if (!ImGui_ImplDX12_Init(
			device,
			numFrameInFlight,
			rtvFormat,
			m_srvDescriptorHeap,
			m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart())) {
			return false;
		}

		m_isInited = true;
		return true;
	}

	void ImGuiRenderer::Shutdown()
	{
		if (!m_isInited) {
			return;
		}

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		if (m_srvDescriptorHeap != nullptr) {
			m_srvDescriptorHeap->Release();
			m_srvDescriptorHeap = nullptr;
		}
		m_isInited = false;
	}

	void ImGuiRenderer::NewFrame()
	{
		if (!m_isInited) {
			return;
		}

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiRenderer::Render(ID3D12GraphicsCommandList* commandList)
	{
		if (!m_isInited) {
			return;
		}

		ImGui::Render();
		commandList->SetDescriptorHeaps(1, &m_srvDescriptorHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	}

	LRESULT ImGuiRenderer::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	}
}
