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
#include "revolution.rds.hpp"
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

namespace {

using ::refl::enum_values;

}

base::valid_or<string> PlayersState::validate() const {
  // Check that players have the correct player relative to their
  // key in the map.
  for( auto const& [type, player] : players )
    if( player.has_value() )
      REFL_VALIDATE( player->type == type,
                     "mismatch in player nations for the {}.",
                     type );

  // Ensure that at most one human player has declared.
  for( int count = 0; auto const& [type, player] : players ) {
    if( !player.has_value() ) continue;
    if( !player->human ) continue;
    if( player->revolution.status >=
        e_revolution_status::declared )
      ++count;
    REFL_VALIDATE(
        count <= 1,
        "There are multiple human players that have declared "
        "independence, but at most one is allowed to do so." );
  }

  // Check that the players have symmetric relationships.
  for( auto const& [l_type, l_player] : players ) {
    if( !l_player.has_value() ) continue;
    for( e_player const r_type : enum_values<e_player> ) {
      if( r_type == l_type ) continue;
      auto const& r_player = players[r_type];
      if( !r_player.has_value() ) {
        REFL_VALIDATE(
            l_player->relationship_with[r_type] ==
                e_euro_relationship::none,
            "player {} has a relationship status with player {} "
            "but player {} does not exist.",
            l_type, r_type, r_type );
        continue;
      }
      CHECK( r_player.has_value() );
      REFL_VALIDATE( l_player->relationship_with[r_type] ==
                         r_player->relationship_with[l_type],
                     "player {} has assymetric relationship "
                     "status with player {}.",
                     l_type, r_type );
    }
  }

  return base::valid;
}

void reset_players( PlayersState& players_state,
                    vector<e_player> const& nations,
                    base::maybe<e_player> human ) {
  auto& players = players_state.players;
  for( e_player type : refl::enum_values<e_player> )
    players[type].reset();
  for( e_player type : nations )
    players[type] = Player{
      .type  = type,
      .money = 0,
    };
  set_unique_human_player( players_state, human );
}

void set_unique_human_player(
    PlayersState& players, base::maybe<e_player> player_type ) {
  for( e_player const n : refl::enum_values<e_player> )
    if( players.players[n].has_value() )
      players.players[n]->human = ( n == player_type );
  CHECK_HAS_VALUE( players.validate() );
}

Player& player_for_player_or_die( PlayersState& players,
                                  e_player player_type ) {
  UNWRAP_CHECK_MSG( player, players.players[player_type],
                    "player for player {} does not exist.",
                    player_type );
  return player;
}

Player const& player_for_player_or_die(
    PlayersState const& players, e_player player_type ) {
  UNWRAP_CHECK_MSG( player, players.players[player_type],
                    "player for player {} does not exist.",
                    player_type );
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
    u["get"] = [&]( U& obj, e_player player ) -> maybe<Player&> {
      return obj[player];
    };

    u["reset_player"] = []( U& obj,
                            e_player player ) -> Player& {
      obj[player] = Player{};
      return *obj[player];
    };
  }();
};

} // namespace

} // namespace rn
