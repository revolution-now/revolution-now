/****************************************************************
**commodity.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-31.
*
* Description: Unit tests for commodity module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "src/commodity.hpp"
#include "src/game-state.hpp"
#include "src/gs-units.hpp"
#include "src/ustate.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

TEST_CASE(
    "Commodity move_commodity_as_much_as_possible single "
    "ship" ) {
  auto ship = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::merchantman ) );
  auto food_full  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/100 };
  auto sugar_full = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/100 };
  auto food_part  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/30 };
  auto food_10    = Commodity{ /*type=*/e_commodity::food,
                            /*quantity=*/10 };
  auto sugar_part = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/30 };

  SECTION( "throws" ) {
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 0 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 0 );
    add_commodity_to_cargo( GameState::units(), food_full, ship,
                            0,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 100 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
  }

  SECTION( "one commodity" ) {
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 0 );
    add_commodity_to_cargo( GameState::units(), food_part, ship,
                            0,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    add_commodity_to_cargo( GameState::units(), food_part, ship,
                            1,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 60 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 60 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 1 );
    add_commodity_to_cargo( GameState::units(), food_full, ship,
                            0,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 40 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 40 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    add_commodity_to_cargo( GameState::units(), food_part, ship,
                            1,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 10 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 10 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    add_commodity_to_cargo( GameState::units(), food_10, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
  }

  SECTION( "two commodities" ) {
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 0 );
    add_commodity_to_cargo( GameState::units(), food_part, ship,
                            0,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( GameState::units(), sugar_part, ship,
                            1,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 2,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 2, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 3, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    add_commodity_to_cargo( GameState::units(), sugar_full, ship,
                            2,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 2, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 100 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 2, ship, 3,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 2,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 100 );
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 3 );
  }

  SECTION( "max quantity" ) {
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 0 );
    add_commodity_to_cargo( GameState::units(), food_part, ship,
                            0,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( GameState::units(), sugar_part, ship,
                            1,
                            /*try_other_slots=*/false );
    // | f30 | s30 |    |    |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 0, ship, 1,
                 /*max_quantity=*/40,
                 /*try_other_dst_slots=*/true ) == 30 );
    // |     | s30 | f30  |    |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 2,
                 /*max_quantity=*/10,
                 /*try_other_dst_slots=*/true ) == 10 );
    // |     | s20 | f30  | s10 |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 2, ship, 0,
                 /*max_quantity=*/100,
                 /*try_other_dst_slots=*/true ) == 30 );
    // | f30 | s20 |      | s10 |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 3, ship, 1,
                 /*max_quantity=*/5,
                 /*try_other_dst_slots=*/true ) == 5 );
    // | f30 | s25 |      | s05 |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 3 );
    add_commodity_to_cargo( GameState::units(), sugar_full, ship,
                            2,
                            /*try_other_slots=*/false );
    // | f30 | s25 | s100 | s05 |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 4 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 2, ship, 0,
                 /*max_quantity=*/50,
                 /*try_other_dst_slots=*/true ) == 50 );
    // | f30 | s75 | s50 | s05 |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 4 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 2, ship, 3,
                 /*max_quantity=*/50,
                 /*try_other_dst_slots=*/true ) == 50 );
    // | f30 | s75 |     | s55 |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship, 1, ship, 2,
                 /*max_quantity=*/1,
                 /*try_other_dst_slots=*/true ) == 1 );
    // | f30 | s74 | s01 | s55 |
    REQUIRE( GameState::units()
                 .unit_for( ship )
                 .cargo()
                 .slots_occupied() == 4 );
  }
}

TEST_CASE(
    "Commodity move_commodity_as_much_as_possible two "
    "ships" ) {
  auto ship1 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::merchantman ) );
  auto ship2 = create_unit(
      GameState::units(), e_nation::english,
      UnitType::create( e_unit_type::merchantman ) );
  auto sugar_full = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/100 };
  auto food_part  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/30 };
  auto sugar_part = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/30 };

  SECTION( "no max quantity" ) {
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 0 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 0 );
    add_commodity_to_cargo( GameState::units(), food_part, ship1,
                            1,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( GameState::units(), sugar_part,
                            ship2, 0,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship1, 1, ship2, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 0 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship2, 1, ship1, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 1 );
    add_commodity_to_cargo( GameState::units(), sugar_full,
                            ship1, 1,
                            /*try_other_slots=*/false );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship1, 1, ship2, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 100 );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship2, 0, ship1, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 100 );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship2, 1, ship1, 2,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 0 );
  }

  SECTION( "max quantity" ) {
    // ship1 |     |     |     |     |
    // ship2 |     |     |     |     |
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 0 );
    add_commodity_to_cargo( GameState::units(), food_part, ship1,
                            1,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( GameState::units(), sugar_part,
                            ship2, 0,
                            /*try_other_slots=*/false );
    // ship1 |     | f30 |     |     |
    // ship2 | s30 |     |     |     |
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship1, 1, ship2, 0,
                 /*max_quantity=*/99,
                 /*try_other_dst_slots=*/true ) == 30 );
    // ship1 |     |     |     |     |
    // ship2 | s30 | f30 |     |     |
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 0 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship2, 1, ship1, 0,
                 /*max_quantity=*/5,
                 /*try_other_dst_slots=*/false ) == 5 );
    // ship1 | f05 |     |     |     |
    // ship2 | s30 | f25 |     |     |
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 1 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 2 );
    add_commodity_to_cargo( GameState::units(), sugar_full,
                            ship1, 1,
                            /*try_other_slots=*/false );
    // ship1 | f05 | s100|     |     |
    // ship2 | s30 | f25 |     |     |
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship1, 1, ship2, 0,
                 /*max_quantity=*/80,
                 /*try_other_dst_slots=*/true ) == 80 );
    // ship1 | f05 | s20 |     |     |
    // ship2 | s100| f25 | s10 |     |
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 2 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship2, 0, ship1, 1,
                 /*max_quantity=*/81,
                 /*try_other_dst_slots=*/true ) == 81 );
    // ship1 | f05 | s100| s01 |     |
    // ship2 | s19 | f25 | s10 |     |
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 GameState::units(), ship2, 1, ship1, 3,
                 /*max_quantity=*/25,
                 /*try_other_dst_slots=*/false ) == 25 );
    // ship1 | f05 | s100| s01 | f25 |
    // ship2 | s19 |     | s10 |     |
    REQUIRE( GameState::units()
                 .unit_for( ship1 )
                 .cargo()
                 .slots_occupied() == 4 );
    REQUIRE( GameState::units()
                 .unit_for( ship2 )
                 .cargo()
                 .slots_occupied() == 2 );
  }
}

TEST_CASE( "[commodity] with_quantity" ) {
  Commodity comm{ .type = e_commodity::cloth, .quantity = 50 };
  REQUIRE( comm.quantity == 50 );
  REQUIRE( comm.type == e_commodity::cloth );

  Commodity new_comm = with_quantity( comm, 49 );
  REQUIRE( comm.quantity == 50 );
  REQUIRE( comm.type == e_commodity::cloth );
  REQUIRE( new_comm.quantity == 49 );
  REQUIRE( new_comm.type == e_commodity::cloth );
}

} // namespace
