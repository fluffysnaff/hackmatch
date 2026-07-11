#pragma once

#include "settings.h"

#include <imgui.h>

namespace hackmatch::ui {
enum class NavigationIcon { Aim, Visuals, Weapons, Player, Config };

void configure_widgets(ImFont* strong_font, bool motion_enabled);
bool navigation_item(const char* label, NavigationIcon icon, float selection);
bool toggle(const char* label, const char* description, bool& value, const ImVec4* background = nullptr,
    const ImVec4* description_color = nullptr);
void card_begin(const char* id, const char* title, const char* description);
void card_end();
void setting_slider(const char* label, float& value, float minimum, float maximum, const char* format);
void setting_slider(const char* label, int& value, int minimum, int maximum, const char* format);
void color_setting(const char* label, ColorSetting& value, const Rgba& inherited);
}
