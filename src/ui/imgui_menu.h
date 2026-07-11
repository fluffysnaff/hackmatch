#pragma once

#include "config_manager.h"
#include "logo.h"

#include <windows.h>

#include <string>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ImFont;

namespace hackmatch {
class ImGuiMenu {
public:
    bool init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context);
    void render();
    void shutdown();
    void toggle();
    bool visible() const;
    bool capturing_hotkey() const;
    void add_mouse_delta(float x, float y);
#ifdef HACKMATCH_UI_PREVIEW
    void select_page_for_preview(int page);
    void set_preview_state(int theme, int page, float navigation_progress, float content_progress, bool reduced_motion);
#endif

private:
    enum class CursorMode {
        Inactive,
        Native,
        Virtual,
    };

    void render_options();
    void render_config_page(AppSettings& config);
    void start_page_transition(int page);
    void update_animations();
    void request_profile_action(int action, const std::string& profile);
    void perform_profile_action(int action, const std::string& profile);
    void update_cursor();

    bool ready_ = false;
    bool visible_ = false;
    bool virtual_mouse_ready_ = false;
    bool last_mouse_valid_ = false;
    float virtual_mouse_x_ = 0.0f;
    float virtual_mouse_y_ = 0.0f;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    int page_ = 0;
    int previous_page_ = 0;
    bool motion_enabled_ = true;
    float navigation_mix_[5]{1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float navigation_start_mix_[5]{1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float navigation_elapsed_ = 1.0f;
    float indicator_position_ = 0.0f;
    float indicator_start_ = 0.0f;
    float content_alpha_ = 1.0f;
    float content_offset_ = 0.0f;
    float content_start_alpha_ = 1.0f;
    float content_start_offset_ = 0.0f;
    float content_elapsed_ = 1.0f;
    int pending_profile_action_ = 0;
    std::string pending_profile_;
    std::string profile_status_;
    char profile_name_[49]{};
    ConfigValidation import_preview_{};
    bool config_ready_ = false;
#ifdef HACKMATCH_UI_PREVIEW
    bool preview_animation_override_ = false;
    float preview_navigation_progress_ = 1.0f;
    float preview_content_progress_ = 1.0f;
#endif
    HWND window_ = nullptr;
    CursorMode cursor_mode_ = CursorMode::Inactive;
    ImFont* body_font_ = nullptr;
    ImFont* strong_font_ = nullptr;
    ImFont* title_font_ = nullptr;
};

ImGuiMenu& imgui_menu();
}
