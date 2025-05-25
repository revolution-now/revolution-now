/****************************************************************
**nation.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-31.
*
# Description: Nation-specific config info.
*
*****************************************************************/
#pragma once

// Rds
#include "config/nation.rds.hpp"

namespace rn {

config::nation::Player const& player_obj( e_player player );

config::nation::Nation const& nation_obj( e_nation nation );

} // namespace rn
