#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#include "gameplay.h"

namespace hackmatch {
class Hooks {
public:
    bool init(HMODULE module);
    void shutdown();

private:
    using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    using ShotFn = void (*)(il2cpp::Object*, il2cpp::Object*, const MethodInfo*);
    using RaycastFn = bool (*)(Vector3, Vector3, float, int, int, const MethodInfo*);
    using RaycastAllFn = il2cpp::Array* (*)(Vector3, Vector3, float, int, int, const MethodInfo*);
    using RaycastNonAllocFn = int (*)(Vector3, Vector3, il2cpp::Array*, float, int, int, const MethodInfo*);
    using PlayerUpdateFn = void (*)(il2cpp::Object*, const MethodInfo*);

    static DWORD WINAPI unload_thread(void*);
    static LRESULT CALLBACK wnd_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    static HRESULT __stdcall present_hook(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags);
    static HRESULT __stdcall resize_buffers_hook(IDXGISwapChain* swap_chain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags);
    static void shot_hook_a(il2cpp::Object* player, il2cpp::Object* shot, const MethodInfo* method);
    static void shot_hook_b(il2cpp::Object* player, il2cpp::Object* shot, const MethodInfo* method);
    static bool raycast_hook(Vector3 origin, Vector3 direction, float max_distance, int layer_mask, int query_trigger_interaction, const MethodInfo* method);
    static il2cpp::Array* raycast_all_hook(Vector3 origin, Vector3 direction, float max_distance, int layer_mask, int query_trigger_interaction, const MethodInfo* method);
    static int raycast_non_alloc_hook(Vector3 origin, Vector3 direction, il2cpp::Array* results, float max_distance, int layer_mask, int query_trigger_interaction, const MethodInfo* method);
    static void player_update_hook(il2cpp::Object* player, const MethodInfo* method);

    void request_unload();
    void release_render_target();
    void create_render_target(IDXGISwapChain* swap_chain);
    void release_device();
    bool setup_menu(IDXGISwapChain* swap_chain);
    HRESULT present(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags);
    HRESULT resize_buffers(IDXGISwapChain* swap_chain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags);
    void shot(ShotFn original, il2cpp::Object* player, il2cpp::Object* shot, const MethodInfo* method);
    bool raycast(Vector3 origin, Vector3 direction, float max_distance, int layer_mask, int query_trigger_interaction, const MethodInfo* method);
    bool clear_shot(Vector3 origin, Vector3 direction, float max_distance, int layer_mask, int query_trigger_interaction, const MethodInfo* method);
    il2cpp::Array* raycast_all(Vector3 origin, Vector3 direction, float max_distance, int layer_mask, int query_trigger_interaction, const MethodInfo* method);
    int raycast_non_alloc(Vector3 origin, Vector3 direction, il2cpp::Array* results, float max_distance, int layer_mask, int query_trigger_interaction, const MethodInfo* method);
    void player_update(il2cpp::Object* player, const MethodInfo* method);
    bool get_swapchain_vtable(void**& table);
    bool install_hook(const char* name, void* target, void* detour, void** original);

    PresentFn present_ = nullptr;
    ResizeBuffersFn resize_buffers_ = nullptr;
    ShotFn shot_a_ = nullptr;
    ShotFn shot_b_ = nullptr;
    RaycastFn raycast_ = nullptr;
    RaycastAllFn raycast_all_ = nullptr;
    RaycastNonAllocFn raycast_non_alloc_ = nullptr;
    PlayerUpdateFn player_update_ = nullptr;
    HMODULE module_ = nullptr;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    ID3D11RenderTargetView* render_target_ = nullptr;
    HWND window_ = nullptr;
    WNDPROC wnd_proc_ = nullptr;
    bool unload_ = false;
    bool menu_ready_ = false;
    bool menu_failure_logged_ = false;
    bool minhook_ready_ = false;
    bool shutting_down_ = false;
};

Hooks& hooks();
}
