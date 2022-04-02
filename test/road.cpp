/****************************************************************
**road.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-26.
*
* Description: Unit tests for the src/test/road.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/road.hpp"

// Revolution Now
#include "src/gs-terrain.hpp"
#include "src/gs-units.hpp"
#include "src/ustate.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

Coord const kSquare( 0_x, 0_y );

// This will prepare a world with a 1x1 map consisting of a
// single grassland square with one unit on it of the given type.
void prepare_world( TerrainState& terrain_state,
                    UnitsState&   units_state,
                    e_unit_type   unit_type ) {
  terrain_state.world_map = WorldMap( Delta( 1_w, 1_h ) );
  WorldMap& world_map     = terrain_state.world_map;
  world_map[kSquare] =
      map_square_for_terrain( e_terrain::grassland );
  UnitComposition comp = UnitComposition::create( unit_type );
  UnitId          id =
      create_unit( units_state, e_nation::english, comp );
  CHECK( id == 1_id );
  units_state.change_to_map( id, kSquare );
}

TEST_CASE( "[src/road] perform_road_work 100 tools" ) {
  TerrainState terrain_state;
  UnitsState   units_state;
  prepare_world( terrain_state, units_state,
                 e_unit_type::pioneer );

  UnitId id       = 1_id;
  Unit&  unit     = units_state.unit_for( id );
  Coord  location = units_state.coord_for( id );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Before starting road work.
  REQUIRE( has_road( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 4;

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( units_state, terrain_state, unit );
    REQUIRE( has_road( terrain_state, kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] ==
             100 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished.
  unit.new_turn();
  perform_road_work( units_state, terrain_state, unit );
  REQUIRE( has_road( terrain_state, kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 80 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[src/road] perform_road_work 20 tools" ) {
  TerrainState terrain_state;
  UnitsState   units_state;
  prepare_world( terrain_state, units_state,
                 e_unit_type::pioneer );

  UnitId id       = 1_id;
  Unit&  unit     = units_state.unit_for( id );
  Coord  location = units_state.coord_for( id );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Take away most of the units tools.
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();

  // Before starting road work.
  REQUIRE( has_road( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 4;

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( units_state, terrain_state, unit );
    REQUIRE( has_road( terrain_state, kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished.
  unit.new_turn();
  perform_road_work( units_state, terrain_state, unit );
  REQUIRE( has_road( terrain_state, kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE(
    "[src/road] perform_road_work hardy_pioneer 20 tools" ) {
  TerrainState terrain_state;
  UnitsState   units_state;
  prepare_world( terrain_state, units_state,
                 e_unit_type::hardy_pioneer );

  UnitId id       = 1_id;
  Unit&  unit     = units_state.unit_for( id );
  Coord  location = units_state.coord_for( id );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( location == kSquare );

  // Take away most of the units tools.
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();

  // Before starting road work.
  REQUIRE( has_road( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 1;

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( units_state, terrain_state, unit );
    REQUIRE( has_road( terrain_state, kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished.
  unit.new_turn();
  perform_road_work( units_state, terrain_state, unit );
  REQUIRE( has_road( terrain_state, kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::hardy_colonist );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[src/road] perform_road_work with cancel" ) {
  TerrainState terrain_state;
  UnitsState   units_state;
  prepare_world( terrain_state, units_state,
                 e_unit_type::pioneer );

  UnitId id       = 1_id;
  Unit&  unit     = units_state.unit_for( id );
  Coord  location = units_state.coord_for( id );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Before starting road work.
  REQUIRE( has_road( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( terrain_state, kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 4;

  // Do part of the work.
  for( int i = 0; i < kTurnsRequired - 2; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( units_state, terrain_state, unit );
    REQUIRE( has_road( terrain_state, kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] ==
             100 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Effectively cancel it by putting a road on the tile.
  set_road( terrain_state, kSquare );

  // Cancelled.
  unit.new_turn();
  perform_road_work( units_state, terrain_state, unit );
  REQUIRE( has_road( terrain_state, kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );
}

} // namespace
} // namespace rn
