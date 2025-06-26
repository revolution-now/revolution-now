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
#include "ss/natives.hpp"
#include "ss/player.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::refl::enum_values;

/****************************************************************
** Helpers.
*****************************************************************/
void populate_for_REF( SS& ss, Player& player ) {
  // Prevent the REF player from "discovering the new world."
  player.new_world_name = "The New World";

  // Prevent woodcuts.
  for( auto& [woodcut, done] : player.woodcuts ) done = true;

  // Prevent meeting the natives.
  for( e_tribe const tribe_type : enum_values<e_tribe> )
    if( ss.natives.tribe_exists( tribe_type ) )
      ss.natives.tribe_for( tribe_type )
          .relationship[player.type]
          .encountered = true;
}

} // namespace

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

  if( is_ref( type ) ) populate_for_REF( ss, p );

  return p;
}

Player& get_or_add_player( SS& ss, e_player const type ) {
  if( !ss.players.players[type].has_value() )
    return add_new_player( ss, type );
  return *ss.players.players[type];
}

OldWorldState const& old_world_state( SSConst const& ss,
                                      e_player const type ) {
  return ss.players.old_world[nation_for( type )];
}

OldWorldState& old_world_state( SS& ss, e_player const type ) {
  return ss.players.old_world[nation_for( type )];
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( add_new_player, Player&, e_player const player_type ) {
  SS& ss = st["SS"].as<SS&>();
  return add_new_player( ss, player_type );
}

} // namespace

} // namespace rn
