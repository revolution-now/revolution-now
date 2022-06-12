/****************************************************************
**world.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-03.
*
* Description: Helper for building fake worlds for unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/map-square.hpp"
#include "src/unit-id.hpp"

// Rds
#include "src/config/unit-type.rds.hpp"
#include "src/coord.hpp"
#include "src/nation.rds.hpp"

// C++ standard library
#include <memory>
#include <vector>

namespace rn {

// This is so that each unit test doesn't have to include the en-
// tire save game state when it may not need all of it.
struct RootState;

struct FormatVersion;
struct SettingsState;
struct EventsState;
struct UnitsState;
struct PlayersState;
struct TurnState;
struct ColoniesState;
struct LandViewState;
struct TerrainState;

struct Player;
struct IMapUpdater;

} // namespace rn

namespace rn::testing {

// In order to use this the client test file will have to include
// any game state files that it needs.
struct World {
  World();
  ~World() noexcept;

  void build_map( std::vector<MapSquare> tiles, W width );

  UnitId add_unit_in_port( e_unit_type type,
                           e_nation nation = e_nation::dutch );

  UnitId add_unit_on_map( e_unit_type type, Coord where,
                          e_nation nation = e_nation::dutch );

  void add_player( e_nation nation );

  static MapSquare make_ocean();
  static MapSquare make_sea_lane();
  static MapSquare make_grassland();

  Player& dutch();
  Player& english();
  Player& spanish();
  Player& french();

  FormatVersion& version();
  SettingsState& settings();
  EventsState&   events();
  UnitsState&    units();
  PlayersState&  players();
  TurnState&     turn();
  ColoniesState& colonies();
  LandViewState& land_view();
  TerrainState&  terrain();

  RootState& root() { return *root_; }

  IMapUpdater& map_updater() { return *map_updater_; }

 private:
  std::unique_ptr<RootState> root_;

  std::unique_ptr<IMapUpdater> map_updater_;
};

} // namespace rn::testing
