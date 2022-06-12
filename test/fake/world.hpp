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
#include "src/colony-id.hpp"
#include "src/map-square.hpp"
#include "src/unit-id.hpp"

// Rds
#include "src/colony.rds.hpp"
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

struct Colony;
struct IMapUpdater;
struct Player;

} // namespace rn

namespace rn::testing {

// In order to use this the client test file will have to include
// any game state files that it needs.
//
// TODO: consider moving this into the rn library and exposing it
// in lua as it might end up being the right way to expose an API
// to Lua.
struct World {
  World();
  ~World() noexcept;

  void build_map( std::vector<MapSquare> tiles, W width );

  UnitId add_unit_in_port( e_unit_type type,
                           e_nation nation = e_nation::dutch );

  UnitId add_unit_on_map( e_unit_type type, Coord where,
                          e_nation nation = e_nation::dutch );

  // Create a unit and add give it the specified indoor job in
  // the colony.
  UnitId add_unit_indoors(
      ColonyId colony_id, e_indoor_job indoor_job,
      e_unit_type type = e_unit_type::free_colonist );

  // Create a unit and add give it the specified outdoor job in
  // the colony.
  UnitId add_unit_outdoors(
      ColonyId colony_id, e_direction d,
      e_outdoor_job outdoor_job,
      e_unit_type   type = e_unit_type::free_colonist );

  // Create a colony using the founder unit on the same square as
  // the unit.
  Colony& add_colony( UnitId founder );

  // This one will create a free_colonist on the square and then
  // use it to found the colony.
  Colony& add_colony( Coord    where,
                      e_nation nation = e_nation::dutch );

  void add_player( e_nation nation );

  // This will call the validate method on each colony in the
  // state and return an error if any of them fail.
  base::valid_or<std::string> validate_colonies() const;

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

  FormatVersion const& version() const;
  SettingsState const& settings() const;
  EventsState const&   events() const;
  UnitsState const&    units() const;
  PlayersState const&  players() const;
  TurnState const&     turn() const;
  ColoniesState const& colonies() const;
  LandViewState const& land_view() const;
  TerrainState const&  terrain() const;

  RootState&       root() { return *root_; }
  RootState const& root() const { return *root_; }

  IMapUpdater& map_updater() { return *map_updater_; }

 private:
  std::unique_ptr<RootState> root_;

  std::unique_ptr<IMapUpdater> map_updater_;
};

} // namespace rn::testing
