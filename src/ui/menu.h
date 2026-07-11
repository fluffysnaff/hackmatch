#pragma once

#include <windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace hackmatch {
class Menu {
public:
    bool init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context);
    void render();
    void shutdown();
    void toggle();
    bool visible() const;
    bool capturing_hotkey() const;
    void add_mouse_delta(float x, float y);

private:
    bool ready_ = false;
};

Menu& menu();
}
