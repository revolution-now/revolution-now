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
#include "src/unit-mgr.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

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
  World() : Base() { add_default_player(); }

  void initialize_map( MapSquare const& L ) {
    vector<MapSquare> tiles{
        // clang-format off
        L,L,L,L,L,L,L,
        L,L,L,L,L,L,L,
        L,L,L,L,L,L,L,
        L,L,L,L,L,L,L,
        L,L,L,L,L,L,L,
        L,L,L,L,L,L,L,
        L,L,L,L,L,L,L,
        // clang-format on
    };
    build_map( std::move( tiles ), 7 );
  }

  void initialize_default() {
    initialize_map( make_grassland() );
  }

  void initialize( e_unit_type unit_type, e_terrain terrain ) {
    MapSquare const L = make_terrain( terrain );
    initialize_map( L );
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

TEST_CASE( "[plow] plow_square with 40 tools" ) {
  World W;
  W.initialize( e_unit_type::pioneer, e_terrain::conifer );

  UnitId           id{ 1 };
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
  REQUIRE( unit.orders() ==
           unit_orders_t{ unit_orders::none{} } );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (clearing) work.
  unit.orders() = unit_orders::plow{ .turns_worked = 0 };
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( has_forest( square ) );
  REQUIRE( can_irrigate( square ) == false );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == false );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.orders() == unit_orders_t{ unit_orders::plow{
                                .turns_worked = 0 } } );
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
    REQUIRE( unit.orders() == unit_orders_t{ unit_orders::plow{
                                  .turns_worked = i + 1 } } );
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
  REQUIRE( unit.orders() ==
           unit_orders_t{ unit_orders::none{} } );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (irrigation) work.
  unit.orders() = unit_orders::plow{ .turns_worked = 0 };
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.orders() == unit_orders_t{ unit_orders::plow{
                                .turns_worked = 0 } } );
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
    REQUIRE( unit.orders() == unit_orders_t{ unit_orders::plow{
                                  .turns_worked = i + 1 } } );
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
  REQUIRE( unit.orders() ==
           unit_orders_t{ unit_orders::none{} } );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[plow] plow_square hardy_pioneer" ) {
  World W;
  W.initialize( e_unit_type::hardy_pioneer, e_terrain::desert );

  UnitId           id{ 1 };
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
  REQUIRE( unit.orders() ==
           unit_orders_t{ unit_orders::none{} } );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing work.
  unit.orders() = unit_orders::plow{ .turns_worked = 0 };
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.orders() == unit_orders_t{ unit_orders::plow{
                                .turns_worked = 0 } } );
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
    REQUIRE( unit.orders() == unit_orders_t{ unit_orders::plow{
                                  .turns_worked = i + 1 } } );
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
  REQUIRE( unit.orders() ==
           unit_orders_t{ unit_orders::none{} } );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 80 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[plow] plow_square with cancellation" ) {
  World W;
  W.initialize( e_unit_type::pioneer, e_terrain::grassland );

  UnitId           id{ 1 };
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
  REQUIRE( unit.orders() ==
           unit_orders_t{ unit_orders::none{} } );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start plowing (irrigation) work.
  unit.orders() = unit_orders::plow{ .turns_worked = 0 };
  REQUIRE( can_plow( unit ) == true );
  REQUIRE( can_plow( W.terrain(), kSquare ) == true );
  REQUIRE( !has_forest( square ) );
  REQUIRE( can_irrigate( square ) == true );
  REQUIRE( can_irrigate( W.terrain(), kSquare ) == true );
  REQUIRE( has_irrigation( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.orders() == unit_orders_t{ unit_orders::plow{
                                .turns_worked = 0 } } );
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
    REQUIRE( unit.orders() == unit_orders_t{ unit_orders::plow{
                                  .turns_worked = i + 1 } } );
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
  REQUIRE( unit.orders() ==
           unit_orders_t{ unit_orders::none{} } );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 40 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[plow] lumber yield / pioneer" ) {
  // Here we're not going to test in detail the algorithm for de-
  // termining the location/quantity of the yield since that is
  // done separately in the lumber-yield module. We're just going
  // to run through a basic scenario to make sure that they yield
  // is being applied at all.
  //
  //   L,L,L,L,L,L,L,
  //   L,L,L,L,L,L,L,
  //   L,L,L,L,L,L,L,
  //   L,L,L,L,L,L,L,
  //   L,L,L,L,L,L,L,
  //   L,L,L,L,L,L,L,
  //   L,L,L,L,L,L,L,
  SECTION( "no colonies" ) {
    World W;
    W.initialize( e_unit_type::pioneer, e_terrain::scrub );
    UnitId id{ 1 };
    Unit&  unit = W.units().unit_for( id );

    int const kClearTurnsRequired = 6;
    int const kPlowTurnsRequired  = 5;

    // Tell unit to start clearing work.
    unit.orders() = unit_orders::plow{ .turns_worked = 0 };

    // Do the work.
    for( int i = 0; i < kClearTurnsRequired; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      unit.new_turn( W.default_player() );
      PlowResult_t const plow_result = perform_plow_work(
          W.ss(), W.default_player(), W.map_updater(), unit );
      REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    }

    // Finished clearing.
    unit.new_turn( W.default_player() );
    PlowResult_t plow_result = perform_plow_work(
        W.ss(), W.default_player(), W.map_updater(), unit );
    REQUIRE( plow_result ==
             PlowResult_t{ PlowResult::cleared_forest{
                 .yield = nothing } } );

    // Tell unit to start plowing work.
    unit.orders() = unit_orders::plow{ .turns_worked = 0 };

    // Do the work.
    for( int i = 0; i < kPlowTurnsRequired; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      unit.new_turn( W.default_player() );
      PlowResult_t const plow_result = perform_plow_work(
          W.ss(), W.default_player(), W.map_updater(), unit );
      REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    }

    // Finished plowing.
    unit.new_turn( W.default_player() );
    plow_result = perform_plow_work( W.ss(), W.default_player(),
                                     W.map_updater(), unit );
    REQUIRE( plow_result ==
             PlowResult_t{ PlowResult::irrigated{} } );
  }

  SECTION( "one colony" ) {
    World W;
    W.initialize( e_unit_type::pioneer, e_terrain::scrub );
    UnitId id{ 1 };
    Unit&  unit = W.units().unit_for( id );

    int const kClearTurnsRequired = 6;
    int const kPlowTurnsRequired  = 5;

    W.add_colony_with_new_unit( { .x = 1, .y = 1 } );

    // Tell unit to start clearing work.
    unit.orders() = unit_orders::plow{ .turns_worked = 0 };

    // Do the work.
    for( int i = 0; i < kClearTurnsRequired; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      unit.new_turn( W.default_player() );
      PlowResult_t const plow_result = perform_plow_work(
          W.ss(), W.default_player(), W.map_updater(), unit );
      REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    }

    // Finished clearing.
    unit.new_turn( W.default_player() );
    PlowResult_t plow_result = perform_plow_work(
        W.ss(), W.default_player(), W.map_updater(), unit );
    REQUIRE( plow_result ==
             PlowResult_t{ PlowResult::cleared_forest{
                 .yield = LumberYield{
                     .colony_id              = 1,
                     .total_yield            = 20,
                     .yield_to_add_to_colony = 20 } } } );

    // Tell unit to start plowing work.
    unit.orders() = unit_orders::plow{ .turns_worked = 0 };

    // Do the work.
    for( int i = 0; i < kPlowTurnsRequired; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      unit.new_turn( W.default_player() );
      PlowResult_t const plow_result = perform_plow_work(
          W.ss(), W.default_player(), W.map_updater(), unit );
      REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    }

    // Finished plowing.
    unit.new_turn( W.default_player() );
    plow_result = perform_plow_work( W.ss(), W.default_player(),
                                     W.map_updater(), unit );
    REQUIRE( plow_result ==
             PlowResult_t{ PlowResult::irrigated{} } );
  }

  SECTION(
      "two colonies in range, one with lumber mill, hardy "
      "pioneer" ) {
    World W;
    W.initialize( e_unit_type::hardy_pioneer,
                  e_terrain::conifer );
    UnitId id{ 1 };
    Unit&  unit = W.units().unit_for( id );

    int const kClearTurnsRequired = 3;
    int const kPlowTurnsRequired  = 2;

    W.add_colony_with_new_unit( { .x = 0, .y = 0 } );
    Colony& with_lumber_mill =
        W.add_colony_with_new_unit( { .x = 1, .y = 2 } );
    with_lumber_mill.buildings[e_colony_building::lumber_mill] =
        true;
    // Give this one just enough lumber so that the one that also
    // has a lumber mill (but is too far) would normally be pre-
    // ferred were it not too far.
    with_lumber_mill.commodities[e_commodity::lumber] = 20;
    // Should be too far.
    Colony& with_lumber_mill_too_far =
        W.add_colony_with_new_unit( { .x = 4, .y = 0 } );
    with_lumber_mill_too_far
        .buildings[e_colony_building::lumber_mill] = true;
    BASE_CHECK( W.ss().colonies.all().size() == 3 );

    // Tell unit to start clearing work.
    unit.orders() = unit_orders::plow{ .turns_worked = 0 };

    // Do the work.
    for( int i = 0; i < kClearTurnsRequired; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      unit.new_turn( W.default_player() );
      PlowResult_t const plow_result = perform_plow_work(
          W.ss(), W.default_player(), W.map_updater(), unit );
      REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    }

    // Finished clearing.
    unit.new_turn( W.default_player() );
    PlowResult_t plow_result = perform_plow_work(
        W.ss(), W.default_player(), W.map_updater(), unit );
    REQUIRE( plow_result ==
             PlowResult_t{ PlowResult::cleared_forest{
                 .yield = LumberYield{
                     .colony_id              = 2,
                     .total_yield            = 160,
                     .yield_to_add_to_colony = 80 } } } );

    // Tell unit to start plowing work.
    unit.orders() = unit_orders::plow{ .turns_worked = 0 };

    // Do the work.
    for( int i = 0; i < kPlowTurnsRequired; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      unit.new_turn( W.default_player() );
      PlowResult_t const plow_result = perform_plow_work(
          W.ss(), W.default_player(), W.map_updater(), unit );
      REQUIRE( plow_result.holds<PlowResult::ongoing>() );
    }

    // Finished plowing.
    unit.new_turn( W.default_player() );
    plow_result = perform_plow_work( W.ss(), W.default_player(),
                                     W.map_updater(), unit );
    REQUIRE( plow_result ==
             PlowResult_t{ PlowResult::irrigated{} } );
  }
}

TEST_CASE( "[plow] has_pioneer_working" ) {
  World W;
  W.initialize_default();
  Coord coord;

  auto f = [&] { return has_pioneer_working( W.ss(), coord ); };

  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 0 } );
  Unit& pioneer1 = W.add_unit_on_map( e_unit_type::pioneer,
                                      { .x = 2, .y = 0 } );
  Unit& pioneer2 = W.add_unit_on_map(
      UnitType::create( e_unit_type::pioneer,
                        e_unit_type::petty_criminal )
          .value(),
      { .x = 3, .y = 0 } );
  W.add_unit_on_map( e_unit_type::hardy_pioneer,
                     { .x = 4, .y = 0 } );
  Unit& pioneer3 = W.add_unit_on_map( e_unit_type::hardy_pioneer,
                                      { .x = 5, .y = 0 } );
  W.add_unit_on_map( e_unit_type::pioneer, { .x = 5, .y = 0 } );

  coord = { .x = 0, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 1, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 2, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 3, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 4, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 5, .y = 0 };
  REQUIRE_FALSE( f() );

  pioneer1.orders() = unit_orders::plow{};
  pioneer2.orders() = unit_orders::road{};
  pioneer3.orders() = unit_orders::plow{};

  coord = { .x = 0, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 1, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 2, .y = 0 };
  REQUIRE( f() );
  coord = { .x = 3, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 4, .y = 0 };
  REQUIRE_FALSE( f() );
  coord = { .x = 5, .y = 0 };
  REQUIRE( f() );
}

} // namespace
} // namespace rn
