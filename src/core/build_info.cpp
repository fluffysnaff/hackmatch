#include "build_info.h"

#include "game_offsets.h"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace hackmatch {
namespace {
BuildCompatibility compatibility;
}

std::optional<std::string> parse_steam_build_id(std::string_view manifest)
{
    constexpr std::string_view key = "\"buildid\"";
    const size_t key_position = manifest.find(key);
    if (key_position == std::string_view::npos) return std::nullopt;
    const size_t value_start = manifest.find('"', key_position + key.size());
    if (value_start == std::string_view::npos) return std::nullopt;
    const size_t value_end = manifest.find('"', value_start + 1);
    if (value_end == std::string_view::npos || value_end == value_start + 1) return std::nullopt;
    const std::string_view value = manifest.substr(value_start + 1, value_end - value_start - 1);
    if (!std::ranges::all_of(value, [](char character) { return character >= '0' && character <= '9'; })) return std::nullopt;
    return std::string(value);
}

const BuildCompatibility& detect_steam_build()
{
    compatibility = {};
    wchar_t executable_path[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, executable_path, MAX_PATH) == 0) return compatibility;
    const std::filesystem::path manifest_path =
        std::filesystem::path(executable_path).parent_path().parent_path().parent_path() / L"appmanifest_1280770.acf";
    std::ifstream input(manifest_path, std::ios::binary);
    if (!input) return compatibility;
    const std::string manifest(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>{});
    const std::optional<std::string> build = parse_steam_build_id(manifest);
    if (!build) return compatibility;
    compatibility.current_build = *build;
    compatibility.status = compatibility.current_build == game_offsets::supported_build ? BuildStatus::Compatible : BuildStatus::Mismatch;
    return compatibility;
}

const BuildCompatibility& build_compatibility()
{
    return compatibility;
}
}
