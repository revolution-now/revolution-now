/****************************************************************
**nation.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-21.
*
* Description: Player/nation enums.
*
*****************************************************************/
#pragma once

#include "nation.rds.hpp"

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
e_nation nation_for( e_player player );

e_player colonist_player_for( e_nation nation );

e_player ref_player_for( e_nation nation );

bool is_ref( e_player player );

} // namespace rn
