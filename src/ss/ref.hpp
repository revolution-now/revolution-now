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
struct LandViewState;
struct TerrainState;

struct RootState;

/****************************************************************
** SS (serialized state)
*****************************************************************/
struct SS {
  SS();
  ~SS();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  FormatVersion&      version;
  SettingsState&      settings;
  EventsState&        events;
  UnitsState&         units;
  PlayersState&       players;
  TurnState&          turn;
  ColoniesState&      colonies;
  LandViewState&      land_view;
  TerrainState const& terrain;

  // This should really only be used to initialize the IMapUp-
  // dater object, since terrain updates typically always need to
  // be made via that interface so that the rendered copy on the
  // GPU can be updated properly.
  TerrainState& mutable_terrain_use_with_care;

  RootState& root;
};

struct SSConst {
  SSConst( SS& ss );

 private:
  SS& ss_;

 public:
  FormatVersion const& version;
  SettingsState const& settings;
  EventsState const&   events;
  UnitsState const&    units;
  PlayersState const&  players;
  TurnState const&     turn;
  ColoniesState const& colonies;
  LandViewState const& land_view;
  TerrainState const&  terrain;

  RootState const& root;

  // This will run validation routines recursively over the en-
  // tire save-game state. It is probably expensive to run.
  base::valid_or<std::string> validate_game_state() const;
};

} // namespace rn
