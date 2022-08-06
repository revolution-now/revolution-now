/****************************************************************
**plow.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-27.
*
* Description: Unit tests for the src/plow.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/plow.hpp"

// Revolution Now
#include "src/map-square.hpp"
#include "src/map-updater.hpp"
#include "src/on-map.hpp"
#include "src/terrain.hpp"
#include "src/ustate.hpp"

// ss
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

Coord const kSquare{};

// This will prepare a world with a 1x1 map of the given terrain
// type with one unit on it of the given type.
void prepare_world( TerrainState& terrain_state,
                    UnitsState& units_state, e_terrain terrain,
                    e_unit_type unit_type ) {
  NonRenderingMapUpdater map_updater( terrain_state );
  map_updater.modify_entire_map( [&]( Matrix<MapSquare>& m ) {
    m          = Matrix<MapSquare>( Delta{ .w = 1, .h = 1 } );
    m[kSquare] = map_square_for_terrain( terrain );
  } );
  UnitComposition comp = UnitComposition::create( unit_type );
  UnitId          id =
      create_free_unit( units_state, e_nation::english, comp );
  CHECK( id == 1 );
  unit_to_map_square_non_interactive( units_state, map_updater,
                                      id, kSquare );
}

TEST_CASE( "[plow] can_irrigate" ) {
  MapSquare square;
  bool      expected;

  square = MapSquare{
      .surface         = e_surface::water,
      .ground_resource = e_natural_resource::fish,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .road    = true,
  };
  expected = true;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::forest,
      .road    = true,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::hills,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::savannah,
      .overlay = e_land_overlay::mountains,
  };
  expected = false;
  REQUIRE( can_irrigate( square ) == expected );

  square = MapSquare{
      .surface = e_surface::land,
      .ground  = e_ground_terrain::arctic,
  };
  expected = true;
  REQUIRE( can_irrigate( square ) == expected );
}

TEST_CASE( "[src/plow] plow_square with 40 tools" ) {
  TerrainState           terrain_state;
  NonRenderingMapUpdater map_updater( terrain_state );
  UnitsState             units_state;
  prepare_world( terrain_state, units_state, e_terrain::conifer,
                 e_unit_type::pioneer );

  UnitId           id       = 1;
  Unit&            unit     = units_state.unit_for( id );
  Coord            location = units_state.coord_for( id );
  MapSquare const& square   = terrain_state.square_at( kSquare );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Take away most of the units tools.
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );

  // Before starting plowing work.
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == true );
  REQUIRE( has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == false );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (clearing) work.
  unit.plow();
  unit.set_turns_worked( 0 );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == true );
  REQUIRE( has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == false );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::plow );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  int const kClearTurnsRequired = 6;

  // Do the work.
  for( int i = 0; i < kClearTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_plow_work( units_state, terrain_state, map_updater,
                       unit );
    REQUIRE( can_plow( unit ) == true );
    REQUIRE( can_plow( terrain_state, kSquare ) == true );
    REQUIRE( has_forest( square ) );
    REQUIRE( can_irrigate( square ) == false );
    REQUIRE( can_irrigate( terrain_state, kSquare ) == false );
    REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::plow );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished clearing.
  unit.new_turn();
  perform_plow_work( units_state, terrain_state, map_updater,
                     unit );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (irrigation) work.
  unit.plow();
  unit.set_turns_worked( 0 );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::plow );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  int const kPlowTurnsRequired = 5;

  // Do the work.
  for( int i = 0; i < kPlowTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_plow_work( units_state, terrain_state, map_updater,
                       unit );
    REQUIRE( can_plow( unit ) == true );
    REQUIRE( can_plow( terrain_state, kSquare ) == true );
    REQUIRE( !has_forest( square ) );
    REQUIRE( can_irrigate( square ) == true );
    REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
    REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::plow );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished irrigating.
  unit.new_turn();
  perform_plow_work( units_state, terrain_state, map_updater,
                     unit );
  REQUIRE( can_plow( unit ) == false );
  REQUIRE( can_plow( terrain_state, kSquare ) == false );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == false );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[src/plow] plow_square hardy_pioneer" ) {
  TerrainState           terrain_state;
  NonRenderingMapUpdater map_updater( terrain_state );
  UnitsState             units_state;
  prepare_world( terrain_state, units_state, e_terrain::desert,
                 e_unit_type::hardy_pioneer );

  UnitId           id       = 1;
  Unit&            unit     = units_state.unit_for( id );
  Coord            location = units_state.coord_for( id );
  MapSquare const& square   = terrain_state.square_at( kSquare );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( location == kSquare );

  // Before starting plowing work.
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing work.
  unit.plow();
  unit.set_turns_worked( 0 );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::plow );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  int const kPlowTurnsRequired = 2;

  // Do the work.
  for( int i = 0; i < kPlowTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_plow_work( units_state, terrain_state, map_updater,
                       unit );
    REQUIRE( can_plow( unit ) == true );
    REQUIRE( can_plow( terrain_state, kSquare ) == true );
    REQUIRE( !has_forest( square ) );
    REQUIRE( can_irrigate( square ) == true );
    REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
    REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::plow );
    REQUIRE( unit.composition()[e_unit_inventory::tools] ==
             100 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished clearing.
  unit.new_turn();
  perform_plow_work( units_state, terrain_state, map_updater,
                     unit );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == false );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == false );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 80 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[src/plow] plow_square with cancellation" ) {
  TerrainState           terrain_state;
  NonRenderingMapUpdater map_updater( terrain_state );
  UnitsState             units_state;
  prepare_world( terrain_state, units_state,
                 e_terrain::grassland, e_unit_type::pioneer );

  UnitId           id       = 1;
  Unit&            unit     = units_state.unit_for( id );
  Coord            location = units_state.coord_for( id );
  MapSquare const& square   = terrain_state.square_at( kSquare );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Take away most of the units tools.
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );

  // Before starting plowing work.
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (irrigation) work.
  unit.plow();
  unit.set_turns_worked( 0 );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::plow );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 5;

  // Do some of the work.
  for( int i = 0; i < kTurnsRequired - 2; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_plow_work( units_state, terrain_state, map_updater,
                       unit );
    REQUIRE( can_plow( unit ) == true );
    REQUIRE( can_plow( terrain_state, kSquare ) == true );
    REQUIRE( !has_forest( square ) );
    REQUIRE( can_irrigate( square ) == true );
    REQUIRE( can_irrigate( square ) == true );
    REQUIRE( can_irrigate( terrain_state, kSquare ) == true );
    REQUIRE( has_irrigation( terrain_state, kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::plow );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Effectively cancel it by putting a road on the tile.
  plow_square( terrain_state, map_updater, kSquare );

  // Finished clearing.
  unit.new_turn();
  perform_plow_work( units_state, terrain_state, map_updater,
                     unit );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( terrain_state, kSquare ) == false );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( terrain_state, kSquare ) == false );
  REQUIRE( has_irrigation( terrain_state, kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );
}

} // namespace
} // namespace rn
