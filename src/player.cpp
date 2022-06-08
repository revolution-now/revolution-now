/****************************************************************
**player.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-29.
*
* Description: Data structure representing a human or AI player.
*
*****************************************************************/
#include "player.hpp"

// Revolution Now
#include "fathers.hpp"
#include "game-state.hpp"
#include "gs-players.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "old-world-state.hpp"
#include "util.hpp"

// Rds
#include "gs-players.rds.hpp"

// luapp
#include "luapp/as.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/iter.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"
#include "base/to-str-tags.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
Player& player_for_nation( e_nation nation ) {
  auto& players = GameState::players().players;
  CHECK( players.contains( nation ) );
  return players[nation];
}

Player& player_for_nation( PlayersState& players_state,
                           e_nation      nation ) {
  auto it = players_state.players.find( nation );
  CHECK( it != players_state.players.end(),
         "player for nation {} does not exist.", nation );
  return it->second;
}

Player const& player_for_nation(
    PlayersState const& players_state, e_nation nation ) {
  auto it = players_state.players.find( nation );
  CHECK( it != players_state.players.end(),
         "player for nation {} does not exist.", nation );
  return it->second;
}

void linker_dont_discard_module_player() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::Player;

  auto u = st.usertype.create<U>();

  u["nation"]                = &U::nation;
  u["human"]                 = &U::human;
  u["money"]                 = &U::money;
  u["crosses"]               = &U::crosses;
  u["old_world"]             = &U::old_world;
  u["discovered_new_world"]  = &U::discovered_new_world;
  u["independence_declared"] = &U::independence_declared;
  u["fathers"]               = &U::fathers;
  u["starting_position"]     = &U::starting_position;
  u["last_high_seas"]        = &U::last_high_seas;
};

} // namespace

} // namespace rn
