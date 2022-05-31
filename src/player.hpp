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
#include "fathers.hpp"
#include "nation.hpp"

// Rds
#include "player.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// Abseil
#include "absl/types/span.h"

namespace rn {

struct PlayersState;

struct Player {
  Player() = default;

  bool operator==( Player const& ) const = default;

  e_nation nation() const { return o_.nation; }
  void     set_nation( e_nation nation );

  // Human status.
  bool is_human() const { return o_.human; }
  void set_human( bool yes );

  // Crosses.
  int  crosses() const { return o_.crosses; }
  void set_crosses( int n );

  OldWorldState const& old_world() const {
    return o_.old_world_state;
  }
  OldWorldState& old_world() { return o_.old_world_state; }

  // Adds the amount (which could be negative) to the player's
  // gold and returns the amount after adding.
  int  add_money( int amount );
  int  money() const { return o_.money; }
  void set_money( int amount );

  // Founding fathers.
  void give_father( e_founding_father father );
  bool has_father( e_founding_father father ) const;

  bool independence_declared() const {
    return o_.independence_declared;
  }

  maybe<std::string> discovered_new_world() const {
    return o_.discovered_new_world;
  }
  void set_discovered_new_world( maybe<std::string> name ) {
    o_.discovered_new_world = name;
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

void linker_dont_discard_module_player();

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::Player, owned_by_cpp ){};
}
