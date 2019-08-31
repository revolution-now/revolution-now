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
#include "src/ownership.hpp"

// Must be last.
#include "catch-common.hpp"

FMT_TO_CATCH( ::rn::UnitId );

namespace {

using namespace std;
using namespace rn;

TEST_CASE(
    "Commodity move_commodity_as_much_as_possible single "
    "ship" ) {
  auto ship =
      create_unit( e_nation::english, e_unit_type::merchantman )
          .id();
  auto food_full  = Commodity{/*type=*/e_commodity::food,
                             /*quantity=*/100};
  auto sugar_full = Commodity{/*type=*/e_commodity::sugar,
                              /*quantity=*/100};
  auto food_part  = Commodity{/*type=*/e_commodity::food,
                             /*quantity=*/30};
  auto food_10    = Commodity{/*type=*/e_commodity::food,
                           /*quantity=*/10};
  auto sugar_part = Commodity{/*type=*/e_commodity::sugar,
                              /*quantity=*/30};

  SECTION( "throws" ) {
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 0 );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 0, ship, 0, /*try_other_dst_slots=*/false ) );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 0, ship, 1, /*try_other_dst_slots=*/false ) );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 10, ship, 0, /*try_other_dst_slots=*/false ) );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 0 );
    add_commodity_to_cargo( food_full, ship, 0,
                            /*try_other_slots=*/false );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 0, ship, 10, /*try_other_dst_slots=*/false ) );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 0, ship, 1,
                 /*try_other_dst_slots=*/false ) == 100 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
  }

  SECTION( "one commodity" ) {
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 0 );
    add_commodity_to_cargo( food_part, ship, 0,
                            /*try_other_slots=*/false );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 0, ship, 1,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 0, ship, 1, /*try_other_dst_slots=*/false ) );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 1, ship, 0,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 1, ship, 0, /*try_other_dst_slots=*/false ) );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    add_commodity_to_cargo( food_part, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 0, ship, 1,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 1, ship, 0,
                 /*try_other_dst_slots=*/false ) == 60 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 0, ship, 1,
                 /*try_other_dst_slots=*/false ) == 60 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 1 );
    add_commodity_to_cargo( food_full, ship, 0,
                            /*try_other_slots=*/false );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 0, ship, 1,
                 /*try_other_dst_slots=*/false ) == 40 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 1, ship, 0,
                 /*try_other_dst_slots=*/false ) == 40 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 1, ship, 0,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    add_commodity_to_cargo( food_part, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 0, ship, 1,
                 /*try_other_dst_slots=*/false ) == 10 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 1, ship, 0,
                 /*try_other_dst_slots=*/false ) == 10 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    add_commodity_to_cargo( food_10, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 0, ship, 1,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 1, ship, 0,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
  }

  SECTION( "two commodities" ) {
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 0 );
    add_commodity_to_cargo( food_part, ship, 0,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( sugar_part, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 0, ship, 1,
        /*try_other_dst_slots=*/false ) );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 1, ship, 0,
        /*try_other_dst_slots=*/false ) );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 0, ship, 1,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 1, ship, 2,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
        ship, 2, ship, 3,
        /*try_other_dst_slots=*/false ) );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 2, ship, 0,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 3, ship, 1,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 2 );
    add_commodity_to_cargo( sugar_full, ship, 2,
                            /*try_other_slots=*/false );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 2, ship, 0,
                 /*try_other_dst_slots=*/true ) == 100 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 2, ship, 3,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 ship, 1, ship, 2,
                 /*try_other_dst_slots=*/true ) == 100 );
    REQUIRE( unit_from_id( ship ).cargo().slots_occupied() == 3 );
  }
}

TEST_CASE(
    "Commodity move_commodity_as_much_as_possible two "
    "ships" ) {
  auto ship1 =
      create_unit( e_nation::english, e_unit_type::merchantman )
          .id();
  auto ship2 =
      create_unit( e_nation::english, e_unit_type::merchantman )
          .id();
  auto sugar_full = Commodity{/*type=*/e_commodity::sugar,
                              /*quantity=*/100};
  auto food_part  = Commodity{/*type=*/e_commodity::food,
                             /*quantity=*/30};
  auto sugar_part = Commodity{/*type=*/e_commodity::sugar,
                              /*quantity=*/30};

  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 0 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 0 );
  add_commodity_to_cargo( food_part, ship1, 1,
                          /*try_other_slots=*/false );
  add_commodity_to_cargo( sugar_part, ship2, 0,
                          /*try_other_slots=*/false );
  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 1 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 1 );
  REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
      ship1, 1, ship2, 0,
      /*try_other_dst_slots=*/false ) );
  REQUIRE_THROWS_AS_RN( move_commodity_as_much_as_possible(
      ship1, 1, ship2, 0,
      /*try_other_dst_slots=*/false ) );
  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 1 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 1 );
  REQUIRE( move_commodity_as_much_as_possible(
               ship1, 1, ship2, 0,
               /*try_other_dst_slots=*/true ) == 30 );
  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 0 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 2 );
  REQUIRE( move_commodity_as_much_as_possible(
               ship2, 1, ship1, 0,
               /*try_other_dst_slots=*/false ) == 30 );
  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 1 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 1 );
  add_commodity_to_cargo( sugar_full, ship1, 1,
                          /*try_other_slots=*/false );
  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 2 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 1 );
  REQUIRE( move_commodity_as_much_as_possible(
               ship1, 1, ship2, 0,
               /*try_other_dst_slots=*/true ) == 100 );
  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 1 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 2 );
  REQUIRE( move_commodity_as_much_as_possible(
               ship2, 0, ship1, 1,
               /*try_other_dst_slots=*/false ) == 100 );
  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 2 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 1 );
  REQUIRE( move_commodity_as_much_as_possible(
               ship2, 1, ship1, 2,
               /*try_other_dst_slots=*/false ) == 30 );
  REQUIRE( unit_from_id( ship1 ).cargo().slots_occupied() == 3 );
  REQUIRE( unit_from_id( ship2 ).cargo().slots_occupied() == 0 );
}

} // namespace
