#include "widgets.h"

#include "theme.h"

#include <algorithm>

namespace hackmatch::ui {
namespace {
ImFont* widget_strong_font = nullptr;
bool widget_motion_enabled = true;

ImVec4 mix(const ImVec4& from, const ImVec4& to, float amount)
{
    return {from.x + (to.x - from.x) * amount, from.y + (to.y - from.y) * amount,
        from.z + (to.z - from.z) * amount, from.w + (to.w - from.w) * amount};
}

ImU32 packed(ImVec4 color)
{
    color.w *= ImGui::GetStyle().Alpha;
    return ImGui::ColorConvertFloat4ToU32(color);
}

void draw_icon(ImDrawList* draw, NavigationIcon icon, ImVec2 center, ImU32 color)
{
    constexpr float thickness = 1.7f;
    switch (icon) {
    case NavigationIcon::Aim:
        draw->AddCircle(center, 6.0f, color, 20, thickness);
        draw->AddLine({center.x - 10.0f, center.y}, {center.x - 4.0f, center.y}, color, thickness);
        draw->AddLine({center.x + 4.0f, center.y}, {center.x + 10.0f, center.y}, color, thickness);
        draw->AddLine({center.x, center.y - 10.0f}, {center.x, center.y - 4.0f}, color, thickness);
        draw->AddLine({center.x, center.y + 4.0f}, {center.x, center.y + 10.0f}, color, thickness);
        draw->AddCircleFilled(center, 1.6f, color);
        break;
    case NavigationIcon::Visuals:
        draw->AddBezierCubic({center.x - 10.0f, center.y}, {center.x - 4.0f, center.y - 8.0f},
            {center.x + 4.0f, center.y - 8.0f}, {center.x + 10.0f, center.y}, color, thickness);
        draw->AddBezierCubic({center.x - 10.0f, center.y}, {center.x - 4.0f, center.y + 8.0f},
            {center.x + 4.0f, center.y + 8.0f}, {center.x + 10.0f, center.y}, color, thickness);
        draw->AddCircle(center, 3.0f, color, 14, thickness);
        break;
    case NavigationIcon::Weapons: {
        const ImVec2 points[]{{center.x - 10.0f, center.y - 3.0f}, {center.x + 6.0f, center.y - 3.0f},
            {center.x + 10.0f, center.y}, {center.x + 4.0f, center.y + 2.0f}, {center.x, center.y + 8.0f},
            {center.x - 4.0f, center.y + 8.0f}, {center.x - 2.0f, center.y + 2.0f}, {center.x - 10.0f, center.y + 2.0f}};
        draw->AddPolyline(points, static_cast<int>(std::size(points)), color, ImDrawFlags_Closed, thickness);
        break;
    }
    case NavigationIcon::Player:
        draw->AddCircle({center.x, center.y - 5.0f}, 3.5f, color, 16, thickness);
        draw->AddBezierCubic({center.x - 8.0f, center.y + 9.0f}, {center.x - 7.0f, center.y + 2.0f},
            {center.x + 7.0f, center.y + 2.0f}, {center.x + 8.0f, center.y + 9.0f}, color, thickness);
        break;
    case NavigationIcon::Config:
        for (int row = -1; row <= 1; ++row) {
            const float y = center.y + static_cast<float>(row) * 6.0f;
            const float knob = center.x + (row == -1 ? -3.0f : row == 0 ? 4.0f : -1.0f);
            draw->AddLine({center.x - 9.0f, y}, {center.x + 9.0f, y}, color, thickness);
            draw->AddCircleFilled({knob, y}, 2.3f, color);
        }
        break;
    }
}
}

void configure_widgets(ImFont* strong_font, bool motion_enabled)
{
    widget_strong_font = strong_font;
    widget_motion_enabled = motion_enabled;
}

bool navigation_item(const char* label, NavigationIcon icon, float selection)
{
    const ThemePalette& color = active_palette();
    selection = std::clamp(selection, 0.0f, 1.0f);
    ImGui::PushID(label);
    const ImVec2 size{ImGui::GetContentRegionAvail().x, 48.0f};
    const bool clicked = ImGui::InvisibleButton("nav", size);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    if (selection > 0.001f || hovered) {
        ImVec4 fill = selection > 0.001f ? color.surface_elevated : color.card_hover;
        fill.w *= selection > 0.001f ? selection : 1.0f;
        draw->AddRectFilled({minimum.x + 11.0f, minimum.y}, maximum, packed(fill), 7.0f);
    }

    const ImVec4 icon_color = mix(color.muted, color.accent, selection);
    draw_icon(draw, icon, {minimum.x + 30.0f, minimum.y + 24.0f}, packed(icon_color));
    const ImVec4 label_color = mix(color.text, color.accent, selection);
    if (widget_strong_font) {
        draw->AddText(widget_strong_font, widget_strong_font->FontSize, {minimum.x + 52.0f, minimum.y + 14.0f}, packed(label_color), label);
    } else {
        draw->AddText({minimum.x + 52.0f, minimum.y + 14.0f}, packed(label_color), label);
    }
    if (ImGui::IsItemFocused()) {
        draw->AddRect({minimum.x + 11.0f, minimum.y + 2.0f}, {maximum.x - 2.0f, maximum.y - 2.0f},
            packed(color.accent), 6.0f, 0, 2.0f);
    }
    ImGui::PopID();
    return clicked;
}

bool toggle(const char* label, const char* description, bool& value)
{
    const ThemePalette& color = active_palette();
    ImGui::PushID(label);
    const float row_height = description && description[0] ? 56.0f : 44.0f;
    const ImVec2 size{ImGui::GetContentRegionAvail().x, row_height};
    const bool clicked = ImGui::InvisibleButton("toggle", size);
    if (clicked) value = !value;

    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID animation_id = ImGui::GetID("animation");
    float animation = storage->GetFloat(animation_id, value ? 1.0f : 0.0f);
    const float target = value ? 1.0f : 0.0f;
    animation = widget_motion_enabled ? animation + (target - animation) * std::min(1.0f, ImGui::GetIO().DeltaTime * 16.0f) : target;
    storage->SetFloat(animation_id, animation);

    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    constexpr float track_width = 44.0f;
    constexpr float track_height = 24.0f;
    const ImVec2 track_min{maximum.x - track_width, minimum.y + (row_height - track_height) * 0.5f};
    const ImVec2 track_max{maximum.x, track_min.y + track_height};
    if (ImGui::IsItemHovered()) {
        draw->AddRectFilled({minimum.x - 8.0f, minimum.y}, {maximum.x + 8.0f, maximum.y}, packed(color.card_hover), 5.0f);
    }
    const ImU32 text_color = packed(color.text);
    if (widget_strong_font) draw->AddText(widget_strong_font, widget_strong_font->FontSize, {minimum.x, minimum.y + 5.0f}, text_color, label);
    else draw->AddText({minimum.x, minimum.y + 5.0f}, text_color, label);
    if (description && description[0]) {
        const ImVec4 clip{minimum.x, minimum.y, track_min.x - 16.0f, maximum.y};
        draw->AddText(nullptr, 0.0f, {minimum.x, minimum.y + 29.0f}, packed(color.muted), description, nullptr, 0.0f, &clip);
    }
    ImVec4 off = color.border_strong;
    off.w = 1.0f;
    draw->AddRectFilled(track_min, track_max, packed(mix(off, color.accent_surface, animation)), track_height * 0.5f);
    const float knob_x = track_min.x + 12.0f + animation * (track_width - 24.0f);
    draw->AddCircleFilled({knob_x, track_min.y + 12.0f}, 8.5f, text_color);
    if (ImGui::IsItemFocused()) {
        draw->AddRect({track_min.x - 4.0f, track_min.y - 4.0f}, {track_max.x + 4.0f, track_max.y + 4.0f}, packed(color.accent), 16.0f, 0, 2.0f);
    }
    ImGui::PopID();
    return clicked;
}

void card_begin(const char* id, const char* title, const char* description)
{
    const ThemePalette& color = active_palette();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20.0f, 18.0f});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, color.card);
    ImGui::PushStyleColor(ImGuiCol_Border, color.border);
    ImGui::BeginChild(id, {0.0f, 0.0f}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor(2);
    if (widget_strong_font) ImGui::PushFont(widget_strong_font);
    ImGui::TextUnformatted(title);
    if (widget_strong_font) ImGui::PopFont();
    if (description && description[0]) {
        ImGui::PushStyleColor(ImGuiCol_Text, color.muted);
        ImGui::TextWrapped("%s", description);
        ImGui::PopStyleColor();
    }
    ImGui::Dummy({0.0f, 6.0f});
    ImGui::Separator();
    ImGui::Dummy({0.0f, 4.0f});
}

void card_end()
{
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void setting_slider(const char* label, float& value, float minimum, float maximum, const char* format)
{
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderFloat("##value", &value, minimum, maximum, format);
    ImGui::PopID();
}

void setting_slider(const char* label, int& value, int minimum, int maximum, const char* format)
{
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderInt("##value", &value, minimum, maximum, format);
    ImGui::PopID();
}

void color_setting(const char* label, ColorSetting& value, const Rgba& inherited)
{
    ImGui::PushID(label);
    ImGui::Checkbox("##custom", &value.custom);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    Rgba displayed = value.custom ? value.value : inherited;
    float channels[4]{displayed.r, displayed.g, displayed.b, displayed.a};
    ImGui::BeginDisabled(!value.custom);
    if (ImGui::ColorEdit4("##rgba", channels, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreviewHalf)) {
        value.value = {channels[0], channels[1], channels[2], channels[3]};
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("RGBA %.2f, %.2f, %.2f, %.2f%s", displayed.r, displayed.g, displayed.b, displayed.a,
        value.custom ? "" : " (theme default)");
    ImGui::PopID();
}
}
