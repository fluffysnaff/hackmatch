#include "imgui_menu.h"

#ifndef HACKMATCH_UI_PREVIEW
#include "build_info.h"
#include "esp.h"
#include "gameplay.h"
#endif
#include "game_offsets.h"
#include "settings.h"
#include "logo.h"
#include "theme.h"
#include "widgets.h"

#include <windows.h>
#include <shellapi.h>

#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdio>
#include <initializer_list>
#include <string>

namespace hackmatch {
namespace {
struct HotkeyCapture {
    ImGuiID id = 0;
    std::array<bool, 256> down{};
    bool suppress_actions = false;
    int wait_for_release = 0;
};

HotkeyCapture hotkey_capture;
constexpr const char* page_titles[] = {"Aimbot", "Visuals", "Weapons", "Player", "Config"};
constexpr ui::NavigationIcon page_icons[] = {ui::NavigationIcon::Aim, ui::NavigationIcon::Visuals,
    ui::NavigationIcon::Weapons, ui::NavigationIcon::Player, ui::NavigationIcon::Config};
constexpr float transition_duration = 0.180f;

float ease_out(float value)
{
    value = std::clamp(value, 0.0f, 1.0f);
    const float inverse = 1.0f - value;
    return 1.0f - inverse * inverse * inverse;
}

ImU32 packed(ImVec4 color, float alpha = 1.0f)
{
    color.w *= alpha * ImGui::GetStyle().Alpha;
    return ImGui::ColorConvertFloat4ToU32(color);
}

std::string key_name(int key)
{
    switch (key) {
    case 0: return "None";
    case VK_LBUTTON: return "Left mouse";
    case VK_RBUTTON: return "Right mouse";
    case VK_MBUTTON: return "Middle mouse";
    case VK_XBUTTON1: return "Mouse 4";
    case VK_XBUTTON2: return "Mouse 5";
    default: break;
    }
    const UINT scan = MapVirtualKeyA(static_cast<UINT>(key), MAPVK_VK_TO_VSC_EX);
    LONG parameter = static_cast<LONG>((scan & 0xffU) << 16U);
    const bool extended = (scan & 0xff00U) != 0 || key == VK_INSERT || key == VK_DELETE || key == VK_HOME ||
        key == VK_END || key == VK_PRIOR || key == VK_NEXT || key == VK_LEFT || key == VK_RIGHT || key == VK_UP ||
        key == VK_DOWN || key == VK_DIVIDE || key == VK_NUMLOCK || key == VK_RCONTROL || key == VK_RMENU;
    if (extended) parameter |= 1L << 24;
    char name[64]{};
    if (scan != 0 && GetKeyNameTextA(parameter, name, static_cast<int>(std::size(name))) > 0) return name;
    std::snprintf(name, sizeof(name), "Key 0x%02X", key);
    return name;
}

void update_hotkey_capture()
{
    if (hotkey_capture.wait_for_release != 0 &&
        (GetAsyncKeyState(hotkey_capture.wait_for_release) & 0x8000) == 0) {
        hotkey_capture.wait_for_release = 0;
        hotkey_capture.suppress_actions = false;
    }
}

void hotkey_setting(const char* label, int& key, std::initializer_list<int> conflicts, bool allow_clear = true)
{
    ImGui::TextUnformatted(label);
    const ImGuiID id = ImGui::GetID(label);
    const std::string button_label = (hotkey_capture.id == id ? std::string("Press a key...") : key_name(key)) + "##" + label;
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Button(button_label.c_str(), {allow_clear ? ImGui::GetContentRegionAvail().x - 58.0f : -1.0f, 0.0f})) {
        hotkey_capture.id = id;
        hotkey_capture.suppress_actions = true;
        for (int candidate = 1; candidate <= 255; ++candidate) {
            hotkey_capture.down[static_cast<size_t>(candidate)] = (GetAsyncKeyState(candidate) & 0x8000) != 0;
        }
    }
    if (allow_clear) {
        ImGui::SameLine();
        if (ImGui::Button((std::string("Clear##") + label).c_str())) key = 0;
    }
    if (hotkey_capture.id != id) return;
    for (int candidate = 1; candidate <= 255; ++candidate) {
        const bool down = (GetAsyncKeyState(candidate) & 0x8000) != 0;
        const bool pressed = down && !hotkey_capture.down[static_cast<size_t>(candidate)];
        hotkey_capture.down[static_cast<size_t>(candidate)] = down;
        if (!pressed) continue;
        if (candidate == VK_ESCAPE) {
            hotkey_capture.id = 0;
            hotkey_capture.wait_for_release = candidate;
            return;
        }
        const bool conflict = std::find(conflicts.begin(), conflicts.end(), candidate) != conflicts.end();
        if (!is_bindable_hotkey(candidate) || conflict) continue;
        key = candidate;
        hotkey_capture.id = 0;
        hotkey_capture.wait_for_release = candidate;
        return;
    }
}

int page_column_count()
{
    return ImGui::GetContentRegionAvail().x >= 600.0f ? 2 : 1;
}

void render_aim_page(AppSettings& config)
{
    AimSettings& aim = config.aim;
    if (!ImGui::BeginTable("aim-columns", page_column_count(), ImGuiTableFlags_SizingStretchSame)) return;
    ImGui::TableNextColumn();
    ui::card_begin("aim-main", "Aim assist", nullptr);
    ui::toggle("Enabled", nullptr, aim.enabled);
    ui::toggle("Always active", nullptr, aim.always_on);
    ui::toggle("Ignore field of view", nullptr, aim.ignore_fov);
    ui::toggle("Ignore spawn-protected targets", nullptr, aim.ignore_spawn_protected_targets);
    ui::toggle("Wallbang targeting", "Allow targets behind up to two surfaces", aim.wallbang);
    ui::card_end();
    ImGui::TableNextColumn();
    ui::card_begin("aim-tuning", "Targeting", nullptr);
    ImGui::BeginDisabled(aim.always_on);
    hotkey_setting("Activation key", aim.hotkey, {config.controls.menu_hotkey, config.controls.unload_hotkey,
        config.controls.aim_toggle_hotkey, config.controls.esp_toggle_hotkey});
    ImGui::EndDisabled();
    ImGui::Dummy({0.0f, 5.0f});
    ImGui::BeginDisabled(aim.ignore_fov);
    ui::setting_slider("Aim field of view", aim.fov, 1.0f, 180.0f, "%.0f deg");
    ImGui::EndDisabled();
    ui::card_end();
    ImGui::EndTable();
}

void render_visuals_page(EspSettings& esp)
{
    const ui::ThemePalette& theme = ui::active_palette();
    if (!ImGui::BeginTable("visual-columns", page_column_count(), ImGuiTableFlags_SizingStretchSame)) return;
    ImGui::TableNextColumn();
    ui::card_begin("visuals-overlay", "ESP", nullptr);
    ui::toggle("Enabled", nullptr, esp.enabled);
    ui::toggle("Boxes", nullptr, esp.boxes);
    if (esp.boxes) {
        int style = static_cast<int>(esp.box_style);
        const char* styles[] = {"Full", "Corner"};
        ImGui::TextUnformatted("Box style");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##box-style", &style, styles, 2)) esp.box_style = static_cast<BoxStyle>(style);
        ui::toggle("Filled boxes", nullptr, esp.filled_boxes);
        ImGui::BeginDisabled(!esp.filled_boxes);
        ui::setting_slider("Fill opacity", esp.fill_opacity, 0.0f, 1.0f, "%.2f");
        ImGui::EndDisabled();
        ui::setting_slider("Box thickness", esp.box_thickness, 0.5f, 4.0f, "%.1f px");
    }
    ui::card_end();

    ImGui::TableNextColumn();
    ui::card_begin("visuals-labels", "Labels", nullptr);
    ui::toggle("Names", nullptr, esp.names);
    ui::toggle("Distance", nullptr, esp.show_distance);
    ui::setting_slider("Text scale", esp.text_scale, 0.75f, 1.75f, "%.2fx");
    ui::card_end();

    ImGui::TableNextColumn();
    ui::card_begin("visuals-guides", "Guides", nullptr);
    ui::toggle("Aim field of view", nullptr, esp.show_aim_fov);
    ui::setting_slider("FOV thickness", esp.aim_fov_thickness, 0.5f, 4.0f, "%.1f px");
    ui::setting_slider("FOV opacity", esp.aim_fov_opacity, 0.0f, 1.0f, "%.2f");
    ui::toggle("Snaplines", nullptr, esp.snaplines);
    ImGui::BeginDisabled(!esp.snaplines);
    int origin = static_cast<int>(esp.snapline_origin);
    const char* origins[] = {"Bottom", "Center", "Crosshair"};
    ImGui::TextUnformatted("Snapline origin");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("##snapline-origin", &origin, origins, 3)) esp.snapline_origin = static_cast<SnaplineOrigin>(origin);
    ImGui::EndDisabled();
    ui::toggle("Offscreen arrows", "Point toward included players outside the viewport", esp.offscreen_arrows);
    ui::toggle("Aim target marker", "Mark the target currently selected by aim assist", esp.target_marker);
    ui::card_end();

    ImGui::TableNextColumn();
    ui::card_begin("visuals-filters", "Filters", nullptr);
    ui::toggle("Show teammates", nullptr, esp.show_team);
    ui::toggle("Show enemies", nullptr, esp.show_enemies);
    ui::setting_slider("Maximum distance", esp.maximum_distance, 0.0f, 5000.0f, esp.maximum_distance <= 0.0f ? "Off" : "%.0f m");
    ui::card_end();

    ImGui::TableNextColumn();
    ui::card_begin("visuals-colors", "Colors", nullptr);
    ui::color_setting("Team", esp.team_color, theme.team);
    ui::color_setting("Enemy", esp.enemy_color, theme.enemy);
    ui::color_setting("Current target", esp.target_color, theme.target);
    ui::color_setting("Aim FOV", esp.aim_fov_color, theme.aim_fov);
    ui::card_end();
    ImGui::EndTable();
}

void render_weapons_page(WeaponSettings& weapons)
{
    ImVec4 caution_background = ui::active_palette().warning;
    caution_background.w = 0.10f;
    ImVec4 danger = ui::to_imgui(ui::active_palette().enemy);
    ImVec4 danger_background = danger;
    danger_background.w = 0.12f;
    if (!ImGui::BeginTable("weapon-columns", page_column_count(), ImGuiTableFlags_SizingStretchSame)) return;
    ImGui::TableNextColumn();
    ui::card_begin("weapons-handling", "Weapon handling", "Apply reversible changes to equipped item data.");
    ui::toggle("No spread", "Remove player and projectile spread", weapons.no_spread);
    ui::toggle("Infinite ammo", "CAUTION - Detection risk", weapons.infinite_ammo,
        &caution_background, &ui::active_palette().warning);
    ui::toggle("Instant reload", "CAUTION - Detection risk", weapons.instant_reload,
        &caution_background, &ui::active_palette().warning);
    ImGui::BeginDisabled(!weapons.instant_reload);
    ui::setting_slider("Reload time", weapons.reload_time, 0.0f, 2.0f, "%.2f s");
    ImGui::EndDisabled();
    ui::toggle("No camera shake", "Remove shot camera movement", weapons.no_camera_shake);
    ui::card_end();
    ImGui::TableNextColumn();
    ui::card_begin("weapons-output", "Output", "Modify firing cadence.");
    ui::toggle("Rapid fire", "HIGH BAN RISK - May directly cause bans", weapons.rapid_fire, &danger_background, &danger);
    ui::card_end();
    ImGui::EndTable();
}

void render_player_page(MovementSettings& movement, PlayerSettings& player)
{
    if (!ImGui::BeginTable("player-columns", page_column_count(), ImGuiTableFlags_SizingStretchSame)) return;
    ImGui::TableNextColumn();
    ui::card_begin("player-movement", "Movement", nullptr);
    ui::toggle("Auto sprint", nullptr, movement.auto_sprint);
    ui::toggle("High speed", nullptr, movement.high_speed);
    ImGui::BeginDisabled(!movement.high_speed);
    ui::setting_slider("Movement speed", movement.speed, 5.0f, 80.0f, "%.0f");
    ImGui::EndDisabled();
    ui::toggle("No gravity", nullptr, movement.no_gravity);
    ui::toggle("Custom gravity", nullptr, movement.custom_gravity);
    ImGui::BeginDisabled(!movement.custom_gravity || movement.no_gravity);
    ui::setting_slider("Gravity", movement.gravity, -50.0f, 50.0f, "%.1f");
    ImGui::EndDisabled();
    ui::card_end();
    ImGui::TableNextColumn();
    ui::card_begin("player-local", "Local player", nullptr);
    ui::toggle("Custom camera FOV", nullptr, player.custom_fov);
    ImGui::BeginDisabled(!player.custom_fov);
    ui::setting_slider("Camera field of view", player.camera_fov, 40.0f, 140.0f, "%.0f deg");
    ImGui::EndDisabled();
    ui::toggle("Disable spawn protection", nullptr, player.disable_spawn_protection);
    ui::card_end();
    ImGui::EndTable();
}
}

bool ImGuiMenu::init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (ready_ || !window || !device || !context) return ready_;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ui::apply_theme(settings().theme);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;
#ifdef HACKMATCH_UI_PREVIEW
    io.IniFilename = nullptr;
#endif
    char windows_directory[MAX_PATH]{};
    if (GetWindowsDirectoryA(windows_directory, MAX_PATH) > 0) {
        const std::string font_directory = std::string(windows_directory) + "\\Fonts\\";
        body_font_ = io.Fonts->AddFontFromFileTTF((font_directory + "segoeui.ttf").c_str(), 15.0f);
        if (!body_font_) body_font_ = io.Fonts->AddFontDefault();
        strong_font_ = io.Fonts->AddFontFromFileTTF((font_directory + "seguisb.ttf").c_str(), 15.0f);
        title_font_ = io.Fonts->AddFontFromFileTTF((font_directory + "seguisb.ttf").c_str(), 27.0f);
    }
    if (!body_font_) body_font_ = io.Fonts->AddFontDefault();
    if (!title_font_) title_font_ = strong_font_ ? strong_font_ : body_font_;
    if (!strong_font_) strong_font_ = body_font_;
    io.FontDefault = body_font_;

    BOOL client_animations = TRUE;
    motion_enabled_ = !SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &client_animations, 0) || client_animations != FALSE;
    ui::configure_widgets(strong_font_, motion_enabled_);
    if (!ImGui_ImplWin32_Init(window)) {
        ImGui::DestroyContext(); return false;
    }
    if (!ImGui_ImplDX11_Init(device, context)) {
        ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); return false;
    }
    if (!ui::init_logo(device)) {
        ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); return false;
    }
#ifndef HACKMATCH_UI_PREVIEW
    config_ready_ = config_manager().initialize(settings(), profile_status_);
    ui::apply_theme(settings().theme);
#else
    config_ready_ = true;
#endif
    window_ = window;
    ready_ = true;
    return true;
}

void ImGuiMenu::start_page_transition(int page)
{
    if (page == page_ || page < 0 || page >= static_cast<int>(std::size(page_titles))) return;
    previous_page_ = page_;
    for (int index = 0; index < 5; ++index) navigation_start_mix_[index] = navigation_mix_[index];
    navigation_elapsed_ = 0.0f;
    indicator_start_ = indicator_position_;
    const bool content_was_animating = content_elapsed_ < (motion_enabled_ ? transition_duration : 0.100f);
    content_start_alpha_ = content_was_animating ? content_alpha_ : 0.55f;
    content_start_offset_ = motion_enabled_ ? (content_was_animating ? content_offset_ : (page > page_ ? 12.0f : -12.0f)) : 0.0f;
    content_elapsed_ = 0.0f;
    page_ = page;
}

void ImGuiMenu::update_animations()
{
#ifdef HACKMATCH_UI_PREVIEW
    if (preview_animation_override_) {
        const float nav = ease_out(preview_navigation_progress_);
        for (int index = 0; index < 5; ++index) navigation_mix_[index] = 0.0f;
        navigation_mix_[previous_page_] = 1.0f - nav;
        navigation_mix_[page_] = nav;
        indicator_position_ = static_cast<float>(previous_page_) + (static_cast<float>(page_ - previous_page_) * nav);
        const float content = ease_out(preview_content_progress_);
        content_alpha_ = 0.55f + 0.45f * content;
        content_offset_ = motion_enabled_ ? (page_ >= previous_page_ ? 12.0f : -12.0f) * (1.0f - content) : 0.0f;
        return;
    }
#endif
    const float delta = ImGui::GetIO().DeltaTime;
    navigation_elapsed_ += delta;
    const float navigation = ease_out(navigation_elapsed_ / transition_duration);
    for (int index = 0; index < 5; ++index) {
        const float target = index == page_ ? 1.0f : 0.0f;
        navigation_mix_[index] = navigation_start_mix_[index] + (target - navigation_start_mix_[index]) * navigation;
    }
    indicator_position_ = indicator_start_ + (static_cast<float>(page_) - indicator_start_) * navigation;
    const float duration = motion_enabled_ ? transition_duration : 0.100f;
    content_elapsed_ += delta;
    const float content = ease_out(content_elapsed_ / duration);
    content_alpha_ = content_start_alpha_ + (1.0f - content_start_alpha_) * content;
    content_offset_ = motion_enabled_ ? content_start_offset_ * (1.0f - content) : 0.0f;
}

void ImGuiMenu::perform_profile_action(int action, const std::string& profile)
{
    std::string error;
    bool ok = false;
    if (action == 1) ok = config_manager().load(profile, settings(), error);
    else if (action == 2) ok = config_manager().remove(profile, settings(), error);
    if (ok) {
        ui::apply_theme(settings().theme);
        profile_status_ = action == 1 ? "Loaded '" + profile + "'." : "Deleted '" + profile + "'.";
    } else profile_status_ = error;
}

void ImGuiMenu::request_profile_action(int action, const std::string& profile)
{
    if (config_manager().dirty(settings())) {
        pending_profile_action_ = action;
        pending_profile_ = profile;
        ImGui::OpenPopup("Unsaved changes");
    } else perform_profile_action(action, profile);
}

void ImGuiMenu::render_config_page(AppSettings& config)
{
    if (!ImGui::BeginTable("config-columns", page_column_count(), ImGuiTableFlags_SizingStretchSame)) return;
    ImGui::TableNextColumn();
    ui::card_begin("config-theme", "Appearance", nullptr);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##theme", ui::palette(config.theme).name)) {
        for (const ui::ThemePalette& theme : ui::themes()) {
            if (ImGui::Selectable(theme.name, config.theme == theme.id)) {
                config.theme = theme.id;
                ui::apply_theme(config.theme);
            }
        }
        if (ImGui::Selectable("Custom", config.theme == ThemeId::Custom)) {
            config.theme = ThemeId::Custom;
            ui::apply_theme(config.theme);
        }
        ImGui::EndCombo();
    }
    if (config.theme == ThemeId::Custom) {
        bool changed = false;
        changed |= ImGui::ColorEdit3("Background", &config.custom_theme.background.r);
        changed |= ImGui::ColorEdit3("Surface", &config.custom_theme.surface.r);
        changed |= ImGui::ColorEdit3("Accent", &config.custom_theme.accent.r);
        changed |= ImGui::ColorEdit3("Text", &config.custom_theme.text.r);
        changed |= ImGui::ColorEdit3("Muted", &config.custom_theme.muted.r);
        if (changed) ui::apply_theme(config.theme);
    }
    ui::setting_slider("Menu opacity", config.controls.menu_opacity, 25.0f, 100.0f, "%.0f%%");
    ui::card_end();

    ImGui::TableNextColumn();
    ui::card_begin("config-profiles", "Profiles", nullptr);
    const std::vector<std::string> profiles = config_manager().profiles();
    const std::string& active = config_manager().active_profile();
    if (ImGui::BeginCombo("##profile", active.c_str())) {
        for (const std::string& profile : profiles) {
            if (ImGui::Selectable(profile.c_str(), profile == active) && profile != active) request_profile_action(1, profile);
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    const bool dirty = config_manager().dirty(config);
    ImGui::TextColored(dirty ? ui::active_palette().warning : ui::active_palette().success, dirty ? "Unsaved" : "Saved");
    if (ImGui::Button("New")) { profile_name_[0] = '\0'; ImGui::OpenPopup("New profile"); }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        std::string error;
        profile_status_ = config_manager().save(config, error) ? "Profile saved." : error;
    }
    ImGui::SameLine();
    if (ImGui::Button("Rename")) {
        std::snprintf(profile_name_, sizeof(profile_name_), "%s", active.c_str());
        ImGui::OpenPopup("Rename profile");
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(active == "Default");
    if (ImGui::Button("Delete")) request_profile_action(2, active);
    ImGui::EndDisabled();
    if (ImGui::Button("Reset")) { config_manager().reset(config); ui::apply_theme(config.theme); profile_status_ = "Defaults applied; save to keep them."; }
    ImGui::SameLine();
    if (ImGui::Button("Copy current config")) {
        std::string error;
        profile_status_ = config_manager().copy_to_clipboard(config, error) ? "Current configuration copied." : error;
    }
    ImGui::SameLine();
    if (ImGui::Button("Paste config")) {
        std::string error;
        import_preview_ = config_manager().inspect_clipboard(error);
        ImGui::OpenPopup("Import configuration");
    }
    if (!profile_status_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ui::active_palette().muted);
        ImGui::TextWrapped("%s", profile_status_.c_str());
        ImGui::PopStyleColor();
    }
    ui::card_end();

    ImGui::TableNextColumn();
    ui::card_begin("config-hotkeys", "Hotkeys", nullptr);
    hotkey_setting("Menu", config.controls.menu_hotkey, {config.controls.unload_hotkey,
        config.controls.aim_toggle_hotkey, config.controls.esp_toggle_hotkey, config.aim.hotkey}, false);
    hotkey_setting("Unload", config.controls.unload_hotkey, {config.controls.menu_hotkey,
        config.controls.aim_toggle_hotkey, config.controls.esp_toggle_hotkey, config.aim.hotkey}, false);
    hotkey_setting("Toggle aim", config.controls.aim_toggle_hotkey, {config.controls.menu_hotkey,
        config.controls.unload_hotkey, config.controls.esp_toggle_hotkey, config.aim.hotkey});
    hotkey_setting("Toggle ESP", config.controls.esp_toggle_hotkey, {config.controls.menu_hotkey,
        config.controls.unload_hotkey, config.controls.aim_toggle_hotkey, config.aim.hotkey});
    ui::card_end();

    ImGui::TableNextColumn();
    ImGui::Dummy({0.0f, 4.0f});
    ui::card_begin("config-about", "About", nullptr);
    const ImVec2 avatar_position = ImGui::GetCursorScreenPos();
    ui::draw_avatar(ImGui::GetWindowDrawList(), avatar_position, 64.0f);
    ImGui::Dummy({64.0f, 64.0f});
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::PushFont(strong_font_);
    ImGui::Text("Hackmatch %s by fluffysnaff", HACKMATCH_VERSION);
    ImGui::PopFont();
    ImGui::Text("Redmatch 2 build %.*s", static_cast<int>(game_offsets::supported_build.size()), game_offsets::supported_build.data());
    constexpr const char* repository = "github.com/fluffysnaff/hackmatch";
    ImGui::PushStyleColor(ImGuiCol_Text, ui::active_palette().accent);
    if (ImGui::Selectable(repository, false, ImGuiSelectableFlags_None, ImGui::CalcTextSize(repository))) {
        ShellExecuteA(nullptr, "open", "https://github.com/fluffysnaff/hackmatch", nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    ui::card_end();
    ImGui::EndTable();

    if (ImGui::BeginPopupModal("New profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Create a profile from the current live settings.");
        ImGui::InputText("Name", profile_name_, sizeof(profile_name_));
        if (ImGui::Button("Create")) {
            std::string error;
            if (config_manager().create(profile_name_, config, error)) { profile_status_ = "Profile created."; ImGui::CloseCurrentPopup(); }
            else profile_status_ = error;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Rename profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("New name", profile_name_, sizeof(profile_name_));
        if (ImGui::Button("Rename")) {
            std::string error;
            if (config_manager().rename_active(profile_name_, error)) { profile_status_ = "Profile renamed."; ImGui::CloseCurrentPopup(); }
            else profile_status_ = error;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Import configuration", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", import_preview_.message.c_str());
        ImGui::BeginDisabled(!import_preview_.ok);
        if (ImGui::Button("Apply")) {
            config = import_preview_.value;
            ui::apply_theme(config.theme);
            profile_status_ = "Imported settings applied; save to keep them.";
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Unsaved changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Save the current profile before continuing?");
        if (ImGui::Button("Save and continue")) {
            std::string error;
            if (config_manager().save(config, error)) {
                perform_profile_action(pending_profile_action_, pending_profile_);
                pending_profile_action_ = 0;
                ImGui::CloseCurrentPopup();
            } else profile_status_ = error;
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard and continue")) {
            perform_profile_action(pending_profile_action_, pending_profile_);
            pending_profile_action_ = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) { pending_profile_action_ = 0; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}

void ImGuiMenu::render_options()
{
    AppSettings& config = settings();
    update_animations();
    const ui::ThemePalette& color = ui::active_palette();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 maximum{std::max(760.0f, io.DisplaySize.x - 32.0f), std::max(520.0f, io.DisplaySize.y - 32.0f)};
    ImGui::SetNextWindowPos({16.0f, 16.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({920.0f, 660.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({760.0f, 520.0f}, maximum);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, config.controls.menu_opacity / 100.0f);
    if (!ImGui::Begin("Hackmatch##main", &visible_, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground)) {
        ImGui::End(); ImGui::PopStyleVar(); return;
    }
    const ImVec2 window_position = ImGui::GetWindowPos();
    const ImVec2 window_size = ImGui::GetWindowSize();
    const ImVec2 window_max{window_position.x + window_size.x, window_position.y + window_size.y};
    ImDrawList* root_draw = ImGui::GetWindowDrawList();
    root_draw->AddRectFilled(window_position, window_max, packed(color.background), 10.0f);
    constexpr float sidebar_width = 216.0f;
    root_draw->AddRectFilled(window_position, {window_position.x + sidebar_width, window_max.y}, packed(color.sidebar), 10.0f,
        ImDrawFlags_RoundCornersLeft);
    root_draw->AddLine({window_position.x + sidebar_width, window_position.y + 1.0f},
        {window_position.x + sidebar_width, window_max.y - 1.0f}, packed(color.border));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0, 0, 0, 0});
    ImGui::BeginChild("sidebar", {sidebar_width, window_size.y}, ImGuiChildFlags_None);
    const ImVec2 sidebar_position = ImGui::GetWindowPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ui::draw_logo(draw, {sidebar_position.x + 16.0f, sidebar_position.y + 14.0f}, 44.0f, packed(color.accent));
    ImGui::SetCursorPos({70.0f, 18.0f});
    ImGui::PushFont(strong_font_); ImGui::TextUnformatted("Hackmatch"); ImGui::PopFont();
    draw->AddLine({sidebar_position.x + 20.0f, sidebar_position.y + 72.0f},
        {sidebar_position.x + sidebar_width - 20.0f, sidebar_position.y + 72.0f}, packed(color.border));
    draw->AddText({sidebar_position.x + 20.0f, sidebar_position.y + 88.0f}, packed(color.muted), "MODULES");

    constexpr float navigation_x = 14.0f;
    constexpr float navigation_y = 112.0f;
    constexpr float navigation_step = 56.0f;
    const float indicator_y = sidebar_position.y + navigation_y + navigation_step * indicator_position_;
    draw->AddRectFilled({sidebar_position.x + 14.0f, indicator_y + 8.0f}, {sidebar_position.x + 17.0f, indicator_y + 40.0f},
        packed(color.accent), 2.0f);
    for (int index = 0; index < 5; ++index) {
        ImGui::SetCursorPos({navigation_x, navigation_y + navigation_step * static_cast<float>(index)});
        if (ui::navigation_item(page_titles[index], page_icons[index], navigation_mix_[index])) start_page_transition(index);
    }

    bool build_warning = false;
#ifndef HACKMATCH_UI_PREVIEW
    build_warning = build_compatibility().status != BuildStatus::Compatible;
#endif
    const ImVec2 status_min{sidebar_position.x + 20.0f, sidebar_position.y + window_size.y - (build_warning ? 124.0f : 116.0f)};
    const ImVec2 status_max{status_min.x + 176.0f, status_min.y + (build_warning ? 40.0f : 32.0f)};
    draw->AddRectFilled(status_min, status_max, packed(color.card), 7.0f);
    draw->AddRect(status_min, status_max, packed(build_warning ? color.warning : color.border_strong), 7.0f);
    if (build_warning) {
        draw->AddText({status_min.x + 9.0f, status_min.y + 5.0f}, packed(color.warning), "Unsupported game build");
        draw->AddText({status_min.x + 9.0f, status_min.y + 21.0f}, packed(color.muted), "Hackmatch might not work");
    } else {
        draw->AddCircleFilled({status_min.x + 14.0f, status_min.y + 16.0f}, 4.0f, packed(color.success));
        draw->AddText({status_min.x + 26.0f, status_min.y + 7.0f}, packed(color.text), "Runtime connected");
    }
    const auto draw_keycap = [&](float y, const char* key, const char* label) {
        const ImVec2 key_min{sidebar_position.x + 20.0f, sidebar_position.y + y};
        const ImVec2 key_max{key_min.x + 52.0f, key_min.y + 24.0f};
        draw->AddRectFilled(key_min, key_max, packed(color.background), 5.0f);
        draw->AddRect(key_min, key_max, packed(color.border_strong), 5.0f);
        const ImVec2 key_size = ImGui::CalcTextSize(key);
        draw->AddText({key_min.x + (52.0f - key_size.x) * 0.5f, key_min.y + 4.0f}, packed(color.text), key);
        draw->AddText({key_max.x + 8.0f, key_min.y + 3.0f}, packed(color.muted), label);
    };
    const std::string menu_key = key_name(config.controls.menu_hotkey);
    const std::string unload_key = key_name(config.controls.unload_hotkey);
    draw_keycap(window_size.y - 72.0f, menu_key.c_str(), "Close menu");
    draw_keycap(window_size.y - 40.0f, unload_key.c_str(), "Unload");
    ImGui::EndChild();

    ImGui::SameLine(sidebar_width, 0.0f);
    ImGui::BeginChild("content", {window_size.x - sidebar_width, window_size.y}, ImGuiChildFlags_None);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, content_alpha_);
    constexpr float content_x = 32.0f;
    const float content_y = content_offset_;
    ImGui::SetCursorPos({content_x, 24.0f + content_y});
    ImGui::PushStyleColor(ImGuiCol_Text, color.accent);
    ImGui::Text("MODULE %02d / %02d", page_ + 1, 5);
    ImGui::PopStyleColor();
    ImGui::SetCursorPos({content_x, 48.0f + content_y});
    ImGui::PushFont(title_font_); ImGui::TextUnformatted(page_titles[page_]); ImGui::PopFont();
    draw = ImGui::GetWindowDrawList();
    const ImVec2 content_position = ImGui::GetWindowPos();
    draw->AddLine({content_position.x + content_x, content_position.y + 92.0f + content_y},
        {content_position.x + window_size.x - sidebar_width - 32.0f, content_position.y + 92.0f + content_y}, packed(color.border));
    ImGui::SetCursorPos({content_x, 112.0f + content_y});
    ImGui::BeginChild("page-scroll", {window_size.x - sidebar_width - content_x - 24.0f, window_size.y - 132.0f - content_y}, ImGuiChildFlags_None);
    switch (page_) {
    case 0: render_aim_page(config); break;
    case 1: render_visuals_page(config.esp); break;
    case 2: render_weapons_page(config.weapons); break;
    case 3: render_player_page(config.movement, config.player); break;
    case 4: render_config_page(config); break;
    default: break;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::GetForegroundDrawList()->AddRect({window_position.x + 0.5f, window_position.y + 0.5f}, {window_max.x - 0.5f, window_max.y - 0.5f},
        packed(color.border_strong), 9.5f, 0, 1.0f);
    ImGui::End();
    ImGui::PopStyleVar();
}

void ImGuiMenu::update_cursor()
{
    ImGuiIO& io = ImGui::GetIO();
    const HWND foreground = GetForegroundWindow();
    const bool focused = foreground == window_ || (foreground && IsChild(window_, foreground));
    if (!visible_ || !focused) {
        if (cursor_mode_ != CursorMode::Inactive) { cursor_mode_ = CursorMode::Inactive; virtual_mouse_ready_ = false; }
        io.MouseDrawCursor = false; io.AddMousePosEvent(-FLT_MAX, -FLT_MAX); return;
    }
    CURSORINFO cursor_info{}; cursor_info.cbSize = sizeof(cursor_info);
    const bool native_cursor_visible = GetCursorInfo(&cursor_info) && (cursor_info.flags & CURSOR_SHOWING) != 0;
    const CursorMode requested_mode = native_cursor_visible ? CursorMode::Native : CursorMode::Virtual;
    if (requested_mode != cursor_mode_) { cursor_mode_ = requested_mode; virtual_mouse_ready_ = false; }
    if (cursor_mode_ == CursorMode::Native) {
        POINT position{};
        if (GetCursorPos(&position) && ScreenToClient(window_, &position)) {
            last_mouse_x_ = static_cast<float>(position.x); last_mouse_y_ = static_cast<float>(position.y);
            last_mouse_valid_ = true; io.AddMousePosEvent(last_mouse_x_, last_mouse_y_);
        } else io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        io.MouseDrawCursor = false; return;
    }
    if (!virtual_mouse_ready_) {
        virtual_mouse_x_ = last_mouse_valid_ ? last_mouse_x_ : io.DisplaySize.x * 0.5f;
        virtual_mouse_y_ = last_mouse_valid_ ? last_mouse_y_ : io.DisplaySize.y * 0.5f;
        virtual_mouse_ready_ = true;
    }
    virtual_mouse_x_ = std::clamp(virtual_mouse_x_, 0.0f, io.DisplaySize.x);
    virtual_mouse_y_ = std::clamp(virtual_mouse_y_, 0.0f, io.DisplaySize.y);
    last_mouse_x_ = virtual_mouse_x_; last_mouse_y_ = virtual_mouse_y_; last_mouse_valid_ = true;
    io.MouseDrawCursor = true; io.AddMousePosEvent(virtual_mouse_x_, virtual_mouse_y_);
}

void ImGuiMenu::render()
{
    if (!ready_) return;
    update_hotkey_capture();
    ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); update_cursor(); ImGui::NewFrame();
#ifndef HACKMATCH_UI_PREVIEW
    gameplay().tick(); esp().render();
#endif
    if (visible_) render_options();
    ImGui::Render(); ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiMenu::shutdown()
{
    if (!ready_) return;
    ui::shutdown_logo();
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    ready_ = false; visible_ = false; virtual_mouse_ready_ = false; last_mouse_valid_ = false;
    cursor_mode_ = CursorMode::Inactive; window_ = nullptr; body_font_ = nullptr; strong_font_ = nullptr; title_font_ = nullptr;
}

void ImGuiMenu::toggle() { visible_ = !visible_; cursor_mode_ = CursorMode::Inactive; virtual_mouse_ready_ = false; }
bool ImGuiMenu::visible() const { return visible_; }
bool ImGuiMenu::capturing_hotkey() const { return hotkey_capture.suppress_actions; }
void ImGuiMenu::add_mouse_delta(float x, float y)
{
    if (visible_ && cursor_mode_ == CursorMode::Virtual && virtual_mouse_ready_) { virtual_mouse_x_ += x; virtual_mouse_y_ += y; }
}

#ifdef HACKMATCH_UI_PREVIEW
void ImGuiMenu::select_page_for_preview(int page)
{
    set_preview_state(0, page, 1.0f, 1.0f, false);
}

void ImGuiMenu::set_preview_state(int theme, int page, float navigation_progress, float content_progress, bool reduced_motion)
{
    settings().theme = static_cast<ThemeId>(std::clamp(theme, 0, static_cast<int>(ThemeId::Custom)));
    ui::apply_theme(settings().theme);
    previous_page_ = std::clamp(page - 1, 0, 4);
    page_ = std::clamp(page, 0, 4);
    preview_animation_override_ = true;
    preview_navigation_progress_ = std::clamp(navigation_progress, 0.0f, 1.0f);
    preview_content_progress_ = std::clamp(content_progress, 0.0f, 1.0f);
    motion_enabled_ = !reduced_motion;
    ui::configure_widgets(strong_font_, motion_enabled_);
}
#endif

ImGuiMenu& imgui_menu()
{
    static ImGuiMenu instance;
    return instance;
}
}
