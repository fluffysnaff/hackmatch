#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace hackmatch::game_offsets {
// Redmatch 2 Steam build used to generate the current metadata reference. Update this value together
// with every RVA and raw layout offset below.
inline constexpr std::string_view supported_build = "23904900";

namespace methods {
inline constexpr std::uintptr_t player_shot_primary = 0x1814A80;
inline constexpr std::uintptr_t player_shot_secondary = 0x180DB00;
inline constexpr std::uintptr_t physics_raycast = 0x17D9740;
inline constexpr std::uintptr_t physics_raycast_all = 0x17D84F0;
inline constexpr std::uintptr_t physics_raycast_non_alloc = 0x17D8AA0;
}

namespace fields {
inline constexpr std::ptrdiff_t identity_player_data = 0x20;
inline constexpr std::ptrdiff_t player_data_name = 0x20;
inline constexpr std::ptrdiff_t player_stats = 0x110;
inline constexpr std::ptrdiff_t player_sprinting = 0x164;
}
}
