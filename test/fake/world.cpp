/****************************************************************
**world.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-03.
*
* Description: Helper for building fake worlds for unit tests.
*
*****************************************************************/
#include "world.hpp"

// Revolution Now
#include "src/colony-mgr.hpp"
#include "src/gs-root.hpp"
#include "src/harbor-units.hpp"
#include "src/map-updater.hpp"
#include "src/ustate.hpp"

// base
#include "src/base/keyval.hpp"

using namespace std;

namespace rn::testing {

FormatVersion& World::version() { return root().version; }
EventsState&   World::events() { return root().events; }
SettingsState& World::settings() { return root().settings; }
UnitsState&    World::units() { return root().units; }
PlayersState&  World::players() { return root().players; }
TurnState&     World::turn() { return root().turn; }
ColoniesState& World::colonies() { return root().colonies; }
LandViewState& World::land_view() { return root().land_view; }
TerrainState&  World::terrain() { return root().zzz_terrain; }

FormatVersion const& World::version() const {
  return root().version;
}
EventsState const& World::events() const {
  return root().events;
}
SettingsState const& World::settings() const {
  return root().settings;
}
UnitsState const&   World::units() const { return root().units; }
PlayersState const& World::players() const {
  return root().players;
}
TurnState const&     World::turn() const { return root().turn; }
ColoniesState const& World::colonies() const {
  return root().colonies;
}
LandViewState const& World::land_view() const {
  return root().land_view;
}
TerrainState const& World::terrain() const {
  return root().zzz_terrain;
}

MapSquare World::make_ocean() {
  return map_square_for_terrain( e_terrain::ocean );
}

MapSquare World::make_sea_lane() {
  MapSquare S = map_square_for_terrain( e_terrain::ocean );
  S.sea_lane  = true;
  return S;
}

MapSquare World::make_grassland() {
  return map_square_for_terrain( e_terrain::grassland );
}

void World::build_map( vector<MapSquare> tiles, W width ) {
  map_updater().modify_entire_map( [&]( Matrix<MapSquare>& m ) {
    m = Matrix<MapSquare>( std::move( tiles ), width );
  } );
}

MapSquare& World::square( gfx::point p ) {
  return terrain().mutable_square_at( Coord::from_gfx( p ) );
}

void World::add_forest( gfx::point p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.overlay = e_land_overlay::forest;
      } );
}

void World::add_mountains( gfx::point p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.overlay = e_land_overlay::mountains;
      } );
}

void World::add_hills( gfx::point p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.overlay = e_land_overlay::hills;
      } );
}

void World::add_road( gfx::point p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ),
      []( MapSquare& square ) { square.road = true; } );
}

void World::add_plow( gfx::point p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ),
      []( MapSquare& square ) { square.irrigation = true; } );
}

void World::add_minor_river( gfx::point p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.river = e_river::minor;
      } );
}

void World::add_major_river( gfx::point p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.river = e_river::major;
      } );
}

UnitId World::add_unit_in_port( e_unit_type     type,
                                maybe<e_nation> nation ) {
  if( !nation ) nation = default_nation_;
  return create_unit_in_harbor( root().units, *nation, type );
}

UnitId World::add_unit_on_map( e_unit_type type, Coord where,
                               maybe<e_nation> nation ) {
  return add_unit_on_map( UnitType::create( type ), where,
                          nation );
}

UnitId World::add_unit_on_map( UnitType const& type, Coord where,
                               maybe<e_nation> nation ) {
  if( !nation ) nation = default_nation_;
  return create_unit_on_map_non_interactive(
      root().units, map_updater(), *nation,
      UnitComposition::create( type ), where );
}

UnitId World::add_unit_in_cargo( e_unit_type type, UnitId holder,
                                 maybe<e_nation> nation ) {
  if( !nation ) nation = default_nation_;
  UnitId held = create_unit( units(), *nation,
                             UnitComposition::create( type ) );
  units().change_to_cargo_somewhere( holder, held );
  return held;
}

UnitId World::add_unit_indoors( ColonyId     colony_id,
                                e_indoor_job indoor_job,
                                e_unit_type  type ) {
  Colony& colony = colonies().colony_for( colony_id );
  Coord   loc    = colonies().coord_for( colony_id );
  UnitId unit_id = add_unit_on_map( type, loc, colony.nation() );
  ColonyJob::indoor job{ .job = indoor_job };
  move_unit_to_colony( units(), colony, unit_id, job );
  return unit_id;
}

void World::ship_to_outbound( UnitId id ) {
  CHECK( units().unit_for( id ).desc().ship );
  UnitHarborViewState new_state{
      .port_status = PortStatus::outbound{ .turns = 0 } };
  units().change_to_harbor_view( id, new_state );
}

UnitId World::add_unit_outdoors( ColonyId      colony_id,
                                 e_direction   d,
                                 e_outdoor_job outdoor_job,
                                 e_unit_type   type ) {
  Colony& colony = colonies().colony_for( colony_id );
  Coord   loc    = colonies().coord_for( colony_id );
  UnitId unit_id = add_unit_on_map( type, loc, colony.nation() );
  ColonyJob::outdoor job{ .direction = d, .job = outdoor_job };
  move_unit_to_colony( units(), colony, unit_id, job );
  return unit_id;
}

void World::add_player( e_nation nation ) {
  root().players.players[nation] = {};
  // This is the minimal amount that we need to set for a player.
  root().players.players[nation].nation = nation;
}

Colony& World::add_colony( UnitId founder ) {
  string name =
      fmt::to_string( colonies().last_colony_id()._ + 1 );
  ColonyId id = found_colony( colonies(), terrain(), units(),
                              founder, map_updater(), name );
  return colonies().colony_for( id );
}

Colony& World::add_colony( Coord           where,
                           maybe<e_nation> nation ) {
  string name =
      fmt::to_string( colonies().last_colony_id()._ + 1 );
  return colonies().colony_for( create_empty_colony(
      colonies(), nation.value_or( default_nation_ ), where,
      name ) );
}

Colony& World::add_colony_with_new_unit(
    Coord where, maybe<e_nation> nation ) {
  if( !nation ) nation = default_nation_;
  UnitId founder = add_unit_on_map( e_unit_type::free_colonist,
                                    where, *nation );
  return add_colony( founder );
}

Player& World::dutch() {
  CHECK( root().players.players.contains( e_nation::dutch ) );
  return root().players.players[e_nation::dutch];
}

Player& World::english() {
  CHECK( root().players.players.contains( e_nation::english ) );
  return root().players.players[e_nation::english];
}

Player& World::spanish() {
  CHECK( root().players.players.contains( e_nation::spanish ) );
  return root().players.players[e_nation::spanish];
}

Player& World::french() {
  CHECK( root().players.players.contains( e_nation::french ) );
  return root().players.players[e_nation::french];
}

Player const& World::dutch() const {
  UNWRAP_CHECK( player, base::lookup( root().players.players,
                                      e_nation::dutch ) );
  return player;
}

Player const& World::english() const {
  UNWRAP_CHECK( player, base::lookup( root().players.players,
                                      e_nation::english ) );
  return player;
}

Player const& World::spanish() const {
  UNWRAP_CHECK( player, base::lookup( root().players.players,
                                      e_nation::spanish ) );
  return player;
}

Player const& World::french() const {
  UNWRAP_CHECK( player, base::lookup( root().players.players,
                                      e_nation::french ) );
  return player;
}

Player& World::default_player() {
  return player( default_nation_ );
}

Player& World::player( maybe<e_nation> nation ) {
  switch( nation.value_or( default_nation_ ) ) {
    case e_nation::dutch: return dutch();
    case e_nation::english: return english();
    case e_nation::french: return french();
    case e_nation::spanish: return spanish();
  }
}

Player const& World::player( maybe<e_nation> nation ) const {
  switch( nation.value_or( default_nation_ ) ) {
    case e_nation::dutch: return dutch();
    case e_nation::english: return english();
    case e_nation::french: return french();
    case e_nation::spanish: return spanish();
  }
}

base::valid_or<string> World::validate_colonies() const {
  for( auto const& [colony_id, colony] : colonies().all() ) {
    valid_or<string> res = colony.validate();
    if( !res.valid() )
      return fmt::format( "colony {} failed validation: {}",
                          colony_id, res.error() );
  }
  return base::valid;
}

World::World()
  : root_( new RootState ),
    map_updater_(
        new NonRenderingMapUpdater( root_->zzz_terrain ) ) {}

World::~World() noexcept = default;

} // namespace rn::testing
