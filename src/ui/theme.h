#pragma once

#include "settings.h"

#include <imgui.h>

#include <span>

namespace hackmatch::ui {
struct ThemePalette {
    ThemeId id;
    const char* name;
    ImVec4 background;
    ImVec4 sidebar;
    ImVec4 card;
    ImVec4 card_hover;
    ImVec4 surface_elevated;
    ImVec4 accent;
    ImVec4 accent_surface;
    ImVec4 accent_hover;
    ImVec4 accent_active;
    ImVec4 text;
    ImVec4 muted;
    ImVec4 border;
    ImVec4 border_strong;
    ImVec4 success;
    ImVec4 warning;
    Rgba team;
    Rgba enemy;
    Rgba target;
    Rgba aim_fov;
    Rgba overlay_shadow;
};

std::span<const ThemePalette> themes();
const ThemePalette& palette(ThemeId id);
const ThemePalette& active_palette();
void apply_theme(ThemeId id);
ImVec4 to_imgui(const Rgba& color, float alpha_multiplier = 1.0f);
Rgba effective_color(const ColorSetting& setting, const Rgba& inherited);
}
