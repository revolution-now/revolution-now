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
#include "gs/players.hpp"

// Revolution Now
#include "lua.hpp"

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
  return base::valid;
}

void reset_players( PlayersState&           players_state,
                    vector<e_nation> const& nations ) {
  auto& players = players_state.players;
  for( e_nation nation : refl::enum_values<e_nation> )
    players[nation].reset();
  for( e_nation nation : nations )
    players[nation] = Player{
        .nation = nation,
        .human  = true,
        .money  = 0,
    };
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// PlayersState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::PlayersState;
  auto u  = st.usertype.create<U>();

  u["players"] = &U::players;
};

// PlayersMap
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::PlayersMap;
  auto u  = st.usertype.create<U>();

  // We could instead do this by overriding the __index
  // metamethod, but then we would not be able to register any
  // further (non-metamethod) members of this userdata.
  u["get"] = [&]( U& obj, e_nation nation ) -> maybe<Player&> {
    return obj[nation];
  };

  u["reset_player"] = []( U& obj, e_nation nation ) -> Player& {
    obj[nation] = Player{};
    return *obj[nation];
  };
};

} // namespace

} // namespace rn
