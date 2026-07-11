#include "config_manager.h"

#include "feature_limits.h"

#include <windows.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace hackmatch {
namespace {
using json = nlohmann::json;

std::filesystem::path default_directory()
{
    wchar_t buffer[32768]{};
    const DWORD size = GetEnvironmentVariableW(L"APPDATA", buffer, static_cast<DWORD>(std::size(buffer)));
    if (size == 0 || size >= std::size(buffer)) {
        return std::filesystem::temp_directory_path() / L"Hackmatch" / L"profiles";
    }
    return std::filesystem::path(buffer) / L"Hackmatch" / L"profiles";
}

ThemeId parse_theme(const json& value)
{
    const std::optional<ThemeId> theme = theme_from_key(value.get<std::string>());
    if (theme) return *theme;
    throw std::invalid_argument("invalid theme");
}

const char* box_style_name(BoxStyle value)
{
    return value == BoxStyle::Corner ? "corner" : "full";
}

BoxStyle parse_box_style(const json& value)
{
    const std::string name = value.get<std::string>();
    if (name == "full") return BoxStyle::Full;
    if (name == "corner") return BoxStyle::Corner;
    throw std::invalid_argument("invalid box style");
}

const char* snapline_name(SnaplineOrigin value)
{
    switch (value) {
    case SnaplineOrigin::Bottom: return "bottom";
    case SnaplineOrigin::Center: return "center";
    case SnaplineOrigin::Crosshair: return "crosshair";
    }
    return "bottom";
}

SnaplineOrigin parse_snapline(const json& value)
{
    const std::string name = value.get<std::string>();
    if (name == "bottom") return SnaplineOrigin::Bottom;
    if (name == "center") return SnaplineOrigin::Center;
    if (name == "crosshair") return SnaplineOrigin::Crosshair;
    throw std::invalid_argument("invalid snapline origin");
}

json rgba_json(const Rgba& value)
{
    return json::array({value.r, value.g, value.b, value.a});
}

Rgba parse_rgba(const json& value)
{
    if (!value.is_array() || value.size() != 4) {
        throw std::invalid_argument("color must contain four channels");
    }
    Rgba result{value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>(), value.at(3).get<float>()};
    const std::array channels{result.r, result.g, result.b, result.a};
    for (float channel : channels) {
        if (!std::isfinite(channel) || channel < 0.0f || channel > 1.0f) {
            throw std::invalid_argument("color channels must be between 0 and 1");
        }
    }
    return result;
}

json color_json(const ColorSetting& value)
{
    return {{"custom", value.custom}, {"rgba", rgba_json(value.value)}};
}

ColorSetting parse_color(const json& value)
{
    return {value.at("custom").get<bool>(), parse_rgba(value.at("rgba"))};
}

json custom_theme_json(const CustomThemeSettings& value)
{
    return {{"background", rgba_json(value.background)}, {"surface", rgba_json(value.surface)}, {"accent", rgba_json(value.accent)},
        {"text", rgba_json(value.text)}, {"muted", rgba_json(value.muted)}};
}

CustomThemeSettings parse_custom_theme(const json& value)
{
    return {parse_rgba(value.at("background")), parse_rgba(value.at("surface")), parse_rgba(value.at("accent")),
        parse_rgba(value.at("text")), parse_rgba(value.at("muted"))};
}

std::string base64url_encode(const std::string& input)
{
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string output;
    output.reserve((input.size() * 4 + 2) / 3);
    unsigned int accumulator = 0;
    int bits = -6;
    for (unsigned char byte : input) {
        accumulator = (accumulator << 8) | byte;
        bits += 8;
        while (bits >= 0) {
            output.push_back(alphabet[(accumulator >> bits) & 63U]);
            bits -= 6;
        }
    }
    if (bits > -6) {
        output.push_back(alphabet[((accumulator << 8) >> (bits + 8)) & 63U]);
    }
    return output;
}

bool base64url_decode(const std::string& input, std::string& output)
{
    std::array<int, 256> table{};
    table.fill(-1);
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    for (int index = 0; index < 64; ++index) {
        table[static_cast<unsigned char>(alphabet[index])] = index;
    }
    output.clear();
    output.reserve(input.size() * 3 / 4);
    unsigned int accumulator = 0;
    int bits = -8;
    for (unsigned char character : input) {
        const int value = table[character];
        if (value < 0) {
            return false;
        }
        accumulator = (accumulator << 6) | static_cast<unsigned int>(value);
        bits += 6;
        if (bits >= 0) {
            output.push_back(static_cast<char>((accumulator >> bits) & 0xFFU));
            bits -= 8;
            if (output.size() > ConfigManager::maximum_document_size) {
                return false;
            }
        }
    }
    return bits < 0;
}

bool set_clipboard_text(const std::string& text, std::string& error)
{
    if (!OpenClipboard(nullptr)) {
        error = "Clipboard is unavailable.";
        return false;
    }
    EmptyClipboard();
    const int wide_size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, static_cast<size_t>(wide_size + 1) * sizeof(wchar_t));
    wchar_t* destination = memory ? static_cast<wchar_t*>(GlobalLock(memory)) : nullptr;
    if (!destination) {
        if (memory) GlobalFree(memory);
        CloseClipboard();
        error = "Clipboard allocation failed.";
        return false;
    }
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), destination, wide_size);
    destination[wide_size] = L'\0';
    GlobalUnlock(memory);
    if (!SetClipboardData(CF_UNICODETEXT, memory)) {
        GlobalFree(memory);
        CloseClipboard();
        error = "Clipboard write failed.";
        return false;
    }
    CloseClipboard();
    return true;
}

bool get_clipboard_text(std::string& text, std::string& error)
{
    if (!OpenClipboard(nullptr)) {
        error = "Clipboard is unavailable.";
        return false;
    }
    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    const wchar_t* source = data ? static_cast<const wchar_t*>(GlobalLock(data)) : nullptr;
    if (!source) {
        CloseClipboard();
        error = "Clipboard does not contain text.";
        return false;
    }
    const size_t length = wcsnlen_s(source, ConfigManager::maximum_document_size * 2);
    const int utf8_size = WideCharToMultiByte(CP_UTF8, 0, source, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
    text.resize(static_cast<size_t>(utf8_size));
    WideCharToMultiByte(CP_UTF8, 0, source, static_cast<int>(length), text.data(), utf8_size, nullptr, nullptr);
    GlobalUnlock(data);
    CloseClipboard();
    if (text.size() > ConfigManager::maximum_document_size * 2) {
        error = "Clipboard configuration is too large.";
        return false;
    }
    return true;
}
}

ConfigManager::ConfigManager(std::filesystem::path profile_directory)
    : directory_(profile_directory.empty() ? default_directory() : std::move(profile_directory))
{
}

std::string ConfigManager::serialize(const AppSettings& value)
{
    const EspSettings& esp = value.esp;
    json document{
        {"version", schema_version},
        {"theme", std::string(theme_key(value.theme))},
        {"custom_theme", custom_theme_json(value.custom_theme)},
        {"interface", {{"menu_opacity", value.controls.menu_opacity}, {"menu_hotkey", value.controls.menu_hotkey},
            {"unload_hotkey", value.controls.unload_hotkey}, {"aim_toggle_hotkey", value.controls.aim_toggle_hotkey},
            {"esp_toggle_hotkey", value.controls.esp_toggle_hotkey}}},
        {"aim", {{"enabled", value.aim.enabled}, {"always_on", value.aim.always_on}, {"ignore_fov", value.aim.ignore_fov},
            {"ignore_spawn_protected_targets", value.aim.ignore_spawn_protected_targets}, {"wallbang", value.aim.wallbang},
            {"fov", value.aim.fov}, {"hotkey", value.aim.hotkey}}},
        {"esp", {{"enabled", esp.enabled}, {"boxes", esp.boxes}, {"box_style", box_style_name(esp.box_style)},
            {"filled_boxes", esp.filled_boxes}, {"fill_opacity", esp.fill_opacity}, {"box_thickness", esp.box_thickness},
            {"names", esp.names}, {"show_distance", esp.show_distance}, {"text_scale", esp.text_scale},
            {"snaplines", esp.snaplines}, {"snapline_origin", snapline_name(esp.snapline_origin)},
            {"offscreen_arrows", esp.offscreen_arrows}, {"target_marker", esp.target_marker},
            {"maximum_distance", esp.maximum_distance}, {"show_team", esp.show_team}, {"show_enemies", esp.show_enemies},
            {"show_aim_fov", esp.show_aim_fov}, {"team_color", color_json(esp.team_color)},
            {"enemy_color", color_json(esp.enemy_color)}, {"target_color", color_json(esp.target_color)},
            {"aim_fov_color", color_json(esp.aim_fov_color)}, {"aim_fov_thickness", esp.aim_fov_thickness},
            {"aim_fov_opacity", esp.aim_fov_opacity}}},
        {"weapons", {{"no_spread", value.weapons.no_spread}, {"infinite_ammo", value.weapons.infinite_ammo},
            {"instant_reload", value.weapons.instant_reload}, {"reload_time", value.weapons.reload_time},
            {"no_camera_shake", value.weapons.no_camera_shake}, {"rapid_fire", value.weapons.rapid_fire}}},
        {"movement", {{"auto_sprint", value.movement.auto_sprint}, {"no_gravity", value.movement.no_gravity},
            {"custom_gravity", value.movement.custom_gravity}, {"high_speed", value.movement.high_speed},
            {"gravity", value.movement.gravity}, {"speed", value.movement.speed}}},
        {"player", {{"custom_fov", value.player.custom_fov},
            {"disable_spawn_protection", value.player.disable_spawn_protection}, {"camera_fov", value.player.camera_fov}}},
    };
    return document.dump();
}

ConfigValidation ConfigManager::deserialize(const std::string& document)
{
    ConfigValidation result;
    if (document.empty() || document.size() > maximum_document_size) {
        result.message = document.empty() ? "Configuration is empty." : "Configuration exceeds 256 KiB.";
        return result;
    }
    try {
        const json root = json::parse(document);
        const int version = root.at("version").get<int>();
        if (version != schema_version) {
            result.message = version > schema_version ? "Configuration uses an unsupported future version." : "Configuration version is unsupported.";
            return result;
        }
        AppSettings temporary;
        temporary.theme = parse_theme(root.at("theme"));
        if (root.contains("custom_theme")) temporary.custom_theme = parse_custom_theme(root.at("custom_theme"));
        if (root.contains("interface")) {
            const json& interface = root.at("interface");
            temporary.controls.menu_opacity = std::clamp(interface.at("menu_opacity").get<float>(), 25.0f, 100.0f);
            temporary.controls.menu_hotkey = interface.at("menu_hotkey").get<int>();
            temporary.controls.unload_hotkey = interface.at("unload_hotkey").get<int>();
            temporary.controls.aim_toggle_hotkey = interface.at("aim_toggle_hotkey").get<int>();
            temporary.controls.esp_toggle_hotkey = interface.at("esp_toggle_hotkey").get<int>();
        }
        const json& aim = root.at("aim");
        temporary.aim.enabled = aim.at("enabled").get<bool>();
        temporary.aim.always_on = aim.at("always_on").get<bool>();
        temporary.aim.ignore_fov = aim.at("ignore_fov").get<bool>();
        temporary.aim.ignore_spawn_protected_targets = aim.value("ignore_spawn_protected_targets", false);
        temporary.aim.wallbang = aim.at("wallbang").get<bool>();
        temporary.aim.fov = feature_limits::aim_fov(aim.at("fov").get<float>());
        temporary.aim.hotkey = std::clamp(aim.at("hotkey").get<int>(), 0, 255);

        const std::array hotkeys{temporary.controls.menu_hotkey, temporary.controls.unload_hotkey,
            temporary.controls.aim_toggle_hotkey, temporary.controls.esp_toggle_hotkey, temporary.aim.hotkey};
        if (!is_bindable_hotkey(hotkeys[0]) || !is_bindable_hotkey(hotkeys[1])) {
            throw std::invalid_argument("menu and unload hotkeys must be bindable keys");
        }
        for (size_t index = 2; index < hotkeys.size(); ++index) {
            if (hotkeys[index] != 0 && !is_bindable_hotkey(hotkeys[index])) throw std::invalid_argument("hotkey is reserved");
        }
        for (size_t left = 0; left < hotkeys.size(); ++left) {
            if (hotkeys[left] == 0) continue;
            for (size_t right = left + 1; right < hotkeys.size(); ++right) {
                if (hotkeys[left] == hotkeys[right]) throw std::invalid_argument("hotkeys must be unique");
            }
        }

        const json& esp = root.at("esp");
        temporary.esp.enabled = esp.at("enabled").get<bool>();
        temporary.esp.boxes = esp.at("boxes").get<bool>();
        temporary.esp.box_style = parse_box_style(esp.at("box_style"));
        temporary.esp.filled_boxes = esp.at("filled_boxes").get<bool>();
        temporary.esp.fill_opacity = feature_limits::unit(esp.at("fill_opacity").get<float>());
        temporary.esp.box_thickness = feature_limits::esp_thickness(esp.at("box_thickness").get<float>());
        temporary.esp.names = esp.at("names").get<bool>();
        temporary.esp.show_distance = esp.at("show_distance").get<bool>();
        temporary.esp.text_scale = feature_limits::esp_text_scale(esp.at("text_scale").get<float>());
        temporary.esp.snaplines = esp.at("snaplines").get<bool>();
        temporary.esp.snapline_origin = parse_snapline(esp.at("snapline_origin"));
        temporary.esp.offscreen_arrows = esp.at("offscreen_arrows").get<bool>();
        temporary.esp.target_marker = esp.at("target_marker").get<bool>();
        temporary.esp.maximum_distance = feature_limits::esp_distance(esp.at("maximum_distance").get<float>());
        temporary.esp.show_team = esp.at("show_team").get<bool>();
        temporary.esp.show_enemies = esp.at("show_enemies").get<bool>();
        temporary.esp.show_aim_fov = esp.at("show_aim_fov").get<bool>();
        temporary.esp.team_color = parse_color(esp.at("team_color"));
        temporary.esp.enemy_color = parse_color(esp.at("enemy_color"));
        temporary.esp.target_color = parse_color(esp.at("target_color"));
        temporary.esp.aim_fov_color = parse_color(esp.at("aim_fov_color"));
        temporary.esp.aim_fov_thickness = feature_limits::esp_thickness(esp.at("aim_fov_thickness").get<float>());
        temporary.esp.aim_fov_opacity = feature_limits::unit(esp.at("aim_fov_opacity").get<float>());

        const json& weapons = root.at("weapons");
        temporary.weapons.no_spread = weapons.at("no_spread").get<bool>();
        temporary.weapons.infinite_ammo = weapons.at("infinite_ammo").get<bool>();
        temporary.weapons.instant_reload = weapons.at("instant_reload").get<bool>();
        temporary.weapons.reload_time = feature_limits::reload_time(weapons.value("reload_time", 0.0f));
        temporary.weapons.no_camera_shake = weapons.at("no_camera_shake").get<bool>();
        temporary.weapons.rapid_fire = weapons.at("rapid_fire").get<bool>();

        const json& movement = root.at("movement");
        temporary.movement.auto_sprint = movement.at("auto_sprint").get<bool>();
        temporary.movement.no_gravity = movement.at("no_gravity").get<bool>();
        temporary.movement.custom_gravity = movement.at("custom_gravity").get<bool>();
        temporary.movement.high_speed = movement.at("high_speed").get<bool>();
        temporary.movement.gravity = feature_limits::gravity(movement.at("gravity").get<float>());
        temporary.movement.speed = feature_limits::speed(movement.at("speed").get<float>());

        const json& player = root.at("player");
        temporary.player.custom_fov = player.at("custom_fov").get<bool>();
        temporary.player.disable_spawn_protection = player.at("disable_spawn_protection").get<bool>();
        temporary.player.camera_fov = feature_limits::camera_fov(player.at("camera_fov").get<float>());

        const std::array finite_values{temporary.controls.menu_opacity, temporary.aim.fov, temporary.esp.fill_opacity, temporary.esp.box_thickness,
            temporary.esp.text_scale, temporary.esp.maximum_distance, temporary.esp.aim_fov_thickness,
            temporary.esp.aim_fov_opacity, temporary.movement.gravity, temporary.movement.speed, temporary.player.camera_fov};
        for (float value : finite_values) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument("numeric values must be finite");
            }
        }
        result.ok = true;
        result.message = std::string("Valid version 1 configuration: theme ") + std::string(theme_key(temporary.theme)) +
            ", aim " + (temporary.aim.enabled ? "enabled" : "disabled") +
            ", ESP " + (temporary.esp.enabled ? "enabled." : "disabled.");
        result.value = temporary;
    } catch (const std::exception& exception) {
        result.message = std::string("Invalid configuration: ") + exception.what();
    }
    return result;
}

bool ConfigManager::valid_profile_name(const std::string& name, std::string& error)
{
    if (name.empty() || name.size() > 48 || name == "." || name == ".." || name.back() == '.' || name.back() == ' ') {
        error = "Profile names must contain 1-48 characters and cannot end with a dot or space.";
        return false;
    }
    constexpr char forbidden[] = "<>:\"/\\|?*";
    if (name.find_first_of(forbidden) != std::string::npos ||
        std::any_of(name.begin(), name.end(), [](unsigned char character) { return character < 32; })) {
        error = "Profile name contains a character Windows cannot use in a file name.";
        return false;
    }
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    const std::array reserved{"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
    const std::string stem = upper.substr(0, upper.find('.'));
    if (std::find(reserved.begin(), reserved.end(), stem) != reserved.end()) {
        error = "Profile name is reserved by Windows.";
        return false;
    }
    return true;
}

std::filesystem::path ConfigManager::path_for(const std::string& name) const
{
    std::filesystem::path filename(std::u8string(name.begin(), name.end()));
    filename += L".json";
    return directory_ / filename;
}

bool ConfigManager::write_atomic(const std::filesystem::path& path, const std::string& contents, std::string& error) const
{
    const std::filesystem::path temporary = path.wstring() + L".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream || !stream.write(contents.data(), static_cast<std::streamsize>(contents.size())) || !stream.flush()) {
            error = "Could not write the temporary profile file.";
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return false;
        }
    }
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "Could not replace the profile file atomically (Windows error " + std::to_string(GetLastError()) + ").";
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }
    return true;
}

bool ConfigManager::remember_active(std::string& error) const
{
    return write_atomic(directory_ / L"active.txt", active_profile_, error);
}

bool ConfigManager::read_profile(const std::string& name, AppSettings& value, std::string& error) const
{
    std::string name_error;
    if (!valid_profile_name(name, name_error)) {
        error = name_error;
        return false;
    }
    const std::filesystem::path profile_path = path_for(name);
    std::error_code size_error;
    const uintmax_t size = std::filesystem::file_size(profile_path, size_error);
    if (!size_error && size > maximum_document_size) {
        error = "Profile exceeds 256 KiB.";
        return false;
    }
    std::ifstream stream(profile_path, std::ios::binary);
    if (!stream) {
        error = "Profile does not exist.";
        return false;
    }
    std::string document((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    const ConfigValidation validation = deserialize(document);
    if (!validation.ok) {
        error = validation.message;
        return false;
    }
    value = validation.value;
    return true;
}

bool ConfigManager::initialize(AppSettings& live, std::string& error)
{
    std::error_code filesystem_error;
    std::filesystem::create_directories(directory_, filesystem_error);
    if (filesystem_error) {
        error = "Could not create the profile directory.";
        return false;
    }
    std::string requested;
    {
        std::ifstream active_file(directory_ / L"active.txt", std::ios::binary);
        requested.assign(std::istreambuf_iterator<char>(active_file), std::istreambuf_iterator<char>());
    }
    std::string name_error;
    if (!requested.empty() && valid_profile_name(requested, name_error)) {
        active_profile_ = requested;
    }
    AppSettings loaded;
    if (!read_profile(active_profile_, loaded, error)) {
        active_profile_ = "Default";
        loaded = AppSettings{};
        if (!write_atomic(path_for(active_profile_), serialize(loaded), error)) {
            return false;
        }
    }
    live = loaded;
    saved_ = loaded;
    return remember_active(error);
}

std::vector<std::string> ConfigManager::profiles() const
{
    std::vector<std::string> result;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(directory_, error)) {
        if (entry.is_regular_file() && entry.path().extension() == L".json") {
            const std::u8string utf8 = entry.path().stem().u8string();
            result.emplace_back(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

const std::string& ConfigManager::active_profile() const { return active_profile_; }
bool ConfigManager::dirty(const AppSettings& live) const { return !(live == saved_); }

bool ConfigManager::create(const std::string& name, const AppSettings& live, std::string& error)
{
    if (!valid_profile_name(name, error)) return false;
    if (std::filesystem::exists(path_for(name))) {
        error = "A profile with that name already exists.";
        return false;
    }
    if (!write_atomic(path_for(name), serialize(live), error)) return false;
    active_profile_ = name;
    saved_ = live;
    return remember_active(error);
}

bool ConfigManager::save(const AppSettings& live, std::string& error)
{
    if (!write_atomic(path_for(active_profile_), serialize(live), error)) return false;
    saved_ = live;
    return true;
}

bool ConfigManager::load(const std::string& name, AppSettings& live, std::string& error)
{
    AppSettings temporary;
    if (!read_profile(name, temporary, error)) return false;
    active_profile_ = name;
    live = temporary;
    saved_ = temporary;
    return remember_active(error);
}

bool ConfigManager::rename_active(const std::string& name, std::string& error)
{
    if (!valid_profile_name(name, error)) return false;
    if (std::filesystem::exists(path_for(name))) {
        error = "A profile with that name already exists.";
        return false;
    }
    std::error_code rename_error;
    std::filesystem::rename(path_for(active_profile_), path_for(name), rename_error);
    if (rename_error) {
        error = "Could not rename the profile.";
        return false;
    }
    active_profile_ = name;
    return remember_active(error);
}

bool ConfigManager::remove(const std::string& name, AppSettings& live, std::string& error)
{
    if (name == "Default") {
        error = "The Default profile cannot be deleted.";
        return false;
    }
    std::error_code remove_error;
    if (!std::filesystem::remove(path_for(name), remove_error) || remove_error) {
        error = "Could not delete the profile.";
        return false;
    }
    if (name == active_profile_) {
        return load("Default", live, error);
    }
    return true;
}

void ConfigManager::reset(AppSettings& live) const { live = AppSettings{}; }

std::string ConfigManager::export_string(const AppSettings& live) const
{
    return "HM1:" + base64url_encode(serialize(live));
}

ConfigValidation ConfigManager::inspect_import(const std::string& encoded) const
{
    if (!encoded.starts_with("HM1:")) {
        return {false, "Clipboard text does not start with HM1:.", {}};
    }
    if (encoded.size() > maximum_document_size * 2) {
        return {false, "Clipboard configuration is too large.", {}};
    }
    std::string decoded;
    if (!base64url_decode(encoded.substr(4), decoded)) {
        return {false, "Clipboard configuration contains malformed base64url.", {}};
    }
    return deserialize(decoded);
}

bool ConfigManager::copy_to_clipboard(const AppSettings& live, std::string& error) const
{
    return set_clipboard_text(export_string(live), error);
}

ConfigValidation ConfigManager::inspect_clipboard(std::string& error) const
{
    std::string text;
    if (!get_clipboard_text(text, error)) {
        return {false, error, {}};
    }
    return inspect_import(text);
}

ConfigManager& config_manager()
{
    static ConfigManager instance;
    return instance;
}
}
