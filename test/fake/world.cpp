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

UnitId World::add_unit_in_port( e_unit_type type,
                                e_nation    nation ) {
  return create_unit_in_harbor( root().units, nation, type );
}

UnitId World::add_unit_on_map( e_unit_type type, Coord where,
                               e_nation nation ) {
  return create_unit_on_map_non_interactive(
      root().units, map_updater(), nation,
      UnitComposition::create( type ), where );
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
}

Colony& World::add_colony( UnitId founder ) {
  string name =
      fmt::to_string( colonies().last_colony_id()._ + 1 );
  ColonyId id = found_colony( colonies(), terrain(), units(),
                              founder, map_updater(), name );
  return colonies().colony_for( id );
}

Colony& World::add_colony( Coord where, e_nation nation ) {
  UnitId founder = add_unit_on_map( e_unit_type::free_colonist,
                                    where, nation );
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
