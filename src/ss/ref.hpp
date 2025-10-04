/****************************************************************
**ref.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-02.
*
* Description: Holds what is saved when a game is saved.
*
*****************************************************************/
#pragma once

// luapp
#include "luapp/ext-userdata.hpp"

// base
#include "base/valid.hpp"

// C++ standard library
#include <memory>
#include <string>

namespace rn {

/****************************************************************
** Fwd Decls
*****************************************************************/
// Each of the subsections of the top-level state struct get for-
// ward declared, that way a module that only needs access to one
// can just include it and isn't forced to pull in all of the
// other headers.
struct FormatVersion;
struct SettingsState;
struct EventsState;
struct UnitsState;
struct PlayersState;
struct TurnState;
struct ColoniesState;
struct NativesState;
struct LandViewState;
struct MapState;
struct TerrainState;

struct RootState;

/****************************************************************
** SS (serialized state)
*****************************************************************/
struct SS;

struct SSConst {
  SSConst( SS& ss );
  SSConst( SS const& ss );

  SSConst( SSConst const& ) = delete;

 private:
  SS const& ss_;

 public:
  FormatVersion const& version;
  SettingsState const& settings;
  EventsState const& events;
  UnitsState const& units;
  PlayersState const& players;
  TurnState const& turn;
  ColoniesState const& colonies;
  NativesState const& natives;
  LandViewState const& land_view;
  MapState const& map;
  TerrainState const& terrain;

  RootState const& root;

  // This will run validation routines recursively over the en-
  // tire save-game state. It is expensive to run.
  base::valid_or<std::string> validate_full_game_state() const;

  // This will run validation routines recursively over the en-
  // tire save-game state but without zzz_terrain, thus it is
  // less expensive (though not free).
  base::valid_or<std::string> validate_non_terrain_game_state()
      const;
};

struct SS {
  SS();
  ~SS();

  operator SSConst const&() const ATTR_LIFETIMEBOUND {
    return as_const;
  }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  // This object is self-referential and thus must be immobile.
  SS( SS& ) = delete;

 public:
  FormatVersion& version;
  SettingsState& settings;
  EventsState& events;
  UnitsState& units;
  PlayersState& players;
  TurnState& turn;
  ColoniesState& colonies;
  NativesState& natives;
  LandViewState& land_view;
  MapState& map;
  TerrainState const& terrain;

  // This should really only be used to initialize the IMapUp-
  // dater object, since terrain updates typically always need to
  // be made via that interface so that the rendered copy on the
  // GPU can be updated properly.
  TerrainState& mutable_terrain_use_with_care;

  RootState& root;

  // Should be last.
  SSConst const as_const;
};

void to_str( SS const& o, std::string& out, base::tag<SS> );

/****************************************************************
** Root reference helpers.
*****************************************************************/
// These are here for convenience; they allow performing the
// named actions on the RootState with only references, and thus
// the caller doesn't have to include all of the state. We can
// easily implement these in this module's cpp file because it
// already includes the root header as it manages the root state
// itself.
[[nodiscard]] bool root_states_equal( RootState const& l,
                                      RootState const& r );
void assign_src_to_dst( RootState const& src, RootState& dst );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::SS, owned_by_cpp ){};

}
