/****************************************************************
**harbor-units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
* Description: Unit tests for the src/harbor-units.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Testing.
#include "test/fake/world.hpp"

// Under test.
#include "src/harbor-units.hpp"

// Revolution Now
#include "src/player.hpp"

// game-state
#include "src/gs/terrain.hpp"
#include "src/gs/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

struct HarborUnitsWorld : testing::World {
  using Base = testing::World;
  HarborUnitsWorld() : Base() {
    MapSquare const O = make_ocean();
    MapSquare const S = make_sea_lane();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
     /*     0  1  2  3  4  5  6  7  8  9  */
     /*0*/  O, O, O, O, O, O, O, O, S, S,
     /*1*/  O, O, O, O, O, O, O, O, S, S,
     /*2*/  O, O, L, L, L, L, L, O, S, S,
     /*3*/  O, O, L, L, L, L, L, O, S, S,
     /*4*/  O, O, L, L, L, L, O, O, S, S,
     /*5*/  O, O, L, L, L, L, O, O, S, S,
     /*6*/  O, O, L, L, L, L, L, O, S, S,
     /*7*/  O, O, O, O, O, O, O, O, S, S,
     /*8*/  O, O, O, O, O, O, O, O, S, S,
     /*9*/  O, O, O, O, O, O, O, O, S, S,
    };
    // clang-format on
    build_map( std::move( tiles ), 10 );
    add_player( e_nation::dutch );
    add_player( e_nation::french );
  }
};

TEST_CASE( "[harbor-units] is_unit_?" ) {
  HarborUnitsWorld w;
  Coord            coord{ .x = 8, .y = 5 };

  UnitId caravel =
      w.add_unit_on_map( e_unit_type::caravel, coord );
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( !is_unit_outbound( w.units(), caravel ) );
  REQUIRE( !is_unit_in_port( w.units(), caravel ) );

  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                       caravel );
  int tries = 0;
  do {
    REQUIRE( is_unit_inbound( w.units(), caravel ) );
    REQUIRE( !is_unit_outbound( w.units(), caravel ) );
    REQUIRE( !is_unit_in_port( w.units(), caravel ) );
    e_high_seas_result res = advance_unit_on_high_seas(
        w.terrain(), w.units(), w.dutch(), caravel );
    if( res == e_high_seas_result::arrived_in_harbor ) break;
    ++tries;
  } while( tries < 10 );
  REQUIRE( tries < 10 );
  REQUIRE( tries >= 1 );
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( !is_unit_outbound( w.units(), caravel ) );
  REQUIRE( is_unit_in_port( w.units(), caravel ) );

  unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                          caravel );
  tries = 0;
  do {
    REQUIRE( !is_unit_inbound( w.units(), caravel ) );
    REQUIRE( is_unit_outbound( w.units(), caravel ) );
    REQUIRE( !is_unit_in_port( w.units(), caravel ) );
    e_high_seas_result res = advance_unit_on_high_seas(
        w.terrain(), w.units(), w.dutch(), caravel );
    if( res == e_high_seas_result::arrived_in_new_world ) break;
    ++tries;
  } while( tries < 10 );
  REQUIRE( tries < 10 );
  REQUIRE( tries >= 1 );
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( is_unit_outbound( w.units(), caravel ) );
  REQUIRE( !is_unit_in_port( w.units(), caravel ) );

  REQUIRE( find_new_world_arrival_square(
               w.units(), w.colonies(), w.terrain(), w.dutch(),
               w.units().harbor_view_state_of( caravel ) ) ==
           coord );
}

TEST_CASE( "[harbor-units] harbor_units_?" ) {
  HarborUnitsWorld w;
  UnitId caravel1 = w.add_unit_in_port( e_unit_type::caravel );
  UnitId caravel2 = w.add_unit_in_port( e_unit_type::caravel );
  UnitId merchantman1 =
      w.add_unit_in_port( e_unit_type::merchantman );
  unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                          merchantman1 );
  UnitId merchantman2 = w.add_unit_on_map(
      e_unit_type::merchantman, Coord{ .x = 8, .y = 5 } );
  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                       merchantman2 );
  UnitId free_colonist1 =
      w.add_unit_in_port( e_unit_type::free_colonist );
  UnitId free_colonist2 =
      w.add_unit_in_port( e_unit_type::free_colonist );
  Coord coord{ .x = 4, .y = 4 };
  w.add_unit_on_map( e_unit_type::soldier, coord );
  w.add_unit_on_map( e_unit_type::soldier, coord );

  REQUIRE( harbor_units_on_dock( w.units(), e_nation::dutch ) ==
           vector<UnitId>{ free_colonist1, free_colonist2 } );
  REQUIRE( harbor_units_in_port( w.units(), e_nation::dutch ) ==
           vector<UnitId>{ caravel1, caravel2 } );
  REQUIRE( harbor_units_inbound( w.units(), e_nation::dutch ) ==
           vector<UnitId>{ merchantman2 } );
  REQUIRE( harbor_units_outbound( w.units(), e_nation::dutch ) ==
           vector<UnitId>{ merchantman1 } );

  REQUIRE( harbor_units_on_dock( w.units(), e_nation::french ) ==
           vector<UnitId>{} );
  REQUIRE( harbor_units_in_port( w.units(), e_nation::french ) ==
           vector<UnitId>{} );
  REQUIRE( harbor_units_inbound( w.units(), e_nation::french ) ==
           vector<UnitId>{} );
  REQUIRE(
      harbor_units_outbound( w.units(), e_nation::french ) ==
      vector<UnitId>{} );
}

TEST_CASE( "[harbor-units] create_unit_in_harbor" ) {
  HarborUnitsWorld w;
  UnitId id1 = create_unit_in_harbor( w.units(), e_nation::dutch,
                                      e_unit_type::soldier );
  UnitId id2 = create_unit_in_harbor(
      w.units(), e_nation::dutch, e_unit_type::free_colonist );
  unordered_map<UnitId, UnitOwnership_t> expected;
  expected[id1] = UnitOwnership::harbor{
      .st = UnitHarborViewState{
          .port_status = PortStatus::in_port{},
          .sailed_from = nothing } };
  expected[id2] = UnitOwnership::harbor{
      .st = UnitHarborViewState{
          .port_status = PortStatus::in_port{},
          .sailed_from = nothing } };
  auto& all = w.units().all();
  REQUIRE( all.size() == 2 );
  REQUIRE( all.contains( id1 ) );
  REQUIRE( all.contains( id2 ) );
  REQUIRE( all.find( id1 )->second.ownership == expected[id1] );
  REQUIRE( all.find( id2 )->second.ownership == expected[id2] );
}

TEST_CASE( "[harbor-units] unit_sail_to_new_world" ) {
  HarborUnitsWorld w;
  UnitId caravel1 = w.add_unit_in_port( e_unit_type::caravel );
  REQUIRE( w.units().harbor_view_state_of( caravel1 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                          caravel1 );
  REQUIRE(
      w.units().harbor_view_state_of( caravel1 ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .turns = 0 } } );
}

TEST_CASE( "[harbor-units] unit_sail_to_harbor" ) {
  HarborUnitsWorld w;
  Coord const      coord{ .x = 8, .y = 5 };
  UnitId           caravel1 =
      w.add_unit_on_map( e_unit_type::caravel, coord );
  REQUIRE( w.units().maybe_harbor_view_state_of( caravel1 ) ==
           nothing );
  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                       caravel1 );
  REQUIRE( w.units().harbor_view_state_of( caravel1 ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 0 },
               .sailed_from = coord } );
}

TEST_CASE( "[harbor-units] unit_move_to_port" ) {
  HarborUnitsWorld w;

  // Add units.
  UnitId caravel1 = w.add_unit_in_port( e_unit_type::caravel );
  UnitId caravel2 = w.add_unit_in_port( e_unit_type::caravel );

  UnitId merchantman1 =
      w.add_unit_in_port( e_unit_type::merchantman );
  unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                          merchantman1 );

  UnitId merchantman2 = w.add_unit_on_map(
      e_unit_type::merchantman, Coord{ .x = 8, .y = 5 } );
  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                       merchantman2 );

  UnitId free_colonist1 =
      w.add_unit_in_port( e_unit_type::free_colonist );
  UnitId free_colonist2 =
      w.add_unit_in_port( e_unit_type::free_colonist );

  Coord  coord{ .x = 4, .y = 4 };
  UnitId soldier1 =
      w.add_unit_on_map( e_unit_type::soldier, coord );
  UnitId soldier2 =
      w.add_unit_on_map( e_unit_type::soldier, coord );

  // Sanity checks.
  REQUIRE( w.units().harbor_view_state_of( caravel1 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( caravel2 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE(
      w.units().harbor_view_state_of( merchantman1 ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .turns = 0 } } );
  REQUIRE( w.units().harbor_view_state_of( merchantman2 ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 0 },
               .sailed_from = Coord{ .x = 8, .y = 5 } } );
  REQUIRE( w.units().harbor_view_state_of( free_colonist1 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( free_colonist2 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().maybe_harbor_view_state_of( soldier1 ) ==
           nothing );
  REQUIRE( w.units().maybe_harbor_view_state_of( soldier2 ) ==
           nothing );

  // Move them to port.
  unit_move_to_port( w.units(), caravel1 );
  unit_move_to_port( w.units(), caravel2 );
  unit_move_to_port( w.units(), merchantman1 );
  unit_move_to_port( w.units(), merchantman2 );
  unit_move_to_port( w.units(), free_colonist1 );
  unit_move_to_port( w.units(), free_colonist2 );
  unit_move_to_port( w.units(), soldier1 );
  unit_move_to_port( w.units(), soldier2 );

  // Verify they are in port.
  REQUIRE( w.units().harbor_view_state_of( caravel1 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( caravel2 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( merchantman1 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( merchantman2 ) ==
           UnitHarborViewState{
               .port_status = PortStatus::in_port{},
               .sailed_from = Coord{ .x = 8, .y = 5 } } );
  REQUIRE( w.units().harbor_view_state_of( free_colonist1 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( free_colonist2 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( soldier1 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( soldier1 ) ==
           UnitHarborViewState{ .port_status =
                                    PortStatus::in_port{} } );
}

TEST_CASE( "[harbor-units] advance_unit_on_high_seas" ) {
  HarborUnitsWorld w;
  Coord            coord{ .x = 8, .y = 5 };

  UnitId id =
      w.add_unit_on_map( e_unit_type::merchantman, coord );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           nothing );

  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(), id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 0 },
               .sailed_from = coord } );

  // Let's turn it around.
  unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                          id );
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::outbound{ .turns = 2 },
               .sailed_from = coord } );
  REQUIRE( find_new_world_arrival_square(
               w.units(), w.colonies(), w.terrain(), w.dutch(),
               w.units().harbor_view_state_of( id ) ) == coord );

  // Sail once again.
  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(), id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 0 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 1 },
               .sailed_from = coord } );

  // Turn around.
  unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                          id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::outbound{ .turns = 1 },
               .sailed_from = coord } );

  // Turn back around and advance again.
  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(), id );
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::arrived_in_harbor );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{ .port_status = PortStatus::in_port{},
                           .sailed_from = coord } );
  REQUIRE( find_new_world_arrival_square(
               w.units(), w.colonies(), w.terrain(), w.dutch(),
               w.units().harbor_view_state_of( id ) ) == coord );

  // Sail back to the new world.
  unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                          id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::outbound{ .turns = 0 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::outbound{ .turns = 1 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::outbound{ .turns = 2 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::outbound{ .turns = 2 },
               .sailed_from = coord } );

  REQUIRE( find_new_world_arrival_square(
               w.units(), w.colonies(), w.terrain(), w.dutch(),
               w.units().harbor_view_state_of( id ) ) == coord );
}

TEST_CASE( "[harbor-units] find_new_world_arrival_square" ) {
  HarborUnitsWorld w;
  Coord const      starting{ .x = 8, .y = 3 };
  Coord const      ship_loc{ .x = 8, .y = 5 };

  w.dutch().starting_position = starting;

  UnitId caravel1 =
      w.add_unit_on_map( e_unit_type::caravel, ship_loc );

  // This unit was never in the old world, so will not have a
  // sailed_from set.  We're going to test where it gets placed
  // when it sails to the new world.
  UnitId caravel2 = w.add_unit_in_port( e_unit_type::caravel );

  SECTION( "no ships sailed" ) {
    unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                            caravel2 );
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.colonies(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of( caravel2 ) ) ==
             starting );
  }
  SECTION( "other ship sailed" ) {
    // This will cause caravel1's map position to be recorded and
    // it should then be used as the position to place caravel2,
    // since caravel2 never had a map position.
    unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                         caravel1 );
    unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                            caravel2 );
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.colonies(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of( caravel2 ) ) ==
             ship_loc );
  }
  SECTION( "did sail from new world" ) {
    unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                         caravel1 );
    unit_move_to_port( w.units(), caravel1 );
    unit_sail_to_new_world( w.terrain(), w.units(), w.dutch(),
                            caravel1 );
    int tries = 0;
    do {
      REQUIRE( !is_unit_inbound( w.units(), caravel1 ) );
      REQUIRE( is_unit_outbound( w.units(), caravel1 ) );
      REQUIRE( !is_unit_in_port( w.units(), caravel1 ) );
      e_high_seas_result res = advance_unit_on_high_seas(
          w.terrain(), w.units(), w.dutch(), caravel1 );
      if( res == e_high_seas_result::arrived_in_new_world )
        break;
      ++tries;
    } while( tries < 10 );
    REQUIRE( tries < 10 );
    REQUIRE(
        w.units().maybe_harbor_view_state_of( caravel1 ) ==
        UnitHarborViewState{
            .port_status = PortStatus::outbound{ .turns = 2 },
            .sailed_from = ship_loc } );
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.colonies(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of( caravel1 ) ) ==
             ship_loc );
  }
}

TEST_CASE(
    "[harbor-units] find_new_world_arrival_square with foreign "
    "unit" ) {
  HarborUnitsWorld w;

  SECTION( "friendly unit" ) {
    Coord const ship_loc{ .x = 8, .y = 5 };
    UnitId      dutch_caravel = w.add_unit_on_map(
             e_unit_type::caravel, ship_loc, e_nation::dutch );
    unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                         dutch_caravel );
    unit_move_to_port( w.units(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    UnitId dutch_caravel2 = w.add_unit_on_map(
        e_unit_type::caravel, ship_loc, e_nation::dutch );
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.colonies(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of(
                     dutch_caravel ) ) == ship_loc );
    REQUIRE( w.units().from_coord( ship_loc ).size() == 1 );
    REQUIRE( w.units()
                 .from_coord( ship_loc )
                 .contains( dutch_caravel2 ) );
  }

  SECTION( "foreign unit" ) {
    Coord const ship_loc{ .x = 8, .y = 5 };
    UnitId      dutch_caravel = w.add_unit_on_map(
             e_unit_type::caravel, ship_loc, e_nation::dutch );
    unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                         dutch_caravel );
    unit_move_to_port( w.units(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    UnitId dutch_caravel2 = w.add_unit_on_map(
        e_unit_type::caravel, ship_loc, e_nation::french );
    REQUIRE( w.units().from_coord( ship_loc ).size() == 1 );
    REQUIRE( w.units()
                 .from_coord( ship_loc )
                 .contains( dutch_caravel2 ) );
    Coord const expected{ .x = 7, .y = 4 };
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.colonies(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of(
                     dutch_caravel ) ) == expected );
  }

  SECTION( "map edge + foreign units" ) {
    Coord const ship_loc{ .x = 0, .y = 0 };
    UnitId      dutch_caravel = w.add_unit_on_map(
             e_unit_type::caravel, ship_loc, e_nation::dutch );
    unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                         dutch_caravel );
    unit_move_to_port( w.units(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                       e_nation::french );
    Coord const expected{ .x = 1, .y = 0 };
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.colonies(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of(
                     dutch_caravel ) ) == expected );
  }

  SECTION( "map edge + a lot foreign units" ) {
    Coord const ship_loc{ .x = 0, .y = 0 };
    UnitId      dutch_caravel = w.add_unit_on_map(
             e_unit_type::caravel, ship_loc, e_nation::dutch );
    unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                         dutch_caravel );
    unit_move_to_port( w.units(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 0 } + Delta{ .w = 0 },
        e_nation::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 0 } + Delta{ .w = 1 },
        e_nation::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 0 } + Delta{ .w = 2 },
        e_nation::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 1 } + Delta{ .w = 0 },
        e_nation::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 1 } + Delta{ .w = 1 },
        e_nation::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 1 } + Delta{ .w = 2 },
        e_nation::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 2 } + Delta{ .w = 0 },
        e_nation::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 2 } + Delta{ .w = 1 },
        e_nation::french );
    Coord const expected{ .x = 3, .y = 0 };
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.colonies(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of(
                     dutch_caravel ) ) == expected );
  }

  SECTION( "no squares available" ) {
    Coord const ship_loc{ .x = 0, .y = 0 };
    UnitId      dutch_caravel = w.add_unit_on_map(
             e_unit_type::caravel, ship_loc, e_nation::dutch );
    unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(),
                         dutch_caravel );
    unit_move_to_port( w.units(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    // Fill all ocean squares with foreign units to test the case
    // where we cannot find any where to place the unit.
    for( Coord c : w.terrain().world_rect_tiles() )
      if( w.terrain().square_at( c ).surface ==
          e_surface::water )
        w.add_unit_on_map( e_unit_type::caravel, c,
                           e_nation::french );

    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.colonies(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of(
                     dutch_caravel ) ) == nothing );
  }
}

TEST_CASE( "[harbor-units] sail west edge" ) {
  HarborUnitsWorld w;
  Coord const      coord{ .x = 0, .y = 0 };
  UnitId id = w.add_unit_on_map( e_unit_type::caravel, coord );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           nothing );
  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(), id );
  REQUIRE( w.units().harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 0 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 1 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 2 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 3 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::arrived_in_harbor );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{ .port_status = PortStatus::in_port{},
                           .sailed_from = coord } );
}

TEST_CASE( "[harbor-units] sail east edge" ) {
  HarborUnitsWorld w;
  Coord const      coord{ .x = 9, .y = 0 };
  UnitId id = w.add_unit_on_map( e_unit_type::caravel, coord );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           nothing );
  unit_sail_to_harbor( w.terrain(), w.units(), w.dutch(), id );
  REQUIRE( w.units().harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 0 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitHarborViewState{
               .port_status = PortStatus::inbound{ .turns = 1 },
               .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.terrain(), w.units(),
                                      w.dutch(), id ) ==
           e_high_seas_result::arrived_in_harbor );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{ .port_status = PortStatus::in_port{},
                           .sailed_from = coord } );
}

} // namespace
} // namespace rn
