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
#include "ss/nation.hpp"
#include "ss/old-world-state.hpp"
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
    if( player->control != e_player_control::human ) continue;
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

  // Check that only players who have declared have ref players
  // alive.
  for( auto const& [type, player] : players ) {
    if( !player.has_value() ) continue;
    if( is_ref( type ) ) continue;
    bool const ref_should_exist =
        player.has_value() &&
        player->control == e_player_control::human &&
        player->revolution.status >=
            e_revolution_status::declared;
    e_player const ref_player = ref_player_for( player->nation );
    if( ref_should_exist )
      REFL_VALIDATE(
          players[ref_player].has_value(),
          "human player {} has declared independence, but has "
          "no REF player object.",
          type );
    else
      REFL_VALIDATE( !players[ref_player].has_value(),
                     "player {} has an REF player object but "
                     "should not since player {} is either not "
                     "human or has not declared independence.",
                     type, type );
  }

  // Check that if an REF player exists then its corresponding
  // colonial player should exist.
  for( auto const& [type, player] : players ) {
    if( !player.has_value() ) continue;
    if( !is_ref( type ) ) continue;
    e_player const colonial_player =
        colonial_player_for( player->nation );
    REFL_VALIDATE(
        players[colonial_player].has_value(),
        "REF player {} has no corresponding colonial player.",
        type );
  }

  return base::valid;
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
    u["old_world"]           = &U::old_world;
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
    u["get"] = [&]( U& o, e_player player ) -> maybe<Player&> {
      return o[player];
    };

    u["reset_player"] = []( U& o, e_player player ) -> Player& {
      o[player] = Player{};
      return *o[player];
    };
  }();

  // OldWorldMap
  [&] {
    using U = ::rn::OldWorldMap;
    auto u  = st.usertype.create<U>();

    // TODO: make this generic for enum maps.
    u[lua::metatable_key]["__index"] =
        []( U& o, e_nation const type ) -> auto& {
      return o[type];
    };

    // !! NOTE: because we overwrote the __*index metamethods on
    // this userdata we cannot add any further (non-metatable)
    // members on this object, since there will be no way to look
    // them up by name.
  }();
};

} // namespace

} // namespace rn
