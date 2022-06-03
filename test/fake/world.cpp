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

void World::build_map( vector<MapSquare> tiles ) {
  map_updater().modify_entire_map( [&]( Matrix<MapSquare>& m ) {
    m = Matrix<MapSquare>( std::move( tiles ), 10_w );
  } );
}

UnitId World::add_unit_in_port( e_unit_type type,
                                e_nation    nation ) {
  return create_unit_in_harbor( root().units, nation, type );
}

UnitId World::add_unit_on_map( e_unit_type type, Coord where,
                               e_nation nation ) {
  return create_unit_on_map_no_ui(
      root().units, map_updater(), nation,
      UnitComposition::create( type ), where );
}

void World::add_player( e_nation nation ) {
  root().players.players[nation] = {};
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

World::World()
  : root_( new RootState ),
    map_updater_(
        new NonRenderingMapUpdater( root_->zzz_terrain ) ) {}

World::~World() noexcept = default;

} // namespace rn::testing
