cmake_minimum_required(VERSION 3.24)
file(STRINGS VERSION HACKMATCH_VERSION LIMIT_COUNT 1)
project(Hackmatch VERSION ${HACKMATCH_VERSION} LANGUAGES C CXX)

if(NOT WIN32)
  message(FATAL_ERROR "Hackmatch builds a Windows DLL")
endif()

enable_language(RC)

include(FetchContent)
include(CTest)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

function(hackmatch_enable_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive- /Zc:__cplusplus)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b
  GIT_SHALLOW TRUE
)
FetchContent_Declare(minhook
  GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
  GIT_TAG v1.3.4
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR cmake-not-used
)
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(imgui minhook nlohmann_json)

add_library(imgui STATIC
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
)
target_include_directories(imgui PUBLIC "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_link_libraries(imgui PUBLIC d3d11)

add_library(minhook STATIC
  "${minhook_SOURCE_DIR}/src/buffer.c"
  "${minhook_SOURCE_DIR}/src/hook.c"
  "${minhook_SOURCE_DIR}/src/trampoline.c"
)
target_include_directories(minhook PUBLIC "${minhook_SOURCE_DIR}/include")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  target_sources(minhook PRIVATE "${minhook_SOURCE_DIR}/src/hde/hde64.c")
else()
  target_sources(minhook PRIVATE "${minhook_SOURCE_DIR}/src/hde/hde32.c")
endif()

add_library(hackmatch SHARED
  src/core/build_info.cpp
  src/core/build_info.h
  src/core/config_manager.cpp
  src/core/config_manager.h
  src/core/game.cpp
  src/core/game.h
  src/core/game_offsets.h
  src/core/il2cpp_api.cpp
  src/core/il2cpp_api.h
  src/core/logger.cpp
  src/core/logger.h
  src/core/settings.cpp
  src/core/settings.h
  src/features/esp.cpp
  src/features/esp.h
  src/features/esp_players.cpp
  src/features/esp_players.h
  src/features/gameplay.cpp
  src/features/gameplay.h
  src/features/feature_limits.h
  src/features/gameplay_items.cpp
  src/features/gameplay_items.h
  src/main.cpp
  src/platform/hooks.cpp
  src/platform/hooks.h
  src/platform/render_hooks.cpp
  src/ui/imgui_menu.cpp
  src/ui/imgui_menu.h
  src/ui/assets.rc
  src/ui/logo.cpp
  src/ui/logo.h
  src/ui/resource.h
  src/ui/menu.cpp
  src/ui/menu.h
  src/ui/theme.cpp
  src/ui/theme.h
  src/ui/widgets.cpp
  src/ui/widgets.h
)
target_compile_features(hackmatch PRIVATE cxx_std_20)
target_compile_definitions(hackmatch PRIVATE HACKMATCH_VERSION="${PROJECT_VERSION}" WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(hackmatch PRIVATE
  src/core
  src/features
  src/platform
  src/ui
)
target_link_libraries(hackmatch PRIVATE imgui minhook nlohmann_json::nlohmann_json d2d1 d3d11 dxgi shell32 shlwapi user32 windowscodecs)
hackmatch_enable_warnings(hackmatch)

if(BUILD_TESTING)
  add_executable(hackmatch_tests tests/feature_limits_tests.cpp)
  target_compile_features(hackmatch_tests PRIVATE cxx_std_20)
  target_include_directories(hackmatch_tests PRIVATE src/features)
  hackmatch_enable_warnings(hackmatch_tests)
  add_test(NAME feature_limits COMMAND hackmatch_tests)

  add_executable(hackmatch_config_tests
    tests/config_manager_tests.cpp
    src/core/config_manager.cpp
    src/core/settings.cpp
  )
  target_compile_features(hackmatch_config_tests PRIVATE cxx_std_20)
  target_compile_definitions(hackmatch_config_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_include_directories(hackmatch_config_tests PRIVATE src/core src/features)
  target_link_libraries(hackmatch_config_tests PRIVATE nlohmann_json::nlohmann_json user32)
  hackmatch_enable_warnings(hackmatch_config_tests)
  add_test(NAME config_manager COMMAND hackmatch_config_tests)

  add_executable(hackmatch_build_info_tests
    tests/build_info_tests.cpp
    src/core/build_info.cpp
  )
  target_compile_features(hackmatch_build_info_tests PRIVATE cxx_std_20)
  target_compile_definitions(hackmatch_build_info_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_include_directories(hackmatch_build_info_tests PRIVATE src/core)
  hackmatch_enable_warnings(hackmatch_build_info_tests)
  add_test(NAME build_info COMMAND hackmatch_build_info_tests)
endif()

option(HACKMATCH_BUILD_UI_PREVIEW "Build the deterministic menu screenshot harness" ON)
if(HACKMATCH_BUILD_UI_PREVIEW)
  add_executable(hackmatch_ui_preview WIN32
    src/core/config_manager.cpp
    src/core/settings.cpp
    src/ui/imgui_menu.cpp
    src/ui/assets.rc
    src/ui/logo.cpp
    src/ui/resource.h
    src/ui/preview_main.cpp
    src/ui/theme.cpp
    src/ui/widgets.cpp
  )
  target_compile_features(hackmatch_ui_preview PRIVATE cxx_std_20)
  target_compile_definitions(hackmatch_ui_preview PRIVATE
    HACKMATCH_VERSION="${PROJECT_VERSION}"
    HACKMATCH_UI_PREVIEW
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )
  target_include_directories(hackmatch_ui_preview PRIVATE src/core src/features src/ui)
  target_link_libraries(hackmatch_ui_preview PRIVATE imgui nlohmann_json::nlohmann_json d2d1 d3d11 dxgi ole32 shell32 shlwapi user32 windowscodecs)
  hackmatch_enable_warnings(hackmatch_ui_preview)
endif()
