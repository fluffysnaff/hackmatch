#include "imgui_menu.h"

#include <windows.h>
#include <shellapi.h>

#include <d3d11.h>
#include <imgui_impl_win32.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

namespace {
using Microsoft::WRL::ComPtr;

LRESULT WINAPI window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam)) {
        return 1;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

bool save_back_buffer(
    ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swap_chain, const wchar_t* path, bool preserve_alpha)
{
    ComPtr<ID3D11Texture2D> back_buffer;
    if (FAILED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
        return false;
    }

    D3D11_TEXTURE2D_DESC description{};
    back_buffer->GetDesc(&description);
    description.BindFlags = 0;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;

    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&description, nullptr, &staging))) {
        return false;
    }
    context->CopyResource(staging.Get(), back_buffer.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        return false;
    }

    const UINT output_stride = description.Width * 4;
    std::vector<BYTE> bgra_pixels(static_cast<size_t>(output_stride) * description.Height);
    const auto* source = static_cast<const BYTE*>(mapped.pData);
    for (UINT y = 0; y < description.Height; ++y) {
        const BYTE* source_row = source + static_cast<size_t>(mapped.RowPitch) * y;
        BYTE* destination_row = bgra_pixels.data() + static_cast<size_t>(output_stride) * y;
        for (UINT x = 0; x < description.Width; ++x) {
            destination_row[x * 4] = source_row[x * 4 + 2];
            destination_row[x * 4 + 1] = source_row[x * 4 + 1];
            destination_row[x * 4 + 2] = source_row[x * 4];
            destination_row[x * 4 + 3] = preserve_alpha ? source_row[x * 4 + 3] : 255;
        }
    }

    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    IPropertyBag2* properties = nullptr;
    const bool initialized = SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) &&
        SUCCEEDED(factory->CreateStream(&stream)) &&
        SUCCEEDED(stream->InitializeFromFilename(path, GENERIC_WRITE)) &&
        SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
        SUCCEEDED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)) &&
        SUCCEEDED(encoder->CreateNewFrame(&frame, &properties)) &&
        SUCCEEDED(frame->Initialize(properties));
    if (properties) {
        properties->Release();
    }

    bool saved = false;
    if (initialized && SUCCEEDED(frame->SetSize(description.Width, description.Height))) {
        WICPixelFormatGUID pixel_format = GUID_WICPixelFormat32bppBGRA;
        if (SUCCEEDED(frame->SetPixelFormat(&pixel_format)) && pixel_format == GUID_WICPixelFormat32bppBGRA &&
            SUCCEEDED(frame->WritePixels(description.Height, output_stride, output_stride * description.Height, bgra_pixels.data())) &&
            SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit())) {
            saved = true;
        }
    }
    context->Unmap(staging.Get(), 0);
    return saved;
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command_line, int)
{
    int argument_count = 0;
    wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
    const std::filesystem::path output = argument_count > 1 ? arguments[1] : L"hackmatch_ui_preview.png";
    const int theme = argument_count > 2 ? static_cast<int>(std::wcstol(arguments[2], nullptr, 10)) : 0;
    const int page = argument_count > 3 ? static_cast<int>(std::wcstol(arguments[3], nullptr, 10)) : 0;
    const int preview_width =
        argument_count > 4 ? std::clamp(static_cast<int>(std::wcstol(arguments[4], nullptr, 10)), 792, 2560) : 952;
    const int preview_height =
        argument_count > 5 ? std::clamp(static_cast<int>(std::wcstol(arguments[5], nullptr, 10)), 552, 1440) : 692;
    const float navigation_progress =
        argument_count > 6 ? std::clamp(static_cast<float>(std::wcstod(arguments[6], nullptr)), 0.0f, 1.0f) : 1.0f;
    const float content_progress =
        argument_count > 7 ? std::clamp(static_cast<float>(std::wcstod(arguments[7], nullptr)), 0.0f, 1.0f) : 1.0f;
    const bool reduced_motion = argument_count > 8 && std::wcstol(arguments[8], nullptr, 10) != 0;
    const int background = argument_count > 9 ? std::clamp(static_cast<int>(std::wcstol(arguments[9], nullptr, 10)), 0, 2) : 0;
    if (arguments) {
        LocalFree(arguments);
    }

    constexpr wchar_t window_class[] = L"HackmatchUiPreview";
    const WNDCLASSEXW definition{sizeof(WNDCLASSEXW), CS_CLASSDC, window_proc, 0, 0, instance, nullptr, nullptr, nullptr, nullptr,
        window_class, nullptr};
    if (!RegisterClassExW(&definition)) {
        return 1;
    }

    const HWND window = CreateWindowExW(
        0, window_class, L"Hackmatch UI Preview", WS_POPUP, 0, 0, preview_width, preview_height, nullptr, nullptr, instance, nullptr);
    if (!window) {
        UnregisterClassW(window_class, instance);
        return 2;
    }

    DXGI_SWAP_CHAIN_DESC swap_description{};
    swap_description.BufferCount = 2;
    swap_description.BufferDesc.Width = static_cast<UINT>(preview_width);
    swap_description.BufferDesc.Height = static_cast<UINT>(preview_height);
    swap_description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_description.OutputWindow = window;
    swap_description.SampleDesc.Count = 1;
    swap_description.Windowed = TRUE;
    swap_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swap_chain;
    D3D_FEATURE_LEVEL feature_level{};
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
            &swap_description, &swap_chain, &device, &feature_level, &context))) {
        DestroyWindow(window);
        UnregisterClassW(window_class, instance);
        return 3;
    }

    ComPtr<ID3D11Texture2D> back_buffer;
    ComPtr<ID3D11RenderTargetView> render_target;
    if (FAILED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))) ||
        FAILED(device->CreateRenderTargetView(back_buffer.Get(), nullptr, &render_target)) ||
        !hackmatch::imgui_menu().init(window, device.Get(), context.Get())) {
        DestroyWindow(window);
        UnregisterClassW(window_class, instance);
        return 4;
    }

    hackmatch::imgui_menu().toggle();
    hackmatch::imgui_menu().set_preview_state(theme, page, navigation_progress, content_progress, reduced_motion);
    for (int frame_index = 0; frame_index < 4; ++frame_index) {
        constexpr float backgrounds[3][4] = {
            {0.018f, 0.024f, 0.035f, 1.0f},
            {0.82f, 0.76f, 0.60f, 1.0f},
            {0.0f, 0.0f, 0.0f, 0.0f},
        };
        ID3D11RenderTargetView* target = render_target.Get();
        context->OMSetRenderTargets(1, &target, nullptr);
        context->ClearRenderTargetView(render_target.Get(), backgrounds[background]);
        hackmatch::imgui_menu().render();
        if (frame_index < 3) {
            swap_chain->Present(0, 0);
        }
    }

    static_cast<void>(command_line);
    const bool saved = save_back_buffer(device.Get(), context.Get(), swap_chain.Get(), output.c_str(), background == 2);

    hackmatch::imgui_menu().shutdown();
    render_target.Reset();
    back_buffer.Reset();
    swap_chain.Reset();
    context.Reset();
    device.Reset();
    DestroyWindow(window);
    UnregisterClassW(window_class, instance);
    return saved ? 0 : 5;
}
