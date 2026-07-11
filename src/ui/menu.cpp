#include "menu.h"
#include "imgui_menu.h"

namespace hackmatch {
bool Menu::init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (ready_ || !window || !device || !context) {
        return ready_;
    }

    if (!imgui_menu().init(window, device, context)) {
        return false;
    }

    ready_ = true;
    return true;
}

void Menu::render()
{
    if (!ready_) {
        return;
    }

    imgui_menu().render();
}

void Menu::shutdown()
{
    if (!ready_) {
        return;
    }

    imgui_menu().shutdown();
    ready_ = false;
}

void Menu::toggle()
{
    imgui_menu().toggle();
}

bool Menu::visible() const
{
    return imgui_menu().visible();
}

bool Menu::capturing_hotkey() const
{
    return imgui_menu().capturing_hotkey();
}

void Menu::add_mouse_delta(float x, float y)
{
    imgui_menu().add_mouse_delta(x, y);
}

Menu& menu()
{
    static Menu instance;
    return instance;
}
}
