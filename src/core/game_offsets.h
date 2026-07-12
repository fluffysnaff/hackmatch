#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace hackmatch::game_offsets
{
// Redmatch 2 Steam build used to generate the current metadata reference. Update this value together
// with every RVA and raw layout offset below.
inline constexpr std::string_view supported_build = "23904900";

namespace methods
{
inline constexpr std::uintptr_t fire_primary_shot = 0x1814A80;
inline constexpr std::uintptr_t compute_weapon_spread = 0x1804CD0;
inline constexpr std::uintptr_t update_crosshair_spread = 0x1808B30;
inline constexpr std::uintptr_t physics_raycast = 0x17D9740;
inline constexpr std::uintptr_t physics_raycast_all = 0x17D84F0;
} // namespace methods

namespace fields
{
inline constexpr std::ptrdiff_t identity_player_data = 0x20;
inline constexpr std::ptrdiff_t player_data_name = 0x20;
inline constexpr std::ptrdiff_t player_items = 0x60;
inline constexpr std::ptrdiff_t player_selected_item = 0x108;
inline constexpr std::ptrdiff_t player_shield_state = 0x10C;
inline constexpr std::ptrdiff_t player_stats = 0x110;
inline constexpr std::ptrdiff_t player_movement = 0x11C;
inline constexpr std::ptrdiff_t player_ads = 0x161;
inline constexpr std::ptrdiff_t player_sprinting = 0x164;
inline constexpr std::ptrdiff_t player_crosshair_left = 0x98;
inline constexpr std::ptrdiff_t player_crosshair_right = 0xA0;
inline constexpr std::ptrdiff_t player_crosshair_up = 0xA8;
inline constexpr std::ptrdiff_t player_crosshair_down = 0xB0;
inline constexpr std::ptrdiff_t player_rigidbody = 0x180;
inline constexpr std::ptrdiff_t player_crosshair_style = 0x208;
inline constexpr std::ptrdiff_t player_measured_speed = 0x280;
inline constexpr std::ptrdiff_t item_info = 0x18;
} // namespace fields
} // namespace hackmatch::game_offsets
