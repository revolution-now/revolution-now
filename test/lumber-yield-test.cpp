/****************************************************************
**lumber-yield.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-20.
*
* Description: Unit tests for the src/lumber-yield.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/lumber-yield.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/unit-composer.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {}

  void initialize( e_unit_type pioneer_type, e_terrain terrain,
                   Coord plow_loc ) {
    add_player( e_nation::dutch );
    add_player( e_nation::french );
    set_default_player( e_nation::dutch );
    MapSquare const   L = make_terrain( terrain );
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

    BASE_CHECK( pioneer_type == e_unit_type::pioneer ||
                pioneer_type == e_unit_type::hardy_pioneer );
    add_unit_on_map( pioneer_type, plow_loc );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[lumber-yield] pioneer" ) {
  World       W;
  Coord const plow_loc = { .x = 1, .y = 1 };
  W.initialize( e_unit_type::pioneer, e_terrain::conifer,
                plow_loc );
  vector<LumberYield> expected;

  auto f = [&] {
    return lumber_yields( W.ss(), W.default_player(), plow_loc,
                          e_unit_type::pioneer );
  };

  SECTION( "no colonies" ) {
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one colony out of range" ) {
    W.add_colony_with_new_unit( { .x = 4, .y = 4 } );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, wrong nation" ) {
    W.add_colony_with_new_unit( { .x = 2, .y = 2 },
                                e_nation::french );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, filled capacity" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.commodities[e_commodity::lumber] = 100;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 20,
                              .yield_to_add_to_colony = 0 } };
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, empty capacity" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.commodities[e_commodity::lumber] = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 20,
                              .yield_to_add_to_colony = 20 } };
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, empty capacity, same square" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
    colony.commodities[e_commodity::lumber] = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 20,
                              .yield_to_add_to_colony = 20 } };
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, partial capacity" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.commodities[e_commodity::lumber] = 90;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 20,
                              .yield_to_add_to_colony = 10 } };
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, empty capacity, lumber mill" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.buildings[e_colony_building::lumber_mill] = true;
    colony.commodities[e_commodity::lumber]          = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 80,
                              .yield_to_add_to_colony = 80 } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "one colony, empty capacity, lumber mill, different "
      "terrain" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    // Remove forest under colony to make sure that we compute
    // the lumber produced from the square being plowed and not
    // the colony square.
    W.square( { .x = 2, .y = 2 } ).overlay           = nothing;
    colony.buildings[e_colony_building::lumber_mill] = true;
    colony.commodities[e_commodity::lumber]          = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 80,
                              .yield_to_add_to_colony = 80 } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "one colony, empty capacity, lumber mill, broadleaf" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    W.square( plow_loc ).ground = e_ground_terrain::prairie;
    colony.buildings[e_colony_building::lumber_mill] = true;
    colony.commodities[e_commodity::lumber]          = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 60,
                              .yield_to_add_to_colony = 60 } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "one colony, partial capacity/warehouse, lumber mill" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.buildings[e_colony_building::lumber_mill] = true;
    colony.buildings[e_colony_building::warehouse]   = true;
    colony.commodities[e_commodity::lumber]          = 100;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 80,
                              .yield_to_add_to_colony = 80 } };
    REQUIRE( f() == expected );
  }

  SECTION( "two colonies, empty capacity" ) {
    auto [colony1, founder] =
        W.add_colony_with_new_unit( { .x = 3, .y = 3 } );
    colony1.buildings[e_colony_building::lumber_mill] = true;
    colony1.commodities[e_commodity::lumber]          = 0;
    auto [colony2, founder2] =
        W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
    colony2.commodities[e_commodity::lumber] = 0;

    expected = {
        LumberYield{ .colony_id              = 2,
                     .total_yield            = 20,
                     .yield_to_add_to_colony = 20 },
        LumberYield{ .colony_id              = 1,
                     .total_yield            = 80,
                     .yield_to_add_to_colony = 80 },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "two colonies, empty capacity" ) {
    auto [colony1, founder] =
        W.add_colony_with_new_unit( { .x = 3, .y = 3 } );
    colony1.buildings[e_colony_building::lumber_mill] = true;
    colony1.commodities[e_commodity::lumber]          = 0;
    auto [colony2, founder2] =
        W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
    colony2.commodities[e_commodity::lumber] = 0;

    expected = {
        LumberYield{ .colony_id              = 2,
                     .total_yield            = 20,
                     .yield_to_add_to_colony = 20 },
        LumberYield{ .colony_id              = 1,
                     .total_yield            = 80,
                     .yield_to_add_to_colony = 80 },
    };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[lumber-yield] hardy_pioneer" ) {
  World       W;
  Coord const plow_loc = { .x = 1, .y = 1 };
  W.initialize( e_unit_type::hardy_pioneer, e_terrain::conifer,
                plow_loc );
  vector<LumberYield> expected;

  auto f = [&] {
    return lumber_yields( W.ss(), W.default_player(), plow_loc,
                          e_unit_type::hardy_pioneer );
  };

  SECTION( "no colonies" ) {
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one colony out of range" ) {
    W.add_colony_with_new_unit( { .x = 4, .y = 4 } );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, wrong nation" ) {
    W.add_colony_with_new_unit( { .x = 2, .y = 2 },
                                e_nation::french );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, filled capacity" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.commodities[e_commodity::lumber] = 100;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 40,
                              .yield_to_add_to_colony = 0 } };
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, empty capacity" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.commodities[e_commodity::lumber] = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 40,
                              .yield_to_add_to_colony = 40 } };
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, empty capacity, same square" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
    colony.commodities[e_commodity::lumber] = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 40,
                              .yield_to_add_to_colony = 40 } };
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, partial capacity" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.commodities[e_commodity::lumber] = 90;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 40,
                              .yield_to_add_to_colony = 10 } };
    REQUIRE( f() == expected );
  }

  SECTION( "one colony, empty capacity, lumber mill" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.buildings[e_colony_building::lumber_mill] = true;
    colony.commodities[e_commodity::lumber]          = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 160,
                              .yield_to_add_to_colony = 100 } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "one colony, empty capacity, lumber mill, warehouse" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.buildings[e_colony_building::lumber_mill] = true;
    colony.buildings[e_colony_building::warehouse]   = true;
    colony.commodities[e_commodity::lumber]          = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 160,
                              .yield_to_add_to_colony = 160 } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "one colony, empty capacity, lumber mill, broadleaf" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    W.square( plow_loc ).ground = e_ground_terrain::prairie;
    colony.buildings[e_colony_building::lumber_mill] = true;
    colony.commodities[e_commodity::lumber]          = 0;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 120,
                              .yield_to_add_to_colony = 100 } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "one colony, partial capacity/warehouse, lumber mill" ) {
    auto [colony, founder] =
        W.add_colony_with_new_unit( { .x = 2, .y = 2 } );
    colony.buildings[e_colony_building::lumber_mill] = true;
    colony.buildings[e_colony_building::warehouse]   = true;
    colony.commodities[e_commodity::lumber]          = 100;
    expected = { LumberYield{ .colony_id              = 1,
                              .total_yield            = 160,
                              .yield_to_add_to_colony = 100 } };
    REQUIRE( f() == expected );
  }

  SECTION( "two colonies, empty capacity" ) {
    auto [colony1, founder1] =
        W.add_colony_with_new_unit( { .x = 3, .y = 3 } );
    colony1.buildings[e_colony_building::lumber_mill] = true;
    colony1.commodities[e_commodity::lumber]          = 0;
    auto [colony2, founder2] =
        W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
    colony2.commodities[e_commodity::lumber] = 0;

    expected = {
        LumberYield{ .colony_id              = 2,
                     .total_yield            = 40,
                     .yield_to_add_to_colony = 40 },
        LumberYield{ .colony_id              = 1,
                     .total_yield            = 160,
                     .yield_to_add_to_colony = 100 },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "two colonies, empty capacity" ) {
    auto [colony1, founder1] =
        W.add_colony_with_new_unit( { .x = 3, .y = 3 } );
    colony1.buildings[e_colony_building::lumber_mill] = true;
    colony1.commodities[e_commodity::lumber]          = 0;
    auto [colony2, founder2] =
        W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
    colony2.commodities[e_commodity::lumber] = 0;

    expected = {
        LumberYield{ .colony_id              = 2,
                     .total_yield            = 40,
                     .yield_to_add_to_colony = 40 },
        LumberYield{ .colony_id              = 1,
                     .total_yield            = 160,
                     .yield_to_add_to_colony = 100 },
    };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[lumber-yield] best_lumber_yield" ) {
  vector<LumberYield> input;
  maybe<LumberYield>  expected;

  auto f = [&] { return best_lumber_yield( input ); };

  // No inputs.
  input    = {};
  expected = nothing;
  REQUIRE( f() == expected );

  // One empty input.
  input    = { LumberYield{ .colony_id              = 1,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 0 } };
  expected = nothing;
  REQUIRE( f() == expected );

  // Two empty inputs.
  input    = { LumberYield{ .colony_id              = 1,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 0 },
               LumberYield{ .colony_id              = 2,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 0 } };
  expected = nothing;
  REQUIRE( f() == expected );

  // One empty, one non-empty.
  input    = { LumberYield{ .colony_id              = 1,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 0 },
               LumberYield{ .colony_id              = 2,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 1 } };
  expected = LumberYield{ .colony_id              = 2,
                          .total_yield            = 10,
                          .yield_to_add_to_colony = 1 };
  REQUIRE( f() == expected );

  // One empty, one non-empty.
  input    = { LumberYield{ .colony_id              = 1,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 1 },
               LumberYield{ .colony_id              = 2,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 0 } };
  expected = LumberYield{ .colony_id              = 1,
                          .total_yield            = 10,
                          .yield_to_add_to_colony = 1 };
  REQUIRE( f() == expected );

  // Both non-empty.
  input    = { LumberYield{ .colony_id              = 1,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 1 },
               LumberYield{ .colony_id              = 2,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 1 } };
  expected = LumberYield{ .colony_id              = 1,
                          .total_yield            = 10,
                          .yield_to_add_to_colony = 1 };
  REQUIRE( f() == expected );

  // One non-empty.
  input    = { LumberYield{ .colony_id              = 1,
                            .total_yield            = 10,
                            .yield_to_add_to_colony = 1 } };
  expected = LumberYield{ .colony_id              = 1,
                          .total_yield            = 10,
                          .yield_to_add_to_colony = 1 };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
