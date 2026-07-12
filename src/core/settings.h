#pragma once

#include <optional>
#include <string_view>

namespace hackmatch
{
enum class ThemeId
{
    LogoRed,
    TacticalBlue,
    Emerald,
    Monochrome,
    Dracula,
    CatppuccinMocha,
    TokyoNight,
    Nord,
    GruvboxDark,
    OneDark,
    SolarizedDark,
    Monokai,
    GitHubDark,
    RosePine,
    Kanagawa,
    Everforest,
    Synthwave84,
    NightOwl,
    Poimandres,
    Vesper,
    Andromeeda,
    AuroraX,
    AyuDark,
    CatppuccinFrappe,
    CatppuccinMacchiato,
    DarkPlus,
    Houston,
    KanagawaDragon,
    KanagawaLotus,
    Laserwave,
    MaterialOcean,
    MaterialPalenight,
    Red,
    RosePineMoon,
    SlackDark,
    VitesseDark,
    Custom,
    Count,
};

std::string_view theme_key(ThemeId theme);
std::optional<ThemeId> theme_from_key(std::string_view key);

enum class BoxStyle
{
    Full,
    Corner,
};

enum class SnaplineOrigin
{
    Bottom,
    Center,
    Crosshair,
};

enum class AimTargetPoint
{
    Automatic,
    Head,
    Torso,
    Base,
};

struct Rgba
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    bool operator==(const Rgba&) const = default;
};

struct ColorSetting
{
    bool custom = false;
    Rgba value{};

    bool operator==(const ColorSetting&) const = default;
};

struct AimSettings
{
    bool enabled = false;
    bool always_on = false;
    bool ignore_fov = false;
    bool ignore_spawn_protected_targets = false;
    bool target_teammates = false;
    bool wallbang = false;
    float fov = 25.0f;
    int hotkey = 0x02;
    AimTargetPoint target_point = AimTargetPoint::Automatic;

    bool operator==(const AimSettings&) const = default;
};

struct EspSettings
{
    bool enabled = false;
    bool boxes = true;
    BoxStyle box_style = BoxStyle::Full;
    bool filled_boxes = false;
    float fill_opacity = 0.16f;
    float box_thickness = 1.0f;
    bool names = true;
    bool show_distance = false;
    float text_scale = 1.0f;
    bool snaplines = false;
    SnaplineOrigin snapline_origin = SnaplineOrigin::Bottom;
    bool offscreen_arrows = false;
    bool target_marker = false;
    float maximum_distance = 0.0f;
    bool show_team = true;
    bool show_enemies = true;
    bool show_aim_fov = true;
    ColorSetting team_color{};
    ColorSetting enemy_color{};
    ColorSetting target_color{};
    ColorSetting aim_fov_color{};
    float aim_fov_thickness = 1.5f;
    float aim_fov_opacity = 0.47f;

    bool operator==(const EspSettings&) const = default;
};

struct WeaponSettings
{
    bool no_spread = false;
    bool infinite_ammo = false;
    bool instant_reload = false;
    float reload_time = 0.0f;
    bool no_camera_shake = false;
    bool rapid_fire = false;

    bool operator==(const WeaponSettings&) const = default;
};

struct MovementSettings
{
    bool auto_sprint = false;
    bool no_gravity = false;
    bool custom_gravity = false;
    bool high_speed = false;
    float gravity = -9.81f;
    float speed = 18.0f;

    bool operator==(const MovementSettings&) const = default;
};

struct PlayerSettings
{
    bool custom_fov = false;
    bool disable_spawn_protection = false;
    bool movement_diagnostics = false;
    float camera_fov = 90.0f;

    bool operator==(const PlayerSettings&) const = default;
};

struct CustomThemeSettings
{
    Rgba background{0.075f, 0.086f, 0.118f, 1.0f};
    Rgba surface{0.137f, 0.153f, 0.204f, 1.0f};
    Rgba accent{0.741f, 0.576f, 0.976f, 1.0f};
    Rgba text{0.878f, 0.878f, 0.949f, 1.0f};
    Rgba muted{0.502f, 0.518f, 0.612f, 1.0f};

    bool operator==(const CustomThemeSettings&) const = default;
};

struct InterfaceSettings
{
    bool streamproof = false;
    float menu_opacity = 94.0f;
    int menu_hotkey = 0x2D;
    int unload_hotkey = 0x23;
    int aim_toggle_hotkey = 0;
    int esp_toggle_hotkey = 0;

    bool operator==(const InterfaceSettings&) const = default;
};

struct AppSettings
{
    ThemeId theme = ThemeId::LogoRed;
    CustomThemeSettings custom_theme;
    InterfaceSettings controls;
    AimSettings aim;
    EspSettings esp;
    WeaponSettings weapons;
    MovementSettings movement;
    PlayerSettings player;

    bool operator==(const AppSettings&) const = default;
};

AppSettings& settings();
bool is_bindable_hotkey(int key);
} // namespace hackmatch
