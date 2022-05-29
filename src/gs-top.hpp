/****************************************************************
**gs-top.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-14.
*
* Description: Top-level struct representing the data that is
*              saved when a game is saved.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "gs-top.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

struct TopLevelState {
  TopLevelState();
  bool operator==( TopLevelState const& ) const = default;

  // Implement refl::WrapsReflected.
  TopLevelState( wrapped::TopLevelState&& o );
  wrapped::TopLevelState const&     refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "TopLevelState";

  valid_or<std::string> validate() const;
  void                  validate_or_die() const;

  FormatVersion&       version() { return o_.version; }
  FormatVersion const& version() const { return o_.version; }

  SettingsState&       settings() { return o_.settings; }
  SettingsState const& settings() const { return o_.settings; }

  EventsState&       events() { return o_.events; }
  EventsState const& events() const { return o_.events; }

  UnitsState&       units() { return o_.units; }
  UnitsState const& units() const { return o_.units; }

  PlayersState&       players() { return o_.players; }
  PlayersState const& players() const { return o_.players; }

  TurnState&       turn() { return o_.turn; }
  TurnState const& turn() const { return o_.turn; }

  ColoniesState&       colonies() { return o_.colonies; }
  ColoniesState const& colonies() const { return o_.colonies; }

  LandViewState&       land_view() { return o_.land_view; }
  LandViewState const& land_view() const { return o_.land_view; }

  TerrainState&       terrain() { return o_.zzz_terrain; }
  TerrainState const& terrain() const { return o_.zzz_terrain; }

 private:
  // ----- Serializable state.
  wrapped::TopLevelState o_;

  // ----- Non-serializable (transient) state.
  // None.
};

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::TopLevelState, owned_by_cpp ){};

} // namespace lua
