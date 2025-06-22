/****************************************************************
**player-mgr.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-22.
*
* Description: Helpers to dealing with player objects.
*
*****************************************************************/
#include "player-mgr.hpp"

// ss
#include "ss/nation.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
Player& add_new_player( SS& ss, e_player const type ) {
  CHECK( !ss.players.players[type].has_value() );
  CHECK( !ss.terrain.refl().player_terrain[type].has_value() );
  Player& p = ss.players.players[type].emplace();

  // Minimal amount needed for a player to be added into the game
  // in a consistent way.
  p.type   = type;
  p.nation = nation_for( type );

  // Terrain.
  ss.mutable_terrain_use_with_care.initialize_player_terrain(
      type,
      /*visible=*/false );

  return p;
}

Player& get_or_add_player( SS& ss, e_player const type ) {
  if( !ss.players.players[type].has_value() )
    return add_new_player( ss, type );
  return *ss.players.players[type];
}

} // namespace rn
