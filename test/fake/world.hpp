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
#include "src/maybe.hpp"

// ss
#include "src/ss/colony-id.hpp"
#include "src/ss/colony.rds.hpp"
#include "src/ss/dwelling-id.hpp"
#include "src/ss/fog-square.rds.hpp"
#include "src/ss/map-square.rds.hpp"
#include "src/ss/nation.rds.hpp"
#include "src/ss/native-enums.rds.hpp"
#include "src/ss/unit-id.hpp"
#include "src/ss/unit-type.rds.hpp"

// gfx
#include "src/gfx/coord.hpp"

// C++ standard library
#include <memory>
#include <vector>

// This allows us to define a test case whose body runs as a
// method in the World class (or something derived from it).
#define TEST_WORLD( ... ) TEST_CASE_METHOD( World, __VA_ARGS__ )

namespace lua {
struct state;
}

namespace rn {

// This is so that each unit test doesn't have to include the en-
// tire save game state when it may not need all of it.
struct RootState;

struct ColoniesState;
struct Colony;
struct Dwelling;
struct EventsState;
struct FormatVersion;
struct IMapUpdater;
struct LandViewState;
struct MockIColonyViewer;
struct MockICombat;
struct MockIGui;
struct MockIRand;
struct NativeUnit;
struct NativesState;
struct Planes;
struct Player;
struct PlayersState;
struct SS;
struct SSConst;
struct SettingsState;
struct TS;
struct TerrainConnectivity;
struct TerrainState;
struct Tribe;
struct TurnState;
struct Unit;
struct UnitComposition;
struct UnitType;
struct UnitsState;

} // namespace rn

namespace rn::testing {

// In order to use this the client test file will have to include
// any game state files that it needs.
struct World {
  World();
  ~World() noexcept;

  // ------------------------------------------------------------
  // Terrain.
  // ------------------------------------------------------------
  void build_map( std::vector<MapSquare> tiles, W width );

  // This is not done automatically for efficiency and because
  // very few tests will need it.
  void update_terrain_connectivity();

  // For each player that exists, this will re-initialize its
  // player map to be the size of the real map with all tiles
  // hidden.
  void init_player_maps();

  static MapSquare make_ocean();
  static MapSquare make_sea_lane();
  static MapSquare make_grassland();
  static MapSquare make_terrain( e_terrain terrain );

  // Access the mutable map. NOTE: functions like this should not
  // be used outside of unit tests, since normal game code should
  // always use the IMapUpdater interface to update the map.
  // Const versions of it are fine though.
  MapSquare& square( gfx::point p );

  // This gets the terrain info from the player-specific map.
  maybe<FogSquare>& player_square(
      gfx::point p, maybe<e_nation> nation = nothing );

  void add_forest( gfx::point p );
  void add_mountains( gfx::point p );
  void add_hills( gfx::point p );
  void add_road( gfx::point p );
  void add_plow( gfx::point p );
  void add_minor_river( gfx::point p );
  void add_major_river( gfx::point p );

  // ------------------------------------------------------------
  // Creating units.
  // ------------------------------------------------------------
  Unit& add_unit_in_port( e_unit_type     type,
                          maybe<e_nation> nation = nothing );

  NativeUnit& add_native_unit_on_map( e_native_unit_type type,
                                      Coord              where,
                                      DwellingId dwelling_id );

  Unit& add_unit_on_map( UnitComposition const& comp,
                         Coord                  where,
                         maybe<e_nation> nation = nothing );

  Unit& add_missionary_in_dwelling(
      UnitType missionary_type, DwellingId dwelling_id,
      maybe<e_nation> nation = nothing );

  // Unit created will be the same nation as the holder.
  Unit& add_unit_in_cargo( e_unit_type type, UnitId holder );

  // This will create a unit that is registered in the game but
  // without any ownership.
  Unit& add_free_unit( e_unit_type     type,
                       maybe<e_nation> nation = nothing );
  // This will create a unit that is registered in the game but
  // without any ownership, which for native units also implies
  // that it has no associated dwelling or tribe.
  NativeUnit& add_free_unit( e_native_unit_type type );

  // Create a unit and add give it the specified indoor job in
  // the colony.
  Unit& add_unit_indoors(
      ColonyId colony_id, e_indoor_job indoor_job,
      e_unit_type type = e_unit_type::free_colonist );

  // Same as above but adds the expert colonist type for the job.
  Unit& add_expert_unit_indoors( ColonyId     colony_id,
                                 e_indoor_job indoor_job );

  // Create a unit and add give it the specified outdoor job in
  // the colony.
  Unit& add_unit_outdoors(
      ColonyId colony_id, e_direction d,
      e_outdoor_job outdoor_job,
      e_unit_type   type = e_unit_type::free_colonist );

  // Same as above but adds the expert colonist type for the job.
  Unit& add_expert_unit_outdoors( ColonyId      colony_id,
                                  e_direction   d,
                                  e_outdoor_job outdoor_job );

  // ------------------------------------------------------------
  // Steering ships.
  // ------------------------------------------------------------
  // Given a unit that is a ship (in any state) it will change it
  // to be outbound.
  void ship_to_outbound( UnitId id );

  // ------------------------------------------------------------
  // Creating Colonies.
  // ------------------------------------------------------------
  // Create a colony using the founder unit on the same square as
  // the unit.
  Colony& add_colony( UnitId founder );

  // This one will create an empty colony with no units (though
  // it will have the default set of buildings). It is not really
  // a valid colony therefore, but should be fine for testing.
  Colony& add_colony( Coord           where,
                      maybe<e_nation> nation = nothing );

  // This will create a free colonist on the square and use it to
  // found a colony, this it should be a realistic valid colony.
  Colony& add_colony_with_new_unit(
      Coord where, maybe<e_nation> nation = nothing );

  // ------------------------------------------------------------
  // Creating Native Dwellings.
  // ------------------------------------------------------------
  Dwelling& add_dwelling( Coord where, e_tribe tribe );

  Tribe& add_tribe( e_tribe tribe );

  Tribe& tribe( e_tribe tribe );

  Tribe& apache();
  Tribe& sioux();
  Tribe& tupi();
  Tribe& arawak();
  Tribe& cherokee();
  Tribe& iroquois();
  Tribe& aztec();
  Tribe& inca();

  // ------------------------------------------------------------
  // Colony setup.
  // ------------------------------------------------------------
  void give_all_buildings( Colony& colony );

  // ------------------------------------------------------------
  // Market Prices.
  // ------------------------------------------------------------
  // This will set the current bid price for the default player
  // to the given amount. But note that it will not change the
  // starting bid price or any volumes, so the equilibrium price
  // won't be affected.
  void set_current_bid_price( e_commodity type,
                              int         price_in_hundreds );

  // This will set both the current price and the starting price,
  // effectively setting the equilibrium price as well. But note
  // that this won't work for the processed goods which are in a
  // price group, and so this will check fail for those.
  void set_stable_bid_price( e_commodity type,
                             int         price_in_hundreds );

  // This will set the price of the commodity to the middle of
  // its range, rounded down.
  void init_price_to_average( e_commodity type );

  // This will set the price of each commodity to the middle of
  // its range, rounded down.
  void init_prices_to_average();

  // Sets it for the default player.
  void set_tax_rate( int rate );

  // ------------------------------------------------------------
  // Players.
  // ------------------------------------------------------------
  void add_player( e_nation nation );
  void add_default_player();

  e_nation default_nation() const { return default_nation_; }

  void set_default_player( e_nation nation ) {
    default_nation_ = nation;
  }

  // Note that this function has a different convention than the
  // usual in this module. When `nation` is nothing it means that
  // there is no human player, as opposed to the usual meaning
  // which would be that the "default" player is selected.
  void set_human_player( maybe<e_nation> nation );

  void set_default_player_as_human();

  // This will call the validate method on each colony in the
  // state and return an error if any of them fail.
  base::valid_or<std::string> validate_colonies() const;

  Player&       dutch();
  Player&       english();
  Player&       spanish();
  Player&       french();
  Player const& dutch() const;
  Player const& english() const;
  Player const& spanish() const;
  Player const& french() const;

  Player&       default_player();
  Player const& default_player() const;

  Player&       player( maybe<e_nation> nation = nothing );
  Player const& player( maybe<e_nation> nation = nothing ) const;

  // ------------------------------------------------------------
  // Revolution Status.
  // ------------------------------------------------------------
  void declare_independence( maybe<e_nation> nation = nothing );

  // ------------------------------------------------------------
  // Serialized State / Transient State / Planes.
  // ------------------------------------------------------------
  FormatVersion& version();
  SettingsState& settings();
  EventsState&   events();
  UnitsState&    units();
  PlayersState&  players();
  TurnState&     turn();
  ColoniesState& colonies();
  NativesState&  natives();
  LandViewState& land_view();
  TerrainState&  terrain();

  FormatVersion const& version() const;
  SettingsState const& settings() const;
  EventsState const&   events() const;
  UnitsState const&    units() const;
  PlayersState const&  players() const;
  TurnState const&     turn() const;
  ColoniesState const& colonies() const;
  NativesState const&  natives() const;
  LandViewState const& land_view() const;
  TerrainState const&  terrain() const;

  RootState&       root();
  RootState const& root() const;

  SS&            ss();
  SSConst const& ss() const;
  SS&            ss_saved();

  TerrainConnectivity const& connectivity() const;
  TerrainConnectivity&       connectivity();

  // These will initialize their respective objects the first
  // time they are called, so they should always be used.
  Planes&            planes();
  lua::state&        lua();
  MockIGui&          gui();
  MockIRand&         rand();
  MockICombat&       combat();
  MockIColonyViewer& colony_viewer();
  TS&                ts();

  IMapUpdater& map_updater() { return *map_updater_; }

  // Run lua_init. This will load all of the lua modules; should
  // only be done when needed. It is very bad to have many test
  // cases calling this.
  void expensive_run_lua_init();

  // FIXME: get rid of this once we figure out how to deal with
  // initializing lua's TS global properly.
  void initialize_ts();

 private:
  e_nation default_nation_ = e_nation::dutch;

  // These are unique_ptrs so that we can forward declare them.
  // Otherwise every unit test would have to pull in all of these
  // headers.
  std::unique_ptr<SS>                  ss_;
  std::unique_ptr<SSConst const>       ss_const_;
  std::unique_ptr<SS>                  ss_saved_;
  std::unique_ptr<TerrainConnectivity> connectivity_;
  std::unique_ptr<IMapUpdater>         map_updater_;
  // These should not be accessed directly since they are ini-
  // tially nullptr.
  std::unique_ptr<Planes>      uninitialized_planes_;
  std::unique_ptr<lua::state>  uninitialized_lua_;
  std::unique_ptr<MockIGui>    uninitialized_gui_;
  std::unique_ptr<MockIRand>   uninitialized_rand_;
  std::unique_ptr<MockICombat> uninitialized_combat_;
  std::unique_ptr<MockIColonyViewer>
                      uninitialized_colony_viewer_;
  std::unique_ptr<TS> uninitialized_ts_;
};

} // namespace rn::testing
