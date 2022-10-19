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

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "src/imap-updater.hpp"
#include "src/map-square.hpp"
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

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {}

  void initialize( e_unit_type unit_type, e_terrain terrain ) {
    add_default_player();
    MapSquare const   L = make_terrain( terrain );
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );

    add_unit_on_map( unit_type, Coord{} );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
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
  World W;
  W.initialize( e_unit_type::pioneer, e_terrain::conifer );

  UnitId           id       = 1;
  Unit&            unit     = W.units().unit_for( id );
  Coord            location = W.units().coord_for( id );
  MapSquare const& square   = W.square( kSquare );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Take away most of the units tools.
  unit.consume_20_tools( W.default_player() );
  unit.consume_20_tools( W.default_player() );
  unit.consume_20_tools( W.default_player() );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );

  // Before starting plowing work.
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == false );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (clearing) work.
  unit.plow();
  unit.set_turns_worked( 0 );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == false );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::plow );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  int const kClearTurnsRequired = 6;

  // Do the work.
  for( int i = 0; i < kClearTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn( W.default_player() );
    PlowResult_t const plow_result = perform_plow_work(
        W.ss(), W.default_player(), W.map_updater(), unit );
    REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    REQUIRE( can_plow( unit ) == true );
    REQUIRE( can_plow( W.terrain(), kSquare ) == true );
    REQUIRE( has_forest( square ) );
    REQUIRE( can_irrigate( square ) == false );
    REQUIRE( can_irrigate( W.terrain(), kSquare ) == false );
    REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::plow );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished clearing.
  unit.new_turn( W.default_player() );
  PlowResult_t plow_result = perform_plow_work(
      W.ss(), W.default_player(), W.map_updater(), unit );
  REQUIRE( plow_result ==
           PlowResult_t{ PlowResult::cleared_forest{
               .yield = nothing } } );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (irrigation) work.
  unit.plow();
  unit.set_turns_worked( 0 );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::plow );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  int const kPlowTurnsRequired = 5;

  // Do the work.
  for( int i = 0; i < kPlowTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn( W.default_player() );
    PlowResult_t const plow_result = perform_plow_work(
        W.ss(), W.default_player(), W.map_updater(), unit );
    REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    REQUIRE( can_plow( unit ) == true );
    REQUIRE( can_plow( W.terrain(), kSquare ) == true );
    REQUIRE( !has_forest( square ) );
    REQUIRE( can_irrigate( square ) == true );
    REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
    REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::plow );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished irrigating.
  unit.new_turn( W.default_player() );
  plow_result = perform_plow_work( W.ss(), W.default_player(),
                                   W.map_updater(), unit );
  REQUIRE( plow_result.holds<PlowResult::irrigated>() );
  REQUIRE( can_plow( unit ) == false );
  REQUIRE( can_plow( W.terrain(), kSquare ) == false );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == false );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[src/plow] plow_square hardy_pioneer" ) {
  World W;
  W.initialize( e_unit_type::hardy_pioneer, e_terrain::desert );

  UnitId           id       = 1;
  Unit&            unit     = W.units().unit_for( id );
  Coord            location = W.units().coord_for( id );
  MapSquare const& square   = W.terrain().square_at( kSquare );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( location == kSquare );

  // Before starting plowing work.
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing work.
  unit.plow();
  unit.set_turns_worked( 0 );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::plow );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  int const kPlowTurnsRequired = 2;

  // Do the work.
  for( int i = 0; i < kPlowTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn( W.default_player() );
    PlowResult_t const plow_result = perform_plow_work(
        W.ss(), W.default_player(), W.map_updater(), unit );
    REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    REQUIRE( can_plow( unit ) == true );
    REQUIRE( can_plow( W.terrain(), kSquare ) == true );
    REQUIRE( !has_forest( square ) );
    REQUIRE( can_irrigate( square ) == true );
    REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
    REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::plow );
    REQUIRE( unit.composition()[e_unit_inventory::tools] ==
             100 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished irrigating.
  unit.new_turn( W.default_player() );
  PlowResult_t const plow_result = perform_plow_work(
      W.ss(), W.default_player(), W.map_updater(), unit );
  REQUIRE( plow_result.holds<PlowResult::irrigated>() );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == false );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == false );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 80 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[src/plow] plow_square with cancellation" ) {
  World W;
  W.initialize( e_unit_type::pioneer, e_terrain::grassland );

  UnitId           id       = 1;
  Unit&            unit     = W.units().unit_for( id );
  Coord            location = W.units().coord_for( id );
  MapSquare const& square   = W.terrain().square_at( kSquare );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Take away most of the units tools.
  unit.consume_20_tools( W.default_player() );
  unit.consume_20_tools( W.default_player() );
  unit.consume_20_tools( W.default_player() );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );

  // Before starting plowing work.
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (irrigation) work.
  unit.plow();
  unit.set_turns_worked( 0 );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::plow );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 5;

  // Do some of the work.
  for( int i = 0; i < kTurnsRequired - 2; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn( W.default_player() );
    PlowResult_t const plow_result = perform_plow_work(
        W.ss(), W.default_player(), W.map_updater(), unit );
    REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    REQUIRE( can_plow( unit ) == true );
    REQUIRE( can_plow( W.terrain(), kSquare ) == true );
    REQUIRE( !has_forest( square ) );
    REQUIRE( can_irrigate( square ) == true );
    REQUIRE( can_irrigate( square ) == true );
    REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
    REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::plow );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Effectively cancel it by putting a road on the tile.
  plow_square( W.terrain(), W.map_updater(), kSquare );

  // Finished irrigating.
  unit.new_turn( W.default_player() );
  PlowResult_t const plow_result = perform_plow_work(
      W.ss(), W.default_player(), W.map_updater(), unit );
  REQUIRE( plow_result.holds<PlowResult::cancelled>() );
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == false );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == false );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );
}

} // namespace
} // namespace rn
