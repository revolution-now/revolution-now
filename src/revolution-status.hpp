/****************************************************************
**revolution-status.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-13.
*
* Description: Things that change depending on revolution status.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <string_view>

namespace rn {

struct Player;

// In the OG, after independence is declared, the game no longer
// refers to units as e.g. "Dutch Soldier" but instead "Rebel
// Soldier".
std::string_view player_possessive( Player const& player );

// Same as above but yields "Rebels" after declaration.
std::string_view player_display_name( Player const& player );

} // namespace rn
