#include "esp.h"

#include "esp_players.h"
#include "gameplay.h"
#include "il2cpp_api.h"
#include "settings.h"
#include "theme.h"

#include <windows.h>

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace hackmatch {
namespace {
struct PlayerBounds { ImVec2 min; ImVec2 max; };

ImU32 rgba(const Rgba& color, float alpha = 1.0f)
{
    return ImGui::ColorConvertFloat4ToU32(ui::to_imgui(color, alpha));
}

bool selected_target(const EspPlayer& player)
{
    return player.object && player.object == gameplay().target_player();
}

Rgba player_rgba(const EspPlayer& player)
{
    const EspSettings& config = settings().esp;
    const ui::ThemePalette& theme = ui::active_palette();
    if (selected_target(player)) return ui::effective_color(config.target_color, theme.target);
    return esp_players().is_team(player) ? ui::effective_color(config.team_color, theme.team) :
                                          ui::effective_color(config.enemy_color, theme.enemy);
}

bool passes_filter(const EspPlayer& player)
{
    const EspSettings& config = settings().esp;
    if (!player.projected || (esp_players().is_team(player) ? !config.show_team : !config.show_enemies)) return false;
    return config.maximum_distance <= 0.0f || player.distance <= config.maximum_distance;
}

PlayerBounds player_bounds(const EspPlayer& player)
{
    const float height = std::max(10.0f, player.feet.y - player.head.y);
    const float width = height * 0.45f;
    return {{player.head.x - width * 0.5f, player.feet.y - height}, {player.head.x + width * 0.5f, player.feet.y}};
}

void draw_corner_box(ImDrawList* draw, const PlayerBounds& bounds, ImU32 color, float thickness)
{
    const float width = bounds.max.x - bounds.min.x;
    const float height = bounds.max.y - bounds.min.y;
    const float x = std::max(4.0f, width * 0.28f);
    const float y = std::max(6.0f, height * 0.20f);
    draw->AddLine(bounds.min, {bounds.min.x + x, bounds.min.y}, color, thickness);
    draw->AddLine(bounds.min, {bounds.min.x, bounds.min.y + y}, color, thickness);
    draw->AddLine({bounds.max.x - x, bounds.min.y}, {bounds.max.x, bounds.min.y}, color, thickness);
    draw->AddLine({bounds.max.x, bounds.min.y}, {bounds.max.x, bounds.min.y + y}, color, thickness);
    draw->AddLine({bounds.min.x, bounds.max.y - y}, {bounds.min.x, bounds.max.y}, color, thickness);
    draw->AddLine({bounds.min.x, bounds.max.y}, {bounds.min.x + x, bounds.max.y}, color, thickness);
    draw->AddLine({bounds.max.x - x, bounds.max.y}, bounds.max, color, thickness);
    draw->AddLine({bounds.max.x, bounds.max.y - y}, bounds.max, color, thickness);
}

void draw_box(ImDrawList* draw, const EspPlayer& player, const PlayerBounds& bounds)
{
    const EspSettings& config = settings().esp;
    const Rgba color = player_rgba(player);
    if (config.filled_boxes) draw->AddRectFilled(bounds.min, bounds.max, rgba(color, config.fill_opacity));
    const float thickness = config.box_thickness;
    if (config.box_style == BoxStyle::Corner) {
        draw_corner_box(draw, {{bounds.min.x - 1.0f, bounds.min.y - 1.0f}, {bounds.max.x + 1.0f, bounds.max.y + 1.0f}},
            rgba(ui::active_palette().overlay_shadow, 190.0f / 255.0f), thickness + 2.0f);
        draw_corner_box(draw, bounds, rgba(color), thickness);
    } else {
        draw->AddRect({bounds.min.x - 4.0f, bounds.min.y - 4.0f}, {bounds.max.x + 4.0f, bounds.max.y + 4.0f},
            rgba(color, 45.0f / 255.0f), 0.0f, 0, 4.0f);
        draw->AddRect({bounds.min.x - 2.0f, bounds.min.y - 2.0f}, {bounds.max.x + 2.0f, bounds.max.y + 2.0f},
            rgba(color, 90.0f / 255.0f), 0.0f, 0, 2.0f);
        draw->AddRect({bounds.min.x - 1.0f, bounds.min.y - 1.0f}, {bounds.max.x + 1.0f, bounds.max.y + 1.0f},
            rgba(ui::active_palette().overlay_shadow, 190.0f / 255.0f), 0.0f, 0, thickness + 2.0f);
        draw->AddRect(bounds.min, bounds.max, rgba(color), 0.0f, 0, thickness);
    }
}

void draw_label(ImDrawList* draw, const EspPlayer& player, const PlayerBounds& bounds, int index)
{
    const EspSettings& config = settings().esp;
    char label[128]{};
    size_t used = 0;
    if (config.names) {
        used = static_cast<size_t>(std::snprintf(label, sizeof(label), "%s", player.name.empty() ?
            (std::string("P") + std::to_string(index + 1)).c_str() : player.name.c_str()));
    }
    if (config.show_distance && used < sizeof(label)) {
        std::snprintf(label + used, sizeof(label) - used, "%s%.0fm", used ? "  " : "", player.distance);
    }
    if (!label[0]) return;

    ImFont* font = ImGui::GetFont();
    const float font_size = ImGui::GetFontSize() * config.text_scale;
    const ImVec2 size = font->CalcTextSizeA(font_size, 1000.0f, 0.0f, label);
    const ImVec2 position{player.head.x - size.x * 0.5f, bounds.min.y - size.y - 3.0f};
    draw->AddText(font, font_size, {position.x + 1.0f, position.y + 1.0f},
        rgba(ui::active_palette().overlay_shadow, 225.0f / 255.0f), label);
    draw->AddText(font, font_size, position, rgba(player_rgba(player)), label);
}

ImVec2 snapline_origin(const EspPlayer& player)
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 center{display.x * 0.5f, display.y * 0.5f};
    switch (settings().esp.snapline_origin) {
    case SnaplineOrigin::Bottom: return {center.x, display.y};
    case SnaplineOrigin::Center: return center;
    case SnaplineOrigin::Crosshair: {
        ImVec2 direction{player.screen_center.x - center.x, player.screen_center.y - center.y};
        const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.01f) return {center.x + direction.x / length * 12.0f, center.y + direction.y / length * 12.0f};
        return center;
    }
    }
    return center;
}

void draw_target_marker(ImDrawList* draw, const EspPlayer& player)
{
    if (!settings().esp.target_marker || !selected_target(player)) return;
    const ImVec2 center = player.screen_center;
    const ImU32 color = rgba(player_rgba(player));
    draw->AddCircle(center, 8.0f, rgba(ui::active_palette().overlay_shadow, 200.0f / 255.0f), 24, 4.0f);
    draw->AddCircle(center, 8.0f, color, 24, 2.0f);
    draw->AddLine({center.x - 12.0f, center.y}, {center.x - 7.0f, center.y}, color, 2.0f);
    draw->AddLine({center.x + 7.0f, center.y}, {center.x + 12.0f, center.y}, color, 2.0f);
}

void draw_offscreen_arrow(ImDrawList* draw, const EspPlayer& player)
{
    if (!settings().esp.offscreen_arrows || player.visible) return;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 center{display.x * 0.5f, display.y * 0.5f};
    ImVec2 direction{player.screen_center.x - center.x, player.screen_center.y - center.y};
    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 0.01f) direction = {0.0f, -1.0f};
    else direction = {direction.x / length, direction.y / length};
    const float scale = std::min((center.x - 28.0f) / std::max(std::abs(direction.x), 0.001f),
        (center.y - 28.0f) / std::max(std::abs(direction.y), 0.001f));
    const ImVec2 tip{center.x + direction.x * scale, center.y + direction.y * scale};
    const ImVec2 side{-direction.y, direction.x};
    const ImVec2 base{tip.x - direction.x * 13.0f, tip.y - direction.y * 13.0f};
    const ImVec2 points[]{tip, {base.x + side.x * 7.0f, base.y + side.y * 7.0f}, {base.x - side.x * 7.0f, base.y - side.y * 7.0f}};
    draw->AddTriangleFilled(points[0], points[1], points[2], rgba(player_rgba(player)));
    draw->AddPolyline(points, 3, rgba(ui::active_palette().overlay_shadow, 220.0f / 255.0f), ImDrawFlags_Closed, 1.5f);
}

void draw_aim_fov(ImDrawList* draw)
{
    const AppSettings& config = settings();
    if (!config.esp.show_aim_fov || !config.aim.enabled || config.aim.ignore_fov) return;
    constexpr float pi = 3.14159265358979323846f;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float camera_fov = config.player.custom_fov ? config.player.camera_fov : 90.0f;
    const float radius = (display.y * 0.5f) * std::tan(config.aim.fov * 0.5f * pi / 180.0f) /
        std::tan(camera_fov * 0.5f * pi / 180.0f);
    const Rgba color = ui::effective_color(config.esp.aim_fov_color, ui::active_palette().aim_fov);
    draw->AddCircle({display.x * 0.5f, display.y * 0.5f}, radius, rgba(color, config.esp.aim_fov_opacity), 96,
        config.esp.aim_fov_thickness);
}

void draw_player(ImDrawList* draw, const EspPlayer& player, int index)
{
    if (!passes_filter(player)) return;
    if (!player.visible) {
        draw_offscreen_arrow(draw, player);
        return;
    }
    const PlayerBounds bounds = player_bounds(player);
    const EspSettings& config = settings().esp;
    if (config.boxes) draw_box(draw, player, bounds);
    if (config.names || config.show_distance) draw_label(draw, player, bounds, index);
    if (config.snaplines) draw->AddLine(snapline_origin(player), player.feet, rgba(player_rgba(player)), config.box_thickness);
    draw_target_marker(draw, player);
}

void render_impl()
{
    if (!settings().esp.enabled || !il2cpp::ready()) return;
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    draw_aim_fov(draw);
    if (!esp_players().refresh()) return;
    for (int index = 0; index < esp_players().count(); ++index) draw_player(draw, esp_players().at(index), index);
}
}

void Esp::render()
{
    __try { render_impl(); }
    __except (EXCEPTION_EXECUTE_HANDLER) { esp_players().reset(); }
}

Esp& esp()
{
    static Esp instance;
    return instance;
}
}
