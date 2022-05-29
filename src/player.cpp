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
** Player
*****************************************************************/
int Player::add_money( int amount ) {
  o_.money += amount;
  return o_.money;
}

void Player::set_money( int amount ) {
  DCHECK( amount >= 0 );
  o_.money = amount;
}

void Player::set_human( bool yes ) { o_.human = yes; }

void Player::set_crosses( int n ) {
  DCHECK( n >= 0 );
  o_.crosses = n;
}

void Player::set_nation( e_nation nation ) {
  o_.nation = nation;
}

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

// Founding fathers.
void Player::give_father( e_founding_father father ) {
  o_.fathers[father] = true;
}

bool Player::has_father( e_founding_father father ) const {
  return o_.fathers[father];
}

void linker_dont_discard_module_player() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::Player;

  auto u = st.usertype.create<U>();

  u["nation"]     = &U::nation;
  u["set_nation"] = &U::set_nation;

  u["is_human"]  = &U::is_human;
  u["set_human"] = &U::set_human;

  u["crosses"]     = &U::crosses;
  u["set_crosses"] = &U::set_crosses;

  u["old_world"] = []( U& obj ) -> OldWorldState& {
    return obj.old_world();
  };

  u["add_money"] = &U::add_money;
  u["money"]     = &U::money;
  u["set_money"] = &U::set_money;

  u["give_father"] = &U::give_father;
  u["has_father"]  = &U::has_father;

  u["independence_declared"] = &U::independence_declared;

  u["discovered_new_world"]     = &U::discovered_new_world;
  u["set_discovered_new_world"] = &U::set_discovered_new_world;
};

} // namespace

} // namespace rn
