#include "hooks.h"

#include "logger.h"
#include "menu.h"

namespace hackmatch {
void Hooks::release_render_target()
{
    if (render_target_) {
        render_target_->Release();
        render_target_ = nullptr;
    }
}

void Hooks::create_render_target(IDXGISwapChain* swap_chain)
{
    ID3D11Texture2D* back_buffer = nullptr;
    if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
        device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_);
        back_buffer->Release();
    }
}

void Hooks::release_device()
{
    if (context_) {
        context_->Release();
        context_ = nullptr;
    }
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
}

bool Hooks::setup_menu(IDXGISwapChain* swap_chain)
{
    if (menu_ready_) {
        return true;
    }

    DXGI_SWAP_CHAIN_DESC description{};
    if (FAILED(swap_chain->GetDesc(&description)) || FAILED(swap_chain->GetDevice(IID_PPV_ARGS(&device_)))) {
        if (!menu_failure_logged_) {
            logger::error("DirectX device acquisition failed; menu initialization will retry");
            menu_failure_logged_ = true;
        }
        return false;
    }

    device_->GetImmediateContext(&context_);
    window_ = description.OutputWindow;
    if (!menu().init(window_, device_, context_)) {
        if (!menu_failure_logged_) {
            logger::error("Menu initialization failed; initialization will retry");
            menu_failure_logged_ = true;
        }
        release_device();
        return false;
    }

    wnd_proc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wnd_proc)));
    create_render_target(swap_chain);
    menu_ready_ = true;
    logger::success("DirectX renderer and menu are ready");
    return true;
}

HRESULT __stdcall Hooks::present_hook(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags)
{
    return hooks().present(swap_chain, sync_interval, flags);
}

HRESULT Hooks::present(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags)
{
    if (!unload_ && setup_menu(swap_chain)) {
        if (!render_target_) {
            create_render_target(swap_chain);
        }
        context_->OMSetRenderTargets(1, &render_target_, nullptr);
        menu().render();
    }
    return present_(swap_chain, sync_interval, flags);
}

HRESULT __stdcall Hooks::resize_buffers_hook(IDXGISwapChain* swap_chain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    return hooks().resize_buffers(swap_chain, buffer_count, width, height, format, flags);
}

HRESULT Hooks::resize_buffers(IDXGISwapChain* swap_chain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    release_render_target();
    return resize_buffers_(swap_chain, buffer_count, width, height, format, flags);
}

bool Hooks::get_swapchain_vtable(void**& table)
{
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_CLASSDC;
    window_class.lpfnWndProc = DefWindowProcW;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.lpszClassName = L"hackmatch_dummy";
    RegisterClassExW(&window_class);

    HWND window = CreateWindowW(window_class.lpszClassName, L"Hackmatch", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, window_class.hInstance, nullptr);
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferCount = 1;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.OutputWindow = window;
    description.SampleDesc.Count = 1;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    D3D_FEATURE_LEVEL feature_level{};
    const D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_levels, 2, D3D11_SDK_VERSION, &description, &swap_chain, &device, &feature_level, &context);
    if (FAILED(result)) {
        result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, feature_levels, 2, D3D11_SDK_VERSION, &description, &swap_chain, &device, &feature_level, &context);
    }
    if (SUCCEEDED(result)) {
        table = *reinterpret_cast<void***>(swap_chain);
    }

    if (swap_chain) {
        swap_chain->Release();
    }
    if (context) {
        context->Release();
    }
    if (device) {
        device->Release();
    }
    DestroyWindow(window);
    UnregisterClassW(window_class.lpszClassName, window_class.hInstance);
    return SUCCEEDED(result);
}
}
