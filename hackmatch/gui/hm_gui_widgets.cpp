#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui.h"

#include "../imgui/imgui_internal.h"
#include "../hm_global_vars.h"

#include "hm_gui.h"
#include "../data/fonts/fa.h"
#include "../hm_helper.h"

// Note: This should be only used for tabs!
bool HMGui::TabButton(const char* label, const char* icon, ImGuiButtonFlags flags, const ImVec2& size_arg,
    const ImU32 color)
{
    ImGuiWindow* wnd = ImGui::GetCurrentWindow();
    if (wnd->SkipItems)
        return false;

    // Define constants
    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = wnd->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

    ImVec2 pos = wnd->DC.CursorPos;
    const ImVec2 size = ImGui::CalcItemSize(size_arg,
        label_size.x + style.FramePadding.x * 2.0f,
        label_size.y + style.FramePadding.y * 2.0f);

    // Align buttons that are smaller/have no padding
    if (flags & ImGuiButtonFlags_AlignTextBaseLine &&
        style.FramePadding.y < wnd->DC.CurrLineTextBaseOffset)
        pos.y += wnd->DC.CurrLineTextBaseOffset - style.FramePadding.y;

    // Add button rect
    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    // Button behaviour
    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render frame
    ImGui::RenderNavHighlight(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, color, false, style.FrameRounding);

    // Define text position
    ImVec2 text_min = bb.Min + style.FramePadding;
    const ImVec2 text_max = bb.Max - style.FramePadding;

    // Render icon
    ImGui::RenderTextClipped(text_min, text_max, icon, nullptr, &label_size, style.ButtonTextAlign, &bb);

    // Render text
    text_min.x += 35.f; // Note: distance between icon and text
    ImGui::RenderTextClipped(text_min, text_max, label, nullptr, &label_size, style.ButtonTextAlign, &bb);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

// Note: Recreated ImGui::InvisibleButton() to work with pos argument
bool HMGui::InvisibleButton(const char* str_id, const ImVec2& size_arg, const ImVec2& pos_arg, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    // Cannot use zero-size for InvisibleButton(). Unlike Button() there is not way to fallback using the label size.
    IM_ASSERT(size_arg.x != 0.0f && size_arg.y != 0.0f);

    const ImGuiID id = window->GetID(str_id);
    const ImVec2 size = ImGui::CalcItemSize(size_arg, 0.0f, 0.0f);
    const ImRect bb{ pos_arg, pos_arg + size };
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

    return pressed;
}

// Base: https://github.com/ocornut/imgui/issues/1537#issuecomment-355569554
bool HMGui::ToggleButton(const char* label, bool* v, bool* vr, int* hotkey, bool* gear, ImVec2* screenPosOut)
{
    // Define constants
    const auto& g = *GImGui;
    const auto p = ImGui::GetCursorScreenPos();
    auto* drawList = ImGui::GetWindowDrawList();
    const auto style = ImGui::GetStyle();

    // Define sizes
    const float height = ImGui::GetFrameHeight() * 0.8f;
    const float width = height * 2.f;
    const float radius = height * 0.5f;
    const float buttDistance = ImGui::GetContentRegionAvail().x - (vars->widgetOffset + width);

    // Render button label
    ImGui::RenderText({ p.x, p.y }, label);
    const auto cp = ImGui::GetCursorPos();

    // Button behaviour
    InvisibleButton(label, ImVec2{ width + style.ItemInnerSpacing.x, height }, ImVec2{ p.x + buttDistance, p.y });
    if (ImGui::IsItemClicked())
        *v = !*v;

    // Show bind
    static auto screenPos = ImVec2{ 0.f, 0.f };
    if ((screenPos.x == 0.f && screenPos.y == 0.f) || (ImGui::IsItemClicked(ImGuiMouseButton_Right) && *v))
        screenPos = ImGui::GetMousePos();
    static bool pressed = false;
    bool niggers = false;
    int niggersKey = 0x0;
    bool* fvr = &niggers;
    int* hotkeyr = &niggersKey;
    if (vr != nullptr)
        fvr = vr;
    if (hotkey != nullptr)
        hotkeyr = hotkey;

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && *v && vr != nullptr)// && !changingBind)
    {
        *fvr = !*fvr;
    }
    if (*fvr && ImGui::IsKeyPressed(ImGuiKey_Escape) && vr != nullptr)
    {
        *fvr = false;
        pressed = false;
        // changingBind = false;
    }

    if (*fvr && vr != nullptr)
    {
        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar;
        const auto bindMenuSize = ImVec2(130.f, 0.f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove free space around the window
        ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::tabsBgColor.imGui());
        ImGui::SetNextWindowPos(screenPos, ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(bindMenuSize);
        ImGui::Begin("#bindMenu", nullptr, flags);

        ImGui::PushStyleColor(ImGuiCol_Button, Colors::BrightXiketic.imGui());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.f));

        // const auto buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 30.f);
        // ImGui::SetCursorPosX(0.f);
        // std::string bindText = "  New Bind";
        // if (hotkey != nullptr)
        // {
        //     if (*hotkey != 0)
        //         bindText = "  Change Bind";
        // }
        // if (IconButton(ICON_FA_CIRCLE_PLUS, bindText.c_str(), buttonSize, ImGuiButtonFlags_None, false))
        //     pressed = !pressed;
        // 
        // ImGui::SetCursorPosX(0.f);
        // if (IconButton(ICON_FA_BARS, "  Hotkeys", buttonSize, ImGuiButtonFlags_None, false))
        // {
        //     pressed = false;
        //     changingBind = false;
        //     manager->config->hotkeysWindow.hotkeysWindow = !manager->config->hotkeysWindow.hotkeysWindow;
        //     manager->config->hotkeysWindow.screenPos = screenPos;
        // }
        // 
        // ImGui::PushStyleColor(ImGuiCol_Text, Colors::RubyRed.imGui());
        // ImGui::SetCursorPosX(0.f);
        // if (IconButton(ICON_FA_TRASH_ARROW_UP, "  Reset", buttonSize, ImGuiButtonFlags_None, false))
        // {
        //     if (hotkey != nullptr)
        //         *hotkey = 0;
        // }
        // ImGui::PopStyleColor();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // if (pressed && hotkey != nullptr)
        // {
        //     const auto bindMenuSelectorSize = ImVec2(125.f, 0.f);
        //     ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove free space around the window
        //     ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::tabsBgColor.imGui());
        //     ImGui::SetNextWindowPos(ImVec2(screenPos.x + bindMenuSize.x + 2.f, screenPos.y), ImGuiCond_Appearing); // 2.f is arbitrary
        //     ImGui::SetNextWindowSize(bindMenuSelectorSize);
        //     ImGui::Begin("#bindMenuSelector", nullptr, flags);
        // 
        //     ImGui::Indent(vars->childWindowOffset);
        // 
        //     ImGui::Spacing();
        // 
        //     ImGui::AlignTextToFramePadding();
        //     ImGui::Text("Key");
        // 
        //     const auto hotkeySize = ImVec2(40.f, 25.f);
        //     Hotkey(*hotkeyr, bindMenuSelectorSize.x - hotkeySize.x - vars->childWindowOffset, hotkeySize);
        // 
        //     ImGui::Unindent();
        //     ImGui::Spacing();
        //     ImGui::End();
        //     ImGui::PopStyleVar();
        //     ImGui::PopStyleColor();
        // }
        // 
        // changingBind = true;
    }

    // Button animation
    float t = *v ? 1.f : 0.f;
    if (g.LastActiveId == g.CurrentWindow->GetID(label))
    {
        const float tAnim = ImSaturate(g.LastActiveIdTimer / vars->animSpeed);
        t = *v ? tAnim : 1.f - tAnim;
    }

    // Define colors
    ImU32 colBg = *v ? Colors::toggleButtonActiveColor.imGui() : Colors::toggleButtonBgColor.imGui();
    ImU32 colCircle = *v ? Colors::toggleButtonCircleColor.imGui() : Colors::SlateGray.imGui();
    if (ImGui::IsItemHovered() || ImGui::IsItemActive() && !ImGui::IsItemClicked())
        colCircle = Colors::toggleButtonCircleHoveredColor.imGui();
    // ^ Changes circle color to create a visible interaction

    if (gear != nullptr)
    {
        std::string gearText = ICON_FA_GEAR;
        const auto gearTextSize = ImGui::CalcTextSize(gearText.c_str());
        const ImU32 color = *gear ? Colors::UltramarineBlue.imGui() : Colors::White.imGui();
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::SetCursorPosX(buttDistance - gearTextSize.x - 10.f); // 10.f is arbitrary
        ImGui::SetCursorPosY(cp.y - gearTextSize.y * 0.15f);        // what the fuck is 0.15f?
        std::string gearButtonId = "##gearButton";
        gearButtonId += label;
        if (hmHelper->m_pGui->TabButton(gearButtonId.c_str(), gearText.c_str(), ImGuiButtonFlags_None, ImVec2(25.f, 25.f), Colors::tabsBgColor.imGui()) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            *gear = !*gear;
            if (*gear)
            {
                *screenPosOut = ImGui::GetMousePos();
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            *gear = false;
            *screenPosOut = ImVec2(0, 0);
        }


        ImGui::PopStyleColor();
    }

    drawList->AddRectFilled(ImVec2(p.x + buttDistance, p.y),
        ImVec2{ p.x + buttDistance + width, p.y + height },
        colBg,
        height * 0.5f);
    drawList->AddCircleFilled(ImVec2{ p.x + buttDistance + radius + t * (width - radius * 2.0f), p.y + radius },
        radius - 1.5f,
        colCircle);

    // Have spacing between widgets if `useSpacing` == true
    ImGui::Spacing();
    return *v;
}

bool HMGui::IconButton(const char* icon, const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, bool useSpacing)
{

    ImGuiWindow* wnd = ImGui::GetCurrentWindow();
    if (wnd->SkipItems)
        return false;

    // Define constants
    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = wnd->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

    ImVec2 pos = wnd->DC.CursorPos;
    const ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    // Align buttons that are smaller/have no padding
    if (flags & ImGuiButtonFlags_AlignTextBaseLine && style.FramePadding.y < wnd->DC.CurrLineTextBaseOffset)
        pos.y += wnd->DC.CurrLineTextBaseOffset - style.FramePadding.y;

    // Add button rect
    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    // Button behaviour
    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);
    const bool isMouseDown = g.IO.MouseDown[ImGuiMouseButton_Left];
    if (hovered && isMouseDown)
        held = true;
    // Render
    const ImU32 col = ImGui::GetColorU32(held && hovered ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);

    // Render frame
    ImGui::RenderNavHighlight(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    // Define text position
    ImVec2 text_min = bb.Min + style.FramePadding * 2;
    const ImVec2 text_max = bb.Max - style.FramePadding * 2;

    // Render icon
    ImGui::RenderTextClipped(text_min, text_max, icon, nullptr, &label_size, style.ButtonTextAlign, &bb);

    // Render text
    text_min.x += 20.f; // Note: distance between icon and text
    ImGui::RenderTextClipped(text_min, text_max, label, nullptr, &label_size, style.ButtonTextAlign, &bb);

    if (useSpacing)
        ImGui::Spacing();
        //Spacing(1, ImVec2(0.f, vars->widgetSpacing));

    return pressed;
}

// Note: Recreated ImGui::PatchFormatStringFloatToInt() to be able to make a slider
static const char* PatchFormatStringFloatToInt(const char* fmt)
{
    if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' &&
        fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
        return "%d";
    const char* fmt_start = ImParseFormatFindStart(fmt); // Find % (if any, and ignore %%)
    const char* fmt_end =
        ImParseFormatFindEnd(fmt_start); // Find end of format specifier, which itself is an exercise of
    // confidence/recklessness (because snprintf is dependent on libc or user).
    if (fmt_end > fmt_start && fmt_end[-1] == 'f')
    {
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        if (fmt_start == fmt && fmt_end[0] == 0)
            return "%d";
        ImGuiContext& g = *GImGui;
        ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
        return g.TempBuffer;
#else
        IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please
        // replace with e.g. "%d"
#endif
    }
    return fmt;
}

void HMGui::WCombo(const char* label, int* currentItem, const char* const items[], int size, float width)
{
    if (width == 0.f)
        width = ImGui::GetContentRegionAvail().x * 0.5f; // default size
    const float comboDistance = ImGui::GetContentRegionAvail().x - (2 * vars->widgetOffset + width + ImGui::CalcTextSize(label).x);
    std::string strId = "##";
    strId += label; // Cat 'em

    ImGui::Text(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + comboDistance);
    ImGui::PushItemWidth(width);
    ImGui::Combo(strId.c_str(), currentItem, items, size);
    ImGui::PopItemWidth();

    const float comboOffset = abs(vars->widgetSpacing - IM_FLOOR(ImGui::GetStyle().ItemSpacing.y * 0.5f));
    ImGui::Spacing();
}

void HMGui::MultiCombo(const char* label, const std::vector<const char*>& titles, const std::vector<bool*>& options, float width)
{
    if (titles.size() != options.size())
    {
        // Shouldn't, might cause issues
        return;
    }

    if (width == 0.f)
        width = ImGui::GetContentRegionAvail().x * 0.5f;
    const float comboDistance = ImGui::GetContentRegionAvail().x - (2 * vars->widgetOffset + width + ImGui::CalcTextSize(label).x);

    // Calculate ID
    std::string strId = "##";
    strId += label;

    std::string preview = "Disabled##";
    for (size_t i = 0; i < options.size(); i++)
    {
        if (*options[i])
        {
            if (preview == "Disabled##")
                preview = "";

            preview += titles[i];
            preview.append(", ");
        }
    }

    // Remove last ,
    preview.pop_back();
    preview.pop_back();

    ImGui::Text(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + comboDistance);
    ImGui::PushItemWidth(width);
    if (ImGui::BeginCombo(strId.c_str(), preview.c_str()))
    {
        for (size_t i = 0; i < titles.size(); i++)
        {
            std::string pTitle = "+ ";
            pTitle += titles[i];
            ImGui::Selectable(*options[i] ? pTitle.c_str() : titles[i], options[i], ImGuiSelectableFlags_DontClosePopups);
        }

        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    const float comboOffset = abs(vars->widgetSpacing - IM_FLOOR(ImGui::GetStyle().ItemSpacing.y * 0.5f));
    ImGui::Spacing();
}

void HMGui::MultiCombo(const char* label, const std::vector<const char*>& titles, const std::vector<int>& values, int* flag, float width)
{
    if (titles.size() != values.size())
    {
        // Shouldn't, might cause issues
        return;
    }

    if (width == 0.f)
        width = ImGui::GetContentRegionAvail().x * 0.5f;
    const float comboDistance = ImGui::GetContentRegionAvail().x - (2 * vars->widgetOffset + width + ImGui::CalcTextSize(label).x);

    // Calculate ID
    std::string strId = "##";
    strId += label;

    std::string preview = "None##";
    if (!*flag)
        preview = "None##";
    else
    {
        for (size_t i = 0; i < values.size(); i++)
        {
            if (*flag & values[i])
            {
                if (preview == "None##")
                    preview = "";

                preview += titles[i];
                preview.append(", ");
            }
        }

        // Remove last ,
        preview.pop_back();
        preview.pop_back();
    }

    ImGui::Text(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + comboDistance);
    ImGui::PushItemWidth(width);
    if (ImGui::BeginCombo(strId.c_str(), preview.c_str()))
    {
        for (size_t i = 0; i < titles.size(); i++)
        {
            std::string pTitle = "+ ";
            pTitle += titles[i];
            const bool hasFlag = *flag & values[i];
            if (ImGui::Selectable(hasFlag ? pTitle.c_str() : titles[i], hasFlag, ImGuiSelectableFlags_DontClosePopups))
            {
                if (hasFlag)
                    *flag &= ~values[i];
                else
                    *flag |= values[i];
            }
        }

        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    const float comboOffset = abs(vars->widgetSpacing - IM_FLOOR(ImGui::GetStyle().ItemSpacing.y * 0.5f));
    ImGui::Spacing();
}

void HMGui::ColorPickerWithText(const char* name, ColorRGBA& colRGBA) noexcept
{
    ImGui::PushID(name);
    ColorRGBA colorIn = colRGBA;
    if (colRGBA.r > 1.f || colRGBA.g > 1.f || colRGBA.b > 1.f || colRGBA.a > 1.f)
        colorIn = colorIn.without255();
    float col[4] = { colorIn.r, colorIn.g, colorIn.b, colorIn.a };
    ImGui::Text(name);
    const float buttDistance = ImGui::GetContentRegionAvail().x - (2 * vars->widgetOffset + ImGui::GetFrameHeight() + ImGui::CalcTextSize(name).x);

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttDistance);

    const bool openPopup =
        ImGui::ColorButton("##btn",
            ImVec4{ col[0], col[1], col[2], col[3] },
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview);
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
        {
            std::copy(static_cast<float*>(payload->Data), static_cast<float*>(payload->Data) + 3, col);
            col[3] = 1.f;
        }
        if (const auto payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
            std::copy(static_cast<float*>(payload->Data), static_cast<float*>(payload->Data) + 4, col);
        ImGui::EndDragDropTarget();
    }

    if (openPopup)
        ImGui::OpenPopup("##popup");

    if (ImGui::BeginPopup("##popup"))
    {
        ImGui::PushItemWidth(200.f);
        ImGui::ColorPicker4("##picker",
            col,
            ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaPreview |
            ImGuiColorEditFlags_AlphaBar);
        ImGui::PopItemWidth();
        colRGBA.r = col[0];
        colRGBA.g = col[1];
        colRGBA.b = col[2];
        colRGBA.a = col[3];
        colRGBA = colRGBA.with255();
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

void HMGui::ColorPicker(const char* id, ColorRGBA& colRGBA) noexcept
{
    ImGui::PushID(id);
    ColorRGBA colorIn = colRGBA;
    if (colRGBA.r > 1.f || colRGBA.g > 1.f || colRGBA.b > 1.f || colRGBA.a > 1.f)
        colorIn = colorIn.without255();
    float col[4] = { colorIn.r, colorIn.g, colorIn.b, colorIn.a };
    const bool openPopup =
        ImGui::ColorButton("##btn", ImVec4{ col[0], col[1], col[2], col[3] }, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview);
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
        {
            std::copy(static_cast<float*>(payload->Data), static_cast<float*>(payload->Data) + 3, col);
            col[3] = 1.f;
        }
        if (const auto payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
            std::copy(static_cast<float*>(payload->Data), static_cast<float*>(payload->Data) + 4, col);
        ImGui::EndDragDropTarget();
    }

    if (openPopup)
        ImGui::OpenPopup("##popup");

    if (ImGui::BeginPopup("##popup"))
    {
        ImGui::PushItemWidth(200.f);
        ImGui::ColorPicker4("##picker",
            col,
            ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaPreview |
            ImGuiColorEditFlags_AlphaBar);
        ImGui::PopItemWidth();
        colRGBA.r = col[0];
        colRGBA.g = col[1];
        colRGBA.b = col[2];
        colRGBA.a = col[3];
        colRGBA = colRGBA.with255();
        ImGui::EndPopup();
    }
    ImGui::PopID();
}
