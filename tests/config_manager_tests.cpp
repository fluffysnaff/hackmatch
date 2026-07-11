#include "config_manager.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {
std::filesystem::path unique_test_directory()
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / (L"hackmatch-config-tests-" + std::to_wstring(stamp));
}

}

int main()
{
    using namespace hackmatch;
    const std::filesystem::path directory = unique_test_directory();
    std::filesystem::create_directories(directory);

    AppSettings source;
    source.theme = ThemeId::CatppuccinMocha;
    source.custom_theme.accent = {0.1f, 0.8f, 0.6f, 1.0f};
    source.controls.menu_opacity = 72.0f;
    source.controls.aim_toggle_hotkey = 0x75;
    source.controls.esp_toggle_hotkey = 0x76;
    source.aim.enabled = true;
    source.aim.always_on = true;
    source.aim.fov = 73.0f;
    source.esp.enabled = true;
    source.esp.box_style = BoxStyle::Corner;
    source.esp.filled_boxes = true;
    source.esp.fill_opacity = 0.31f;
    source.esp.box_thickness = 2.5f;
    source.esp.show_distance = true;
    source.esp.text_scale = 1.4f;
    source.esp.snaplines = true;
    source.esp.snapline_origin = SnaplineOrigin::Crosshair;
    source.esp.offscreen_arrows = true;
    source.esp.target_marker = true;
    source.esp.maximum_distance = 800.0f;
    source.esp.enemy_color = {true, {0.2f, 0.3f, 0.4f, 0.8f}};
    source.weapons.rapid_fire = true;
    source.movement.high_speed = true;
    source.movement.speed = 44.0f;
    source.player.custom_fov = true;
    source.player.camera_fov = 110.0f;

    const std::string json = ConfigManager::serialize(source);
    const ConfigValidation roundtrip = ConfigManager::deserialize(json);
    assert(roundtrip.ok);
    assert(roundtrip.value == source);
    for (int index = 0; index < static_cast<int>(ThemeId::Count); ++index) {
        const ThemeId theme = static_cast<ThemeId>(index);
        assert(theme_from_key(theme_key(theme)) == theme);
    }
    nlohmann::json legacy_json = nlohmann::json::parse(json);
    legacy_json.erase("custom_theme");
    legacy_json.erase("interface");
    legacy_json["esp"]["show_ping"] = true;
    legacy_json["player"]["spoof_ping"] = true;
    legacy_json["player"]["spoof_ping_ms"] = 1;
    const ConfigValidation legacy = ConfigManager::deserialize(legacy_json.dump());
    assert(legacy.ok);
    assert(legacy.value.custom_theme == CustomThemeSettings{});
    assert(legacy.value.controls == InterfaceSettings{});

    ConfigManager manager(directory);
    AppSettings live;
    std::string error;
    assert(manager.initialize(live, error));
    assert(manager.active_profile() == "Default");
    assert(!manager.dirty(live));
    live = source;
    assert(manager.dirty(live));
    assert(manager.save(live, error));
    assert(!manager.dirty(live));
    assert(manager.create("Competitive", live, error));
    assert(manager.rename_active("Shared", error));
    live.aim.fov = 12.0f;
    assert(manager.load("Default", live, error));
    assert(live == source);
    assert(manager.load("Shared", live, error));

    ConfigManager restarted(directory);
    AppSettings restarted_live;
    assert(restarted.initialize(restarted_live, error));
    assert(restarted.active_profile() == "Shared");
    assert(restarted_live == source);
    assert(restarted.remove("Shared", restarted_live, error));
    assert(restarted.active_profile() == "Default");

    const std::string portable = restarted.export_string(source);
    assert(portable.starts_with("HM1:"));
    const ConfigValidation portable_roundtrip = restarted.inspect_import(portable);
    assert(portable_roundtrip.ok && portable_roundtrip.value == source);
    assert(!restarted.inspect_import("not-a-config").ok);
    assert(!restarted.inspect_import("HM1:***").ok);
    assert(!ConfigManager::deserialize("{\"version\":1").ok);
    assert(!ConfigManager::deserialize("{\"version\":2}").ok);
    assert(!ConfigManager::deserialize("{\"version\":1}").ok);
    nlohmann::json invalid_style = nlohmann::json::parse(json);
    invalid_style["esp"]["box_style"] = "diagonal";
    assert(!ConfigManager::deserialize(invalid_style.dump()).ok);
    nlohmann::json invalid_color = nlohmann::json::parse(json);
    invalid_color["esp"]["enemy_color"]["rgba"][0] = 2.0f;
    assert(!ConfigManager::deserialize(invalid_color.dump()).ok);
    assert(!ConfigManager::deserialize(std::string(ConfigManager::maximum_document_size + 1, 'x')).ok);
    nlohmann::json out_of_range = nlohmann::json::parse(json);
    out_of_range["aim"]["fov"] = 999.0f;
    const ConfigValidation clamped = ConfigManager::deserialize(out_of_range.dump());
    assert(clamped.ok && clamped.value.aim.fov == 180.0f);
    nlohmann::json reserved_hotkey = nlohmann::json::parse(json);
    reserved_hotkey["interface"]["menu_hotkey"] = 0x1B;
    assert(!ConfigManager::deserialize(reserved_hotkey.dump()).ok);

    std::string name_error;
    assert(!ConfigManager::valid_profile_name("../escape", name_error));
    assert(!ConfigManager::valid_profile_name("CON", name_error));
    assert(!ConfigManager::valid_profile_name("trailing.", name_error));

    const std::filesystem::path default_path = directory / L"Default.json";
    std::filesystem::remove(default_path);
    std::filesystem::create_directory(default_path);
    restarted_live.aim.enabled = !restarted_live.aim.enabled;
    assert(!restarted.save(restarted_live, error));
    assert(restarted.dirty(restarted_live));

    std::filesystem::remove_all(directory);
    return 0;
}
