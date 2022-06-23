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
#include "logger.hpp"
#include "lua.hpp"
#include "old-world-state.hpp"
#include "util.hpp"

// game-state
#include "gs/players.hpp"

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
  UNWRAP_CHECK( player, players[nation] );
  return player;
}

Player& player_for_nation( PlayersState& players_state,
                           e_nation      nation ) {
  auto& players = players_state.players;
  UNWRAP_CHECK_MSG( player, players[nation],
                    "player for nation {} does not exist.",
                    nation );
  return player;
}

Player const& player_for_nation(
    PlayersState const& players_state, e_nation nation ) {
  auto& players = players_state.players;
  UNWRAP_CHECK_MSG( player, players[nation],
                    "player for nation {} does not exist.",
                    nation );
  return player;
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
