#pragma once

#include "../includes.h"
#include <vector>
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../base/colors.h"

class HMGui
{
public:
	// Vars
	Present oPresent;
	HWND window = NULL;
	WNDPROC oWndProc;
	ID3D11Device* pDevice = NULL;
	ID3D11DeviceContext* pContext = NULL;
	ID3D11RenderTargetView* mainRenderTargetView;

	bool initDx = false;
    float animLoad = 0.f; // Used for the loading effect when pressing on a tab

	ImFont* espFont = nullptr;
	ImFont* subFont = nullptr;   // Smaller font used in: main menu section names
	ImFont* titleFont = nullptr; // Bigger font used in: titles, for example section titles

	// Main
	static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	// ImGui
	void InitImGui();
	void RenderImGui();

	// Menu
	void RenderMainWindow();
	void RenderMiscTab();
	void RenderEspTab();

    // Helpers - base
	void SetupImGuiFonts();
	void SetupImGuiStyle();

    ImFont* AddFont(float fontSize, const char* fontPath, ImFontConfig* fontCfg = nullptr);
	ImWchar* GetFontGlyphRanges() noexcept;
    ImVec2 CalculateWindowSize(float childWindowOffset, ImVec2 windows = { 2, 2 }); // Windows = how many windows in x, y

    // ImGui widgets
    bool TabButton(const char* label, const char* icon, ImGuiButtonFlags flags, const ImVec2& size_arg,
        ImU32 color);

    bool InvisibleButton(const char* str_id, const ImVec2& size_arg, const ImVec2& pos_arg, ImGuiButtonFlags flags = 0);
    bool ToggleButton(const char* label, bool* v, bool* vr = nullptr, int* hotkey = nullptr, bool* gear = nullptr, ImVec2* screenPosOut = nullptr);
    bool IconButton(const char* icon, const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags = ImGuiButtonFlags_None,
        bool useSpacing = true);

    void WCombo(const char* label, int* currentItem, const char* const items[], int size, float width = 0.f);
    void MultiCombo(const char* label, const std::vector<const char*>& titles, const std::vector<bool*>& options, float width = 0.f);
    void MultiCombo(const char* label, const std::vector<const char*>& titles, const std::vector<int>& values, int* flag, float width = 0.f);

    void ColorPickerWithText(const char* name, ColorRGBA& colRGBA) noexcept;
    void ColorPicker(const char* id, ColorRGBA& colRGBA) noexcept;
};
