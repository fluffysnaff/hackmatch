#include "build_info.h"
#include "game.h"
#include "game_offsets.h"
#include "hooks.h"
#include "il2cpp_api.h"
#include "logger.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>

namespace {
bool initialize_runtime()
{
    hackmatch::logger::info("Waiting for GameAssembly.dll");
    HMODULE game_assembly = nullptr;
    for (int attempt = 0; attempt < 600 && !game_assembly; ++attempt) {
        game_assembly = GetModuleHandleW(L"GameAssembly.dll");
        if (!game_assembly) {
            Sleep(100);
        }
    }
    if (!game_assembly) {
        hackmatch::logger::error("GameAssembly.dll was not detected before the startup timeout");
        return false;
    }
    hackmatch::logger::success("GameAssembly.dll detected");

    if (!hackmatch::il2cpp::init()) {
        hackmatch::logger::error(hackmatch::il2cpp::last_error());
        return false;
    }
    hackmatch::logger::success("IL2CPP exports resolved");

    if (!hackmatch::game().init()) {
        hackmatch::logger::error(hackmatch::game().last_error());
        return false;
    }
    hackmatch::logger::success("Game metadata bindings resolved");
    return true;
}

DWORD WINAPI init_thread(void* module)
{
    if (hackmatch::logger::initialize_console()) {
        hackmatch::logger::info("Console initialized");
    }

    const hackmatch::BuildCompatibility& build = hackmatch::detect_steam_build();
    const std::string supported(hackmatch::game_offsets::supported_build);
    if (build.status == hackmatch::BuildStatus::Compatible) {
        hackmatch::logger::success("Steam game build " + build.current_build + " matches this DLL");
    } else if (build.status == hackmatch::BuildStatus::Mismatch) {
        hackmatch::logger::warning("Steam game build " + build.current_build + " does not match DLL build " + supported + "; Hackmatch might not work");
    } else {
        hackmatch::logger::warning("Steam game build could not be detected; this DLL targets build " + supported + "; Hackmatch might not work");
    }

    if (!initialize_runtime()) {
        return 1;
    }
    if (!hackmatch::hooks().init(static_cast<HMODULE>(module))) {
        hackmatch::logger::error("Startup stopped because hook initialization failed");
        return 1;
    }

    hackmatch::logger::info("Controls: Insert toggles the menu; End unloads Hackmatch");
    hackmatch::logger::success("Hackmatch started successfully");
    return 0;
}
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        // API resolution and hook setup must run after the loader lock is released.
        if (HANDLE thread = CreateThread(nullptr, 0, init_thread, instance, 0, nullptr)) {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
