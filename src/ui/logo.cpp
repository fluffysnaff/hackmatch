#include "logo.h"
#include "resource.h"

#include <d2d1_3.h>
#include <d3d11.h>
#include <shlwapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace hackmatch::ui {
namespace {
using Microsoft::WRL::ComPtr;

extern "C" IMAGE_DOS_HEADER __ImageBase;
ComPtr<ID3D11ShaderResourceView> logo_texture;
}

bool init_logo(ID3D11Device* device)
{
    if (logo_texture || !device) return logo_texture != nullptr;

    const HMODULE module = reinterpret_cast<HMODULE>(&__ImageBase);
    const HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(IDR_LOGO_SVG), MAKEINTRESOURCEW(10));
    const HGLOBAL loaded = resource ? LoadResource(module, resource) : nullptr;
    const void* data = loaded ? LockResource(loaded) : nullptr;
    const DWORD size = resource ? SizeofResource(module, resource) : 0;
    ComPtr<IStream> stream(data && size ? SHCreateMemStream(static_cast<const BYTE*>(data), size) : nullptr);
    if (!stream) return false;

    // The game's device may not have D3D11_CREATE_DEVICE_BGRA_SUPPORT, which Direct2D requires.
    ComPtr<ID3D11Device> raster_device;
    ComPtr<ID3D11DeviceContext> raster_d3d_context;
    D3D_FEATURE_LEVEL feature_level{};
    HRESULT raster_result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, &raster_device, &feature_level, &raster_d3d_context);
    if (FAILED(raster_result)) {
        raster_result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr, 0, D3D11_SDK_VERSION, &raster_device, &feature_level, &raster_d3d_context);
    }

    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<ID2D1Factory1> factory;
    ComPtr<ID2D1Device> d2d_device;
    ComPtr<ID2D1DeviceContext> base_context;
    ComPtr<ID2D1DeviceContext5> context;
    if (FAILED(raster_result) || FAILED(raster_device.As(&dxgi_device)) ||
        FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreateDevice(dxgi_device.Get(), &d2d_device)) ||
        FAILED(d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &base_context)) ||
        FAILED(base_context.As(&context))) return false;

    constexpr UINT texture_size = 192;
    D3D11_TEXTURE2D_DESC texture_description{};
    texture_description.Width = texture_size;
    texture_description.Height = texture_size;
    texture_description.MipLevels = 1;
    texture_description.ArraySize = 1;
    texture_description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texture_description.SampleDesc.Count = 1;
    texture_description.Usage = D3D11_USAGE_DEFAULT;
    texture_description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    ComPtr<ID3D11Texture2D> texture;
    ComPtr<IDXGISurface> surface;
    ComPtr<ID2D1Bitmap1> target;
    const D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(texture_description.Format, D2D1_ALPHA_MODE_PREMULTIPLIED));
    if (FAILED(raster_device->CreateTexture2D(&texture_description, nullptr, &texture)) ||
        FAILED(texture.As(&surface)) || FAILED(context->CreateBitmapFromDxgiSurface(surface.Get(), &properties, &target))) return false;

    ComPtr<ID2D1SvgDocument> document;
    if (FAILED(context->CreateSvgDocument(stream.Get(), D2D1::SizeF(static_cast<float>(texture_size), static_cast<float>(texture_size)), &document))) return false;
    context->SetTarget(target.Get());
    context->BeginDraw();
    context->Clear(D2D1_COLOR_F{0.0f, 0.0f, 0.0f, 0.0f});
    context->DrawSvgDocument(document.Get());
    context->SetTarget(nullptr);
    if (FAILED(context->EndDraw())) return false;

    D3D11_TEXTURE2D_DESC staging_description = texture_description;
    staging_description.Usage = D3D11_USAGE_STAGING;
    staging_description.BindFlags = 0;
    staging_description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(raster_device->CreateTexture2D(&staging_description, nullptr, &staging))) return false;
    raster_d3d_context->CopyResource(staging.Get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(raster_d3d_context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return false;
    constexpr UINT row_size = texture_size * 4;
    std::vector<BYTE> pixels(row_size * texture_size);
    for (UINT row = 0; row < texture_size; ++row) {
        std::memcpy(pixels.data() + row * row_size, static_cast<const BYTE*>(mapped.pData) + row * mapped.RowPitch, row_size);
    }
    for (size_t pixel = 0; pixel < pixels.size(); pixel += 4) {
        const BYTE intensity = std::max({pixels[pixel], pixels[pixel + 1], pixels[pixel + 2]});
        pixels[pixel] = intensity;
        pixels[pixel + 1] = intensity;
        pixels[pixel + 2] = intensity;
    }
    raster_d3d_context->Unmap(staging.Get(), 0);

    texture_description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial_data{pixels.data(), row_size, 0};
    ComPtr<ID3D11Texture2D> uploaded;
    return SUCCEEDED(device->CreateTexture2D(&texture_description, &initial_data, &uploaded)) &&
        SUCCEEDED(device->CreateShaderResourceView(uploaded.Get(), nullptr, &logo_texture));
}

void shutdown_logo()
{
    logo_texture.Reset();
}

void draw_logo(ImDrawList* draw, ImVec2 position, float size, ImU32 tint)
{
    if (draw && logo_texture) {
        draw->AddImage(reinterpret_cast<ImTextureID>(logo_texture.Get()), position, {position.x + size, position.y + size}, {}, {1, 1}, tint);
    }
}
}
