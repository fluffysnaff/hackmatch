#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace hackmatch {
enum class BuildStatus {
    Unknown,
    Compatible,
    Mismatch,
};

struct BuildCompatibility {
    BuildStatus status = BuildStatus::Unknown;
    std::string current_build;
};

std::optional<std::string> parse_steam_build_id(std::string_view manifest);
const BuildCompatibility& detect_steam_build();
const BuildCompatibility& build_compatibility();
}
