/****************************************************************
**roles.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-01.
*
* Description: Manages the different roles that each player can
*              have with regard to driving the game and map
*              visibility.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "roles.rds.hpp"

// Revolution Now
#include "maybe.hpp"

namespace rn {

struct SSConst;

enum class e_player;

maybe<e_player> player_for_role( SSConst const& ss,
                                 e_player_role role );

} // namespace rn
