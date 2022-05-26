/****************************************************************
**player.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-29.
*
* Description: Data structure representing a human or AI player.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "nation.hpp"

// Rds
#include "player.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Abseil
#include "absl/types/span.h"

namespace rn {

struct PlayersState;

class Player {
 public:
  Player() = default;

  bool operator==( Player const& ) const = default;

  e_nation nation() const { return o_.nation; }

  bool is_human() const { return o_.human; }
  void set_human( bool yes );

  // Adds the amount (which could be negative) to the player's
  // gold and returns the amount after adding.
  int  add_money( int amount );
  int  money() const { return o_.money; }
  void set_money( int amount );

  bool independence_declared() const {
    return o_.independence_declared;
  }

  maybe<std::string>& discovered_new_world() {
    return o_.discovered_new_world;
  }

  // Implement refl::WrapsReflected.
  Player( wrapped::Player&& o ) : o_( std::move( o ) ) {}
  wrapped::Player const&            refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "Player";

 private:
  wrapped::Player o_;
};

Player& player_for_nation( PlayersState& players_state,
                           e_nation      nation );

Player const& player_for_nation(
    PlayersState const& players_state, e_nation nation );

// FIXME: deprecated
Player& player_for_nation( e_nation nation );

void set_players( PlayersState&                players_state,
                  std::vector<e_nation> const& nations );

void linker_dont_discard_module_player();

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::Player, owned_by_cpp ){};
}
