#include "hooks.h"

#include "build_info.h"
#include "game.h"
#include "game_offsets.h"
#include "gameplay.h"
#include "il2cpp_api.h"
#include "logger.h"
#include "menu.h"
#include "settings.h"

#include <MinHook.h>
#include <imgui_impl_win32.h>

#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

namespace hackmatch
{
namespace
{
// IDXGISwapChain vtable indices for DirectX 11.
constexpr int present_vtable_index = 8;
constexpr int resize_buffers_vtable_index = 13;
thread_local int local_shot_depth = 0;
thread_local bool selecting_aim_target = false;

bool aim_cast_active()
{
    return !selecting_aim_target && local_shot_depth > 0;
}

bool redirect_shot(Vector3 origin, Vector3& direction, float max_distance)
{
    if (!aim_cast_active())
        return false;
    struct SelectionScope
    {
        SelectionScope()
        {
            selecting_aim_target = true;
        }
        ~SelectionScope()
        {
            selecting_aim_target = false;
        }
    } scope;
    return gameplay().redirect_shot(origin, direction, max_distance);
}

void handle_raw_mouse(HRAWINPUT input)
{
    UINT size = sizeof(RAWINPUT);
    RAWINPUT raw{};
    if (GetRawInputData(input, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == size &&
        raw.header.dwType == RIM_TYPEMOUSE)
    {
        menu().add_mouse_delta(static_cast<float>(raw.data.mouse.lLastX), static_cast<float>(raw.data.mouse.lLastY));
    }
}

int released_key(UINT message, WPARAM wparam)
{
    if (message == WM_KEYUP || message == WM_SYSKEYUP)
        return static_cast<int>(wparam);
    if (message == WM_LBUTTONUP)
        return VK_LBUTTON;
    if (message == WM_RBUTTONUP)
        return VK_RBUTTON;
    if (message == WM_MBUTTONUP)
        return VK_MBUTTON;
    if (message == WM_XBUTTONUP)
        return GET_XBUTTON_WPARAM(wparam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2;
    return 0;
}

bool input_message(UINT message)
{
    return message == WM_INPUT || (message >= WM_KEYFIRST && message <= WM_KEYLAST) ||
           (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST);
}

} // namespace

Hooks::CallbackGuard::CallbackGuard(Hooks& owner) : owner_(owner)
{
    std::scoped_lock lock(owner_.callback_mutex_);
    ++owner_.active_callbacks_;
}

Hooks::CallbackGuard::~CallbackGuard()
{
    std::scoped_lock lock(owner_.callback_mutex_);
    if (--owner_.active_callbacks_ == 0)
        owner_.callback_idle_.notify_all();
}

DWORD WINAPI Hooks::unload_thread(void*)
{
    HMODULE module = hooks().module_;
    hooks().shutdown();
    logger::shutdown_console();
    FreeLibraryAndExitThread(module, 0);
}

void Hooks::request_unload()
{
    if (!unload_.exchange(true, std::memory_order_acq_rel))
    {
        logger::info("Unload requested");
        if (HANDLE thread = CreateThread(nullptr, 0, unload_thread, nullptr, 0, nullptr))
        {
            CloseHandle(thread);
        }
        else
        {
            unload_ = false;
            logger::error("Could not create the unload thread");
        }
    }
}

LRESULT CALLBACK Hooks::wnd_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    Hooks& instance = hooks();
    CallbackGuard guard(instance);
    const int key = released_key(message, wparam);
    const InterfaceSettings& controls = settings().controls;
    if (!menu().capturing_hotkey() && key == controls.menu_hotkey)
    {
        menu().toggle();
        return 0;
    }

    if (!menu().capturing_hotkey() && key == controls.unload_hotkey)
    {
        instance.request_unload();
        return 0;
    }

    if (!menu().capturing_hotkey() && !menu().visible() && key != 0)
    {
        if (key == controls.aim_toggle_hotkey)
            settings().aim.enabled = !settings().aim.enabled;
        if (key == controls.esp_toggle_hotkey)
            settings().esp.enabled = !settings().esp.enabled;
    }

    if (menu().visible())
    {
        if (message == WM_INPUT)
        {
            handle_raw_mouse(reinterpret_cast<HRAWINPUT>(lparam));
        }
        ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam);
        if (input_message(message))
            return 0;
    }

    return CallWindowProcW(instance.wnd_proc_, window, message, wparam, lparam);
}

void Hooks::fire_primary_shot_hook(il2cpp::Object* player, il2cpp::Object* shot, const MethodInfo* method)
{
    Hooks& instance = hooks();
    CallbackGuard guard(instance);
    instance.shot(instance.fire_primary_shot_, player, shot, method);
}

void Hooks::shot(ShotFn original, il2cpp::Object* player, il2cpp::Object* shot, const MethodInfo* method)
{
    static il2cpp::FieldInfo* local_instance = il2cpp::field(game().player_controller(), "LocalInstance");
    il2cpp::Object* local = nullptr;
    const bool is_local =
        gameplay().aiming() && local_instance && il2cpp::read_static_field(local_instance, &local) && player == local;
    struct ShotScope
    {
        explicit ShotScope(bool active) : active(active)
        {
            local_shot_depth += active ? 1 : 0;
        }
        ~ShotScope()
        {
            local_shot_depth -= active ? 1 : 0;
        }
        bool active;
    } scope(is_local);
    original(player, shot, method);
}

il2cpp::Array* Hooks::raycast_all_hook(Vector3 origin, Vector3 direction, float max_distance, int layer_mask,
                                       int query_trigger_interaction, const MethodInfo* method)
{
    Hooks& instance = hooks();
    CallbackGuard guard(instance);
    return instance.raycast_all(origin, direction, max_distance, layer_mask, query_trigger_interaction, method);
}

il2cpp::Array* Hooks::raycast_all(Vector3 origin, Vector3 direction, float max_distance, int layer_mask,
                                  int query_trigger_interaction, const MethodInfo* method)
{
    Vector3 redirected = direction;
    if (redirect_shot(origin, redirected, max_distance) &&
        clear_shot(origin, redirected, max_distance, layer_mask, query_trigger_interaction, method))
    {
        direction = redirected;
    }
    return raycast_all_(origin, direction, max_distance, layer_mask, query_trigger_interaction, method);
}

bool Hooks::clear_shot(Vector3 origin, Vector3 direction, float max_distance, int layer_mask,
                       int query_trigger_interaction, const MethodInfo* method)
{
    return raycast_(origin, direction, max_distance, layer_mask, query_trigger_interaction, method);
}

void Hooks::player_update_hook(il2cpp::Object* player, const MethodInfo* method)
{
    Hooks& instance = hooks();
    CallbackGuard guard(instance);
    instance.player_update(player, method);
}

void Hooks::player_update(il2cpp::Object* player, const MethodInfo* method)
{
    std::scoped_lock lock(player_update_mutex_);
    static il2cpp::FieldInfo* local_instance = il2cpp::field(game().player_controller(), "LocalInstance");
    il2cpp::Object* local = nullptr;
    const bool auto_sprint = build_compatibility().status == BuildStatus::Compatible &&
                             settings().movement.auto_sprint && local_instance &&
                             il2cpp::read_static_field(local_instance, &local) && player == local;
    if (auto_sprint)
    {
        *reinterpret_cast<bool*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_sprinting) = true;
    }
    gameplay().before_player_update(player);
    player_update_(player, method);
    local = nullptr;
    if (auto_sprint && il2cpp::read_static_field(local_instance, &local) && player == local)
    {
        *reinterpret_cast<bool*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_sprinting) = true;
    }
    gameplay().after_player_update(player);
}

bool Hooks::init(HMODULE module)
{
    module_ = module;
    unload_ = false;
    shutting_down_ = false;
    menu_failure_logged_ = false;

    void** table = nullptr;
    if (!get_swapchain_vtable(table))
    {
        logger::error("DirectX 11 swap-chain discovery failed");
        return false;
    }
    logger::success("DirectX 11 swap-chain methods discovered");

    const MH_STATUS initialize_status = MH_Initialize();
    if (initialize_status != MH_OK)
    {
        logger::error(std::string("MinHook initialization failed: ") + MH_StatusToString(initialize_status));
        return false;
    }
    minhook_ready_ = true;

    HMODULE game_assembly = GetModuleHandleA("GameAssembly.dll");
    const auto game_base = reinterpret_cast<std::uintptr_t>(game_assembly);
    void* fire_primary_shot =
        game_assembly ? reinterpret_cast<void*>(game_base + game_offsets::methods::fire_primary_shot) : nullptr;
    void* raycast =
        game_assembly ? reinterpret_cast<void*>(game_base + game_offsets::methods::physics_raycast) : nullptr;
    void* raycast_all =
        game_assembly ? reinterpret_cast<void*>(game_base + game_offsets::methods::physics_raycast_all) : nullptr;
    void* player_update =
        game().player_update() ? reinterpret_cast<void*>(game().player_update()->methodPointer) : nullptr;
    const bool compatible_build = build_compatibility().status == BuildStatus::Compatible;
    raycast_ = compatible_build ? reinterpret_cast<RaycastFn>(raycast) : nullptr;
    const bool aim_hooks_ready =
        !compatible_build ||
        (install_hook("FirePrimaryShot", fire_primary_shot, reinterpret_cast<void*>(fire_primary_shot_hook),
                      reinterpret_cast<void**>(&fire_primary_shot_)) &&
         install_hook("Physics.RaycastAll", raycast_all, reinterpret_cast<void*>(raycast_all_hook),
                      reinterpret_cast<void**>(&raycast_all_)));
    if (!compatible_build)
    {
        logger::warning("Raw aim hooks disabled because the installed game build is not verified");
    }
    if (!aim_hooks_ready ||
        !install_hook("PlayerController.Update", player_update, reinterpret_cast<void*>(player_update_hook),
                      reinterpret_cast<void**>(&player_update_)) ||
        !install_hook("DXGI Present", table[present_vtable_index], reinterpret_cast<void*>(present_hook),
                      reinterpret_cast<void**>(&present_)) ||
        !install_hook("DXGI ResizeBuffers", table[resize_buffers_vtable_index],
                      reinterpret_cast<void*>(resize_buffers_hook), reinterpret_cast<void**>(&resize_buffers_)))
    {
        shutdown();
        return false;
    }

    const MH_STATUS enable_status = MH_EnableHook(MH_ALL_HOOKS);
    if (enable_status != MH_OK)
    {
        logger::error(std::string("Enabling hooks failed: ") + MH_StatusToString(enable_status));
        shutdown();
        return false;
    }
    logger::success("All hooks enabled");

    return true;
}

bool Hooks::install_hook(const char* name, void* target, void* detour, void** original)
{
    if (!target)
    {
        logger::error(std::string(name) + " hook target is unavailable");
        return false;
    }

    const MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status != MH_OK)
    {
        logger::error(std::string(name) + " hook installation failed: " + MH_StatusToString(status));
        return false;
    }
    logger::success(std::string(name) + " hook installed");
    return true;
}

void Hooks::shutdown()
{
    if (shutting_down_.exchange(true, std::memory_order_acq_rel))
    {
        return;
    }
    unload_.store(true, std::memory_order_release);
    logger::info("Shutting down runtime hooks");

    if (minhook_ready_)
    {
        MH_DisableHook(MH_ALL_HOOKS);
    }

    {
        std::scoped_lock render_lock(render_mutex_);
        if (wnd_proc_ && window_)
            SetWindowLongPtrW(window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wnd_proc_));
    }

    {
        std::unique_lock lock(callback_mutex_);
        callback_idle_.wait(lock, [this] { return active_callbacks_ == 0; });
        lock.unlock();
        Sleep(50);
        lock.lock();
        callback_idle_.wait(lock, [this] { return active_callbacks_ == 0; });
    }

    std::scoped_lock runtime_lock(player_update_mutex_, render_mutex_);

    wnd_proc_ = nullptr;
    if (window_)
    {
        SetWindowDisplayAffinity(window_, WDA_NONE);
        display_affinity_initialized_ = false;
        display_affinity_failure_logged_ = false;
        streamproof_applied_ = false;
    }

    logger::info("Restoring modified game state");
    gameplay().restore();
    logger::success("Game state restored");

    menu().shutdown();
    release_render_target();
    release_device();

    if (minhook_ready_)
    {
        MH_Uninitialize();
        minhook_ready_ = false;
    }
    menu_ready_ = false;
    logger::success("Hackmatch shutdown complete");
}

Hooks& hooks()
{
    static Hooks instance;
    return instance;
}
} // namespace hackmatch
