#pragma once

#include <array>

namespace raven::sheets {

// Sprite sheet ids referenced from C++. Each must have a matching entry in
// the "sprite_sheets" list of assets/data/config.json; a sheet that is used
// but not registered draws as a grey placeholder rectangle.
// tests/test_content.cpp checks every id in ALL against the shipped config,
// along with the sheets that data files name (enemies, bullet patterns).

inline constexpr const char* PLAYER = "player";           ///< Default player character.
inline constexpr const char* KNIGHT = "knight";           ///< Knight class character.
inline constexpr const char* PROJECTILES = "projectiles"; ///< Bullets.
inline constexpr const char* PICKUPS = "pickups";         ///< Weapon pickups and stabilizers.
inline constexpr const char* PROPS = "props";             ///< Room props such as exits.

/// @brief Every sheet id above, for validation.
inline constexpr std::array ALL = {PLAYER, KNIGHT, PROJECTILES, PICKUPS, PROPS};

// Frame columns within the PICKUPS and PROPS sheets (all on row 0).

inline constexpr int PICKUP_FRAME_WEAPON = 0;     ///< Dropped weapon pickup.
inline constexpr int PICKUP_FRAME_STABILIZER = 1; ///< Weapon stabilizer.
inline constexpr int PROP_FRAME_EXIT_CLOSED = 0;  ///< Exit before the room is cleared.
inline constexpr int PROP_FRAME_EXIT_OPEN = 1;    ///< Exit after the room is cleared.

} // namespace raven::sheets
