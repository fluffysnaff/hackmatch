#include "settings.h"

#include <windows.h>

namespace hackmatch {
namespace {
constexpr std::string_view theme_keys[] = {
    "logo_red", "tactical_blue", "emerald", "monochrome", "dracula", "catppuccin_mocha", "tokyo_night", "nord",
    "gruvbox_dark", "one_dark", "solarized_dark", "monokai", "github_dark", "rose_pine", "kanagawa", "everforest",
    "synthwave_84", "night_owl", "poimandres", "vesper", "andromeeda", "aurora_x", "ayu_dark", "catppuccin_frappe",
    "catppuccin_macchiato", "dark_plus", "houston", "kanagawa_dragon", "kanagawa_lotus", "laserwave", "material_ocean",
    "material_palenight", "red", "rose_pine_moon", "slack_dark", "vitesse_dark", "custom",
};
static_assert(std::size(theme_keys) == static_cast<size_t>(ThemeId::Count));
}

std::string_view theme_key(ThemeId theme)
{
    const size_t index = static_cast<size_t>(theme);
    return index < std::size(theme_keys) ? theme_keys[index] : theme_keys[0];
}

std::optional<ThemeId> theme_from_key(std::string_view key)
{
    for (size_t index = 0; index < std::size(theme_keys); ++index) {
        if (theme_keys[index] == key) return static_cast<ThemeId>(index);
    }
    return std::nullopt;
}

AppSettings& settings()
{
    static AppSettings instance;
    return instance;
}

bool is_bindable_hotkey(int key)
{
    return key > 0 && key <= 255 && key != VK_ESCAPE && key != VK_LWIN && key != VK_RWIN && key != VK_APPS &&
        key != VK_SLEEP;
}
}
