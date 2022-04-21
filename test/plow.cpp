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
#include "src/gs-terrain.hpp"
#include "src/gs-units.hpp"
#include "src/map-square.hpp"
#include "src/terrain.hpp"
#include "src/ustate.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

Coord const kSquare( 0_x, 0_y );

// This will prepare a world with a 1x1 map of the given terrain
// type with one unit on it of the given type.
void prepare_world( TerrainState& terrain_state,
                    UnitsState& units_state, e_terrain terrain,
                    e_unit_type unit_type ) {
  terrain_state.mutable_world_map() =
      Matrix<MapSquare>( Delta( 1_w, 1_h ) );
  Matrix<MapSquare>& world_map =
      terrain_state.mutable_world_map();
  world_map[kSquare]   = map_square_for_terrain( terrain );
  UnitComposition comp = UnitComposition::create( unit_type );
  UnitId          id =
      create_unit( units_state, e_nation::english, comp );
  CHECK( id == 1_id );
  units_state.change_to_map( id, kSquare );
}

TEST_CASE( "[src/plow] plow_square with 40 tools" ) {
  TerrainState terrain_state;
  UnitsState   units_state;
  prepare_world( terrain_state, units_state, e_terrain::conifer,
                 e_unit_type::pioneer );

  UnitId           id       = 1_id;
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

  int const kTurnsRequired = 4;

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_plow_work( units_state, terrain_state, unit );
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
  perform_plow_work( units_state, terrain_state, unit );
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

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_plow_work( units_state, terrain_state, unit );
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
  perform_plow_work( units_state, terrain_state, unit );
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

TEST_CASE( "[src/plow] plow_square with cancellation" ) {
  TerrainState terrain_state;
  UnitsState   units_state;
  prepare_world( terrain_state, units_state,
                 e_terrain::grassland, e_unit_type::pioneer );

  UnitId           id       = 1_id;
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

  int const kTurnsRequired = 4;

  // Do some of the work.
  for( int i = 0; i < kTurnsRequired - 2; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_plow_work( units_state, terrain_state, unit );
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
  plow_square( terrain_state, kSquare );

  // Finished clearing.
  unit.new_turn();
  perform_plow_work( units_state, terrain_state, unit );
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
