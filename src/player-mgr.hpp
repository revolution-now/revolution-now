/****************************************************************
**player-mgr.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-22.
*
* Description: Helpers to dealing with player objects.
*
*****************************************************************/
#pragma once

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct OldWorldState;
struct Player;
struct SS;
struct SSConst;

enum class e_player;

/****************************************************************
** Functions.
*****************************************************************/
Player& add_new_player( SS& ss, e_player type );

Player& get_or_add_player( SS& ss, e_player type );

OldWorldState const& old_world_state( SSConst const& ss,
                                      e_player type );

OldWorldState& old_world_state( SS& ss, e_player type );

} // namespace rn
