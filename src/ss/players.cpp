/****************************************************************
**gs-players.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Serializable player-related state.
*
*****************************************************************/
#include "ss/players.hpp"

// ss
#include "ss/market.hpp"
#include "ss/player.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

base::valid_or<string> PlayersState::validate() const {
  // Check that players have the correct nation relative to their
  // key in the map.
  for( auto const& [nation, player] : players )
    if( player.has_value() )
      REFL_VALIDATE( player->nation == nation,
                     "mismatch in player nations for the {}.",
                     nation );

  // Check that the default_human nation, if it has a value, cor-
  // responds to a player that exists.
  if( default_human.has_value() )
    REFL_VALIDATE( players[*default_human].has_value(),
                   "default human player set to {} but that "
                   "that player does not exist.",
                   *default_human );

  // Check that, if there is a default human player, that that
  // player is marked as a possible human player in the humans
  // map.
  if( default_human.has_value() )
    REFL_VALIDATE(
        humans[*default_human] == true,
        "default human player set to {} but that that player is "
        "not in the list of allowed human players.",
        *default_human );

  return base::valid;
}

void reset_players( PlayersState&           players_state,
                    vector<e_nation> const& nations,
                    base::maybe<e_nation>   human ) {
  auto& players = players_state.players;
  for( e_nation nation : refl::enum_values<e_nation> )
    players[nation].reset();
  for( e_nation nation : nations )
    players[nation] = Player{
        .nation = nation,
        .money  = 0,
    };
  set_unique_human_player( players_state, human );
}

void set_unique_human_player( PlayersState&         players,
                              base::maybe<e_nation> nation ) {
  players.default_human = nation;
  for( e_nation const n : refl::enum_values<e_nation> )
    players.humans[n] = ( n == nation );
  CHECK_HAS_VALUE( players.validate() );
}

Player& player_for_nation_or_die( PlayersState& players,
                                  e_nation      nation ) {
  UNWRAP_CHECK_MSG( player, players.players[nation],
                    "player for nation {} does not exist.",
                    nation );
  return player;
}

Player const& player_for_nation_or_die(
    PlayersState const& players, e_nation nation ) {
  UNWRAP_CHECK_MSG( player, players.players[nation],
                    "player for nation {} does not exist.",
                    nation );
  return player;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// PlayersState
LUA_STARTUP( lua::state& st ) {
  [&] {
    using U = ::rn::PlayersState;
    auto u  = st.usertype.create<U>();

    u["players"]             = &U::players;
    u["humans"]              = &U::humans;
    u["default_human"]       = &U::default_human;
    u["global_market_state"] = &U::global_market_state;
    u[lua::metatable_key]    = st.table.create();
  }();

  // PlayersMap
  [&] {
    using U = ::rn::PlayersMap;
    auto u  = st.usertype.create<U>();

    // We could instead do this by overriding the __index
    // metamethod, but then we would not be able to register any
    // further (non-metamethod) members of this userdata.
    u["get"] = [&]( U& obj, e_nation nation ) -> maybe<Player&> {
      return obj[nation];
    };

    u["reset_player"] = []( U&       obj,
                            e_nation nation ) -> Player& {
      obj[nation] = Player{};
      return *obj[nation];
    };
  }();

  // HumansMap.
  [&] {
    using U = ::rn::HumansMap;
    auto u  = st.usertype.create<U>();

    // TODO: make this more automated and/or find a different way
    // to do it, since we can't add anymore methods to this ob-
    // ject since we're overriding the metatable.
    u[lua::metatable_key]["__index"] =
        [&]( U& obj, e_nation nation ) { return obj[nation]; };

    u[lua::metatable_key]["__newindex"] =
        [&]( U& obj, e_nation nation, bool b ) {
          obj[nation] = b;
        };
  }();
};

} // namespace

} // namespace rn
