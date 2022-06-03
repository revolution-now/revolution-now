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
#include "src/gs-units.hpp"
#include "src/player.hpp"

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
    build_map( std::move( tiles ) );
    add_player( e_nation::dutch );
    add_player( e_nation::french );
  }
};

TEST_CASE( "[harbor-units] is_unit_?" ) {
  HarborUnitsWorld w;
  Coord            coord( 8_x, 5_y );

  UnitId caravel =
      w.add_unit_on_map( e_unit_type::caravel, coord );
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( !is_unit_outbound( w.units(), caravel ) );
  REQUIRE( !is_unit_in_port( w.units(), caravel ) );

  unit_sail_to_harbor( w.units(), w.dutch(), caravel );
  int tries = 0;
  do {
    REQUIRE( is_unit_inbound( w.units(), caravel ) );
    REQUIRE( !is_unit_outbound( w.units(), caravel ) );
    REQUIRE( !is_unit_in_port( w.units(), caravel ) );
    e_high_seas_result res =
        advance_unit_on_high_seas( w.units(), caravel );
    if( res == e_high_seas_result::arrived_in_harbor ) break;
    ++tries;
  } while( tries < 10 );
  REQUIRE( tries < 10 );
  REQUIRE( tries > 1 );
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( !is_unit_outbound( w.units(), caravel ) );
  REQUIRE( is_unit_in_port( w.units(), caravel ) );

  unit_sail_to_new_world( w.units(), caravel );
  tries = 0;
  do {
    REQUIRE( !is_unit_inbound( w.units(), caravel ) );
    REQUIRE( is_unit_outbound( w.units(), caravel ) );
    REQUIRE( !is_unit_in_port( w.units(), caravel ) );
    e_high_seas_result res =
        advance_unit_on_high_seas( w.units(), caravel );
    if( res == e_high_seas_result::arrived_in_new_world ) break;
    ++tries;
  } while( tries < 10 );
  REQUIRE( tries < 10 );
  REQUIRE( tries > 1 );
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( is_unit_outbound( w.units(), caravel ) );
  REQUIRE( !is_unit_in_port( w.units(), caravel ) );

  REQUIRE( find_new_world_arrival_square(
               w.units(), w.terrain(), w.dutch(),
               w.units().harbor_view_state_of( caravel ) ) ==
           coord );
}

TEST_CASE( "[harbor-units] harbor_units_?" ) {
  HarborUnitsWorld w;
  UnitId caravel1 = w.add_unit_in_port( e_unit_type::caravel );
  UnitId caravel2 = w.add_unit_in_port( e_unit_type::caravel );
  UnitId merchantman1 =
      w.add_unit_in_port( e_unit_type::merchantman );
  unit_sail_to_new_world( w.units(), merchantman1 );
  UnitId merchantman2 = w.add_unit_on_map(
      e_unit_type::merchantman, Coord( 8_x, 5_y ) );
  unit_sail_to_harbor( w.units(), w.dutch(), merchantman2 );
  UnitId free_colonist1 =
      w.add_unit_in_port( e_unit_type::free_colonist );
  UnitId free_colonist2 =
      w.add_unit_in_port( e_unit_type::free_colonist );
  Coord coord( 4_x, 4_y );
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
  unit_sail_to_new_world( w.units(), caravel1 );
  REQUIRE( w.units().harbor_view_state_of( caravel1 ) ==
           UnitHarborViewState{
               .port_status =
                   PortStatus::outbound{ .percent = 0.0 } } );
}

TEST_CASE( "[harbor-units] unit_sail_to_harbor" ) {
  HarborUnitsWorld w;
  Coord const      coord( 8_x, 5_y );
  UnitId           caravel1 =
      w.add_unit_on_map( e_unit_type::caravel, coord );
  REQUIRE( w.units().maybe_harbor_view_state_of( caravel1 ) ==
           nothing );
  unit_sail_to_harbor( w.units(), w.dutch(), caravel1 );
  REQUIRE(
      w.units().harbor_view_state_of( caravel1 ) ==
      UnitHarborViewState{
          .port_status = PortStatus::inbound{ .percent = 0.0 },
          .sailed_from = coord } );
}

TEST_CASE( "[harbor-units] unit_move_to_port" ) {
  HarborUnitsWorld w;

  // Add units.
  UnitId caravel1 = w.add_unit_in_port( e_unit_type::caravel );
  UnitId caravel2 = w.add_unit_in_port( e_unit_type::caravel );

  UnitId merchantman1 =
      w.add_unit_in_port( e_unit_type::merchantman );
  unit_sail_to_new_world( w.units(), merchantman1 );

  UnitId merchantman2 = w.add_unit_on_map(
      e_unit_type::merchantman, Coord( 8_x, 5_y ) );
  unit_sail_to_harbor( w.units(), w.dutch(), merchantman2 );

  UnitId free_colonist1 =
      w.add_unit_in_port( e_unit_type::free_colonist );
  UnitId free_colonist2 =
      w.add_unit_in_port( e_unit_type::free_colonist );

  Coord  coord( 4_x, 4_y );
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
  REQUIRE( w.units().harbor_view_state_of( merchantman1 ) ==
           UnitHarborViewState{
               .port_status =
                   PortStatus::outbound{ .percent = 0.0 } } );
  REQUIRE(
      w.units().harbor_view_state_of( merchantman2 ) ==
      UnitHarborViewState{
          .port_status = PortStatus::inbound{ .percent = 0.0 },
          .sailed_from = Coord( 8_x, 5_y ) } );
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
  REQUIRE(
      w.units().harbor_view_state_of( merchantman2 ) ==
      UnitHarborViewState{ .port_status = PortStatus::in_port{},
                           .sailed_from = Coord( 8_x, 5_y ) } );
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
  Coord            coord( 8_x, 5_y );

  UnitId id =
      w.add_unit_on_map( e_unit_type::merchantman, coord );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           nothing );

  unit_sail_to_harbor( w.units(), w.dutch(), id );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::inbound{ .percent = 0.0 },
          .sailed_from = coord } );

  // Let's turn it around.
  unit_sail_to_new_world( w.units(), id );
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 1.0 },
          .sailed_from = coord } );
  REQUIRE( find_new_world_arrival_square(
               w.units(), w.terrain(), w.dutch(),
               w.units().harbor_view_state_of( id ) ) == coord );

  // Sail once again.
  unit_sail_to_harbor( w.units(), w.dutch(), id );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::inbound{ .percent = 0.0 },
          .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::inbound{ .percent = 0.25 },
          .sailed_from = coord } );

  // Turn around.
  unit_sail_to_new_world( w.units(), id );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 0.75 },
          .sailed_from = coord } );

  // Turn back around and advance again.
  unit_sail_to_harbor( w.units(), w.dutch(), id );
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::inbound{ .percent = 0.5 },
          .sailed_from = coord } );

  // Turn around.
  unit_sail_to_new_world( w.units(), id );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 0.5 },
          .sailed_from = coord } );

  // Turn back around and advance again.
  unit_sail_to_harbor( w.units(), w.dutch(), id );
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::inbound{ .percent = 0.75 },
          .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::arrived_in_harbor );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{ .port_status = PortStatus::in_port{},
                           .sailed_from = coord } );
  REQUIRE( find_new_world_arrival_square(
               w.units(), w.terrain(), w.dutch(),
               w.units().harbor_view_state_of( id ) ) == coord );

  // Sail back to the new world.
  unit_sail_to_new_world( w.units(), id );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 0.0 },
          .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 0.25 },
          .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 0.5 },
          .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 0.75 },
          .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 1.0 },
          .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.units(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE(
      w.units().maybe_harbor_view_state_of( id ) ==
      UnitHarborViewState{
          .port_status = PortStatus::outbound{ .percent = 1.0 },
          .sailed_from = coord } );

  REQUIRE( find_new_world_arrival_square(
               w.units(), w.terrain(), w.dutch(),
               w.units().harbor_view_state_of( id ) ) == coord );
}

TEST_CASE( "[harbor-units] find_new_world_arrival_square" ) {
  HarborUnitsWorld w;
  Coord const      starting( 8_x, 3_y );
  Coord const      ship_loc( 8_x, 5_y );

  w.dutch().starting_position = starting;

  UnitId caravel1 =
      w.add_unit_on_map( e_unit_type::caravel, ship_loc );

  // This unit was never in the old world, so will not have a
  // sailed_from set.  We're going to test where it gets placed
  // when it sails to the new world.
  UnitId caravel2 = w.add_unit_in_port( e_unit_type::caravel );

  SECTION( "no ships sailed" ) {
    unit_sail_to_new_world( w.units(), caravel2 );
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of( caravel2 ) ) ==
             starting );
  }
  SECTION( "other ship sailed" ) {
    // This will cause caravel1's map position to be recorded and
    // it should then be used as the position to place caravel2,
    // since caravel2 never had a map position.
    unit_sail_to_harbor( w.units(), w.dutch(), caravel1 );
    unit_sail_to_new_world( w.units(), caravel2 );
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of( caravel2 ) ) ==
             ship_loc );
  }
  SECTION( "did sail from new world" ) {
    unit_sail_to_harbor( w.units(), w.dutch(), caravel1 );
    unit_move_to_port( w.units(), caravel1 );
    unit_sail_to_new_world( w.units(), caravel1 );
    int tries = 0;
    do {
      REQUIRE( !is_unit_inbound( w.units(), caravel1 ) );
      REQUIRE( is_unit_outbound( w.units(), caravel1 ) );
      REQUIRE( !is_unit_in_port( w.units(), caravel1 ) );
      e_high_seas_result res =
          advance_unit_on_high_seas( w.units(), caravel1 );
      if( res == e_high_seas_result::arrived_in_new_world )
        break;
      ++tries;
    } while( tries < 10 );
    REQUIRE( tries < 10 );
    REQUIRE( w.units().maybe_harbor_view_state_of( caravel1 ) ==
             UnitHarborViewState{
                 .port_status =
                     PortStatus::outbound{ .percent = 1.0 },
                 .sailed_from = ship_loc } );
    REQUIRE( find_new_world_arrival_square(
                 w.units(), w.terrain(), w.dutch(),
                 w.units().harbor_view_state_of( caravel1 ) ) ==
             ship_loc );
  }
}

TEST_CASE(
    "[harbor-units] sail to new world with foreign unit" ) {
  HarborUnitsWorld w;
  // TODO
}

} // namespace
} // namespace rn
