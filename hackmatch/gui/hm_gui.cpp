#include <filesystem>

#include "../hm_helper.h"
#include "../base/colors.h"

#include "../imgui/imgui_internal.h"
#include "hm_gui.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool wasClipped = false;
	static bool wasInvisible = false;
	if (hmHelper->m_pConfig->menu.enabled)
	{
		// Make mouse movable and visible
		RECT rectCheck;
		if(GetClipCursor(&rectCheck))
		{
			wasClipped = true;
			ClipCursor(NULL);
		}

		int displayCount = ShowCursor(TRUE); // Increment display count
		ShowCursor(FALSE); // Decrement it back to its original value
		if(displayCount <= 0)
		{
			wasInvisible = true;
			ShowCursor(TRUE);
		}

		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		return true;
	}
	if (wasClipped)
	{
		// RECT rect;
		// GetWindowRect(hmHelper->m_pGui->window, &rect);
		// ClipCursor(&rect);
		wasClipped = false;
	}

	if (wasInvisible)
	{
		ShowCursor(FALSE);
		wasInvisible = false;
	}

	switch (uMsg)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	}

	return CallWindowProc(hmHelper->m_pGui->oWndProc, hWnd, uMsg, wParam, lParam);
}

// Main
HRESULT __stdcall HMGui::hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!hmHelper->m_pGui->initDx)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&hmHelper->m_pGui->pDevice)))
		{
			hmHelper->m_pGui->pDevice->GetImmediateContext(&hmHelper->m_pGui->pContext);

			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			hmHelper->m_pGui->window = sd.OutputWindow;

			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

			hmHelper->m_pGui->pDevice->CreateRenderTargetView(pBackBuffer, NULL, &hmHelper->m_pGui->mainRenderTargetView);

			pBackBuffer->Release();

			hmHelper->m_pGui->oWndProc = (WNDPROC)SetWindowLongPtr(hmHelper->m_pGui->window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			hmHelper->m_pGui->InitImGui();
			hmHelper->m_pGui->initDx = true;
		}
		else
			return hmHelper->m_pGui->oPresent(pSwapChain, SyncInterval, Flags);
	}

	// Save important fields, so we can reset on disable/unload
	static bool savedFields = false;
	if (!savedFields)
		savedFields = hmHelper->m_pHacks->SaveFields();

	hmHelper->m_pConfig->Hotkeys();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	hmHelper->m_pGui->RenderImGui();

	ImGui::Render();

	// Here clear the player array
	hmHelper->m_pSdk->playersMutex.lock();
	hmHelper->m_pSdk->players.clear();
	hmHelper->m_pSdk->playersMutex.unlock();

	hmHelper->m_pGui->pContext->OMSetRenderTargets(1, &hmHelper->m_pGui->mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Handle unloading
	if (!hmHelper->m_pConfig->menu.injected)
	{
		kiero::shutdown();
		hmHelper->m_pGui->pDevice->Release();
		hmHelper->m_pGui->pContext->Release();
		pSwapChain->Release();
		hmHelper->m_pGui->oWndProc = (WNDPROC)SetWindowLongPtr(hmHelper->m_pGui->window, GWLP_WNDPROC, (LONG_PTR)(hmHelper->m_pGui->oWndProc));
		hmHelper->m_pGui->oPresent(pSwapChain, SyncInterval, Flags);
		return 0;
	}

	return hmHelper->m_pGui->oPresent(pSwapChain, SyncInterval, Flags);
}


// ImGui
void HMGui::InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	io.IniFilename = nullptr;

	SetupImGuiFonts();
	SetupImGuiStyle();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

void HMGui::RenderImGui()
{
	hmHelper->m_pGui->RenderMainWindow();
	hmHelper->m_pEsp->RenderEsp();
}
