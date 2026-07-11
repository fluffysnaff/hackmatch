#include "theme.h"

#include <algorithm>
#include <array>

namespace hackmatch::ui {
namespace {
ImVec4 rgba(unsigned int hex, float alpha = 1.0f)
{
    return {static_cast<float>((hex >> 16) & 0xff) / 255.0f, static_cast<float>((hex >> 8) & 0xff) / 255.0f,
        static_cast<float>(hex & 0xff) / 255.0f, alpha};
}

ImVec4 mix(ImVec4 first, ImVec4 second, float amount)
{
    return {first.x + (second.x - first.x) * amount, first.y + (second.y - first.y) * amount,
        first.z + (second.z - first.z) * amount, first.w + (second.w - first.w) * amount};
}

Rgba color(ImVec4 value) { return {value.x, value.y, value.z, value.w}; }

ThemePalette make_theme(ThemeId id, const char* name, unsigned int background_hex, unsigned int surface_hex,
    unsigned int accent_hex, unsigned int text_hex, unsigned int muted_hex)
{
    const ImVec4 background = rgba(background_hex, 0.985f);
    const ImVec4 surface = rgba(surface_hex);
    const ImVec4 accent = rgba(accent_hex);
    const ImVec4 text = rgba(text_hex);
    const ImVec4 muted = rgba(muted_hex);
    return {id, name, background, mix(background, surface, 0.45f), surface, mix(surface, text, 0.08f),
        mix(surface, text, 0.15f), accent, mix(background, accent, 0.58f), mix(background, accent, 0.72f),
        mix(background, accent, 0.48f), text, muted, mix(background, text, 0.10f), mix(background, text, 0.22f),
        rgba(0x4fd6a0), rgba(0xe5c07b), color(rgba(0x61afef)), color(rgba(0xff5c57)), color(rgba(0xf1c40f)),
        color(text), {0.0f, 0.0f, 0.0f, 1.0f}};
}

const std::array built_in_themes{
    make_theme(ThemeId::LogoRed, "Logo Red", 0x0a0c0f, 0x14161a, 0xff5a5f, 0xf6f7f9, 0xa9b0bb),
    make_theme(ThemeId::TacticalBlue, "Tactical Blue", 0x09131f, 0x102338, 0x5db0ff, 0xf2f7fd, 0xa0b2c8),
    make_theme(ThemeId::Emerald, "Emerald", 0x08100e, 0x0f1a17, 0x4dd9a1, 0xf1faf7, 0x9dbbb1),
    make_theme(ThemeId::Monochrome, "Monochrome", 0x0b0b0c, 0x161618, 0xe8e8ec, 0xf7f7f9, 0xb0b0b5),
    make_theme(ThemeId::Dracula, "Dracula", 0x282a36, 0x44475a, 0xbd93f9, 0xf8f8f2, 0x6272a4),
    make_theme(ThemeId::CatppuccinMocha, "Catppuccin Mocha", 0x1e1e2e, 0x313244, 0xcba6f7, 0xcdd6f4, 0x6c7086),
    make_theme(ThemeId::TokyoNight, "Tokyo Night", 0x1a1b26, 0x24283b, 0x7aa2f7, 0xc0caf5, 0x565f89),
    make_theme(ThemeId::Nord, "Nord", 0x2e3440, 0x3b4252, 0x88c0d0, 0xeceff4, 0x7b88a1),
    make_theme(ThemeId::GruvboxDark, "Gruvbox Dark", 0x282828, 0x3c3836, 0xfe8019, 0xebdbb2, 0xa89984),
    make_theme(ThemeId::OneDark, "One Dark", 0x282c34, 0x3e4451, 0x61afef, 0xabb2bf, 0x5c6370),
    make_theme(ThemeId::SolarizedDark, "Solarized Dark", 0x002b36, 0x073642, 0x2aa198, 0x839496, 0x586e75),
    make_theme(ThemeId::Monokai, "Monokai", 0x272822, 0x3e3d32, 0xf92672, 0xf8f8f2, 0x75715e),
    make_theme(ThemeId::GitHubDark, "GitHub Dark", 0x0d1117, 0x161b22, 0x58a6ff, 0xc9d1d9, 0x8b949e),
    make_theme(ThemeId::RosePine, "Rose Pine", 0x191724, 0x26233a, 0xc4a7e7, 0xe0def4, 0x6e6a86),
    make_theme(ThemeId::Kanagawa, "Kanagawa", 0x1f1f28, 0x2a2a37, 0x7e9cd8, 0xdcd7ba, 0x727169),
    make_theme(ThemeId::Everforest, "Everforest", 0x2d353b, 0x343f44, 0xa7c080, 0xd3c6aa, 0x859289),
    make_theme(ThemeId::Synthwave84, "Synthwave '84", 0x262335, 0x34294f, 0xff7edb, 0xffffff, 0x848bbd),
    make_theme(ThemeId::NightOwl, "Night Owl", 0x011627, 0x0b2942, 0x7fdbca, 0xd6deeb, 0x5f7e97),
    make_theme(ThemeId::Poimandres, "Poimandres", 0x1b1e28, 0x303340, 0x5de4c7, 0xe4f0fb, 0x767c9d),
    make_theme(ThemeId::Vesper, "Vesper", 0x101010, 0x1c1c1c, 0xffc799, 0xffffff, 0xa0a0a0),
    make_theme(ThemeId::Andromeeda, "Andromeeda", 0x23262e, 0x2f333d, 0xff00aa, 0xd5ced9, 0x7b8496),
    make_theme(ThemeId::AuroraX, "Aurora X", 0x07090f, 0x0d1017, 0x00e8c6, 0xc5c8c6, 0x5f6577),
    make_theme(ThemeId::AyuDark, "Ayu Dark", 0x0b0e14, 0x131721, 0xe6b450, 0xbfbdb6, 0x565b66),
    make_theme(ThemeId::CatppuccinFrappe, "Catppuccin Frappe", 0x303446, 0x414559, 0xca9ee6, 0xc6d0f5, 0x838ba7),
    make_theme(ThemeId::CatppuccinMacchiato, "Catppuccin Macchiato", 0x24273a, 0x363a4f, 0xc6a0f6, 0xcad3f5, 0x8087a2),
    make_theme(ThemeId::DarkPlus, "Dark Plus", 0x1e1e1e, 0x252526, 0x569cd6, 0xd4d4d4, 0x808080),
    make_theme(ThemeId::Houston, "Houston", 0x17191e, 0x20232a, 0x54d6ff, 0xe7e9ee, 0x77808f),
    make_theme(ThemeId::KanagawaDragon, "Kanagawa Dragon", 0x181616, 0x282727, 0x8ba4b0, 0xc5c9c5, 0x737c73),
    make_theme(ThemeId::KanagawaLotus, "Kanagawa Lotus", 0xf2ecbc, 0xe7dba0, 0x4d699b, 0x545464, 0x8a8980),
    make_theme(ThemeId::Laserwave, "Laserwave", 0x27212e, 0x3d2b42, 0xeb64b9, 0xf8f8f2, 0x91889b),
    make_theme(ThemeId::MaterialOcean, "Material Ocean", 0x0f111a, 0x1a1c25, 0x89ddff, 0xa6accd, 0x464b5d),
    make_theme(ThemeId::MaterialPalenight, "Material Palenight", 0x292d3e, 0x34324a, 0xc792ea, 0xa6accd, 0x676e95),
    make_theme(ThemeId::Red, "Red", 0x390000, 0x4d0000, 0xff5555, 0xf8d7d7, 0xb97b7b),
    make_theme(ThemeId::RosePineMoon, "Rose Pine Moon", 0x232136, 0x2a273f, 0xc4a7e7, 0xe0def4, 0x6e6a86),
    make_theme(ThemeId::SlackDark, "Slack Dark", 0x222222, 0x2c2c2c, 0x36c5f0, 0xd1d2d3, 0xababad),
    make_theme(ThemeId::VitesseDark, "Vitesse Dark", 0x121212, 0x1a1a1a, 0x4d9375, 0xdbd7ca, 0x758575),
};
static_assert(built_in_themes.size() == static_cast<size_t>(ThemeId::Custom));

ThemePalette custom_palette()
{
    const CustomThemeSettings& custom = settings().custom_theme;
    ThemePalette result = make_theme(ThemeId::Custom, "Custom", 0, 0, 0, 0, 0);
    result.background = to_imgui(custom.background);
    result.sidebar = mix(result.background, to_imgui(custom.surface), 0.45f);
    result.card = to_imgui(custom.surface);
    result.card_hover = mix(result.card, to_imgui(custom.text), 0.08f);
    result.surface_elevated = mix(result.card, to_imgui(custom.text), 0.15f);
    result.accent = to_imgui(custom.accent);
    result.accent_surface = mix(result.background, result.accent, 0.58f);
    result.accent_hover = mix(result.background, result.accent, 0.72f);
    result.accent_active = mix(result.background, result.accent, 0.48f);
    result.text = to_imgui(custom.text);
    result.muted = to_imgui(custom.muted);
    result.border = mix(result.background, result.text, 0.10f);
    result.border_strong = mix(result.background, result.text, 0.22f);
    result.team = color(result.accent);
    result.aim_fov = color(result.text);
    return result;
}
}

std::span<const ThemePalette> themes() { return built_in_themes; }

const ThemePalette& palette(ThemeId id)
{
    static ThemePalette custom;
    if (id == ThemeId::Custom) {
        custom = custom_palette();
        return custom;
    }
    const size_t index = static_cast<size_t>(id);
    return index < built_in_themes.size() ? built_in_themes[index] : built_in_themes[0];
}

const ThemePalette& active_palette() { return palette(settings().theme); }

ImVec4 to_imgui(const Rgba& value, float alpha_multiplier)
{
    return {value.r, value.g, value.b, std::clamp(value.a * alpha_multiplier, 0.0f, 1.0f)};
}

Rgba effective_color(const ColorSetting& setting, const Rgba& inherited)
{
    return setting.custom ? setting.value : inherited;
}

void apply_theme(ThemeId id)
{
    const ThemePalette& color_value = palette(id);
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = {0.0f, 0.0f};
    style.FramePadding = {12.0f, 8.0f};
    style.ItemSpacing = {12.0f, 8.0f};
    style.ItemInnerSpacing = {8.0f, 8.0f};
    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 12.0f;
    style.DisabledAlpha = 0.45f;
    auto& colors = style.Colors;
    colors[ImGuiCol_Text] = color_value.text;
    colors[ImGuiCol_TextDisabled] = color_value.muted;
    colors[ImGuiCol_WindowBg] = {0, 0, 0, 0};
    colors[ImGuiCol_ChildBg] = {0, 0, 0, 0};
    colors[ImGuiCol_PopupBg] = color_value.sidebar;
    colors[ImGuiCol_Border] = color_value.border_strong;
    colors[ImGuiCol_FrameBg] = color_value.surface_elevated;
    colors[ImGuiCol_FrameBgHovered] = color_value.card_hover;
    colors[ImGuiCol_FrameBgActive] = color_value.surface_elevated;
    colors[ImGuiCol_TitleBg] = color_value.background;
    colors[ImGuiCol_TitleBgActive] = color_value.background;
    colors[ImGuiCol_ScrollbarBg] = color_value.background;
    colors[ImGuiCol_ScrollbarGrab] = color_value.border_strong;
    colors[ImGuiCol_ScrollbarGrabHovered] = color_value.muted;
    colors[ImGuiCol_ScrollbarGrabActive] = color_value.accent;
    colors[ImGuiCol_CheckMark] = color_value.text;
    colors[ImGuiCol_SliderGrab] = color_value.accent;
    colors[ImGuiCol_SliderGrabActive] = color_value.accent_hover;
    colors[ImGuiCol_Button] = color_value.accent_surface;
    colors[ImGuiCol_ButtonHovered] = color_value.accent_hover;
    colors[ImGuiCol_ButtonActive] = color_value.accent_active;
    colors[ImGuiCol_Header] = color_value.accent_surface;
    colors[ImGuiCol_HeaderHovered] = color_value.accent_hover;
    colors[ImGuiCol_HeaderActive] = color_value.accent_active;
    colors[ImGuiCol_Separator] = color_value.border;
    colors[ImGuiCol_ResizeGrip] = {0, 0, 0, 0};
    colors[ImGuiCol_ResizeGripHovered] = color_value.accent;
    colors[ImGuiCol_ResizeGripActive] = color_value.accent_hover;
    colors[ImGuiCol_NavHighlight] = color_value.accent;
}
}
