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
#include "src/unit-mgr.hpp"

// config
#include "src/config/tile-enum.rds.hpp"

// ss
#include "src/ss/units.hpp"

// Testing
#include "test/fake/world.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;
using namespace rn;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_player::dutch );
    MapSquare const O = make_ocean();
    vector<MapSquare> tiles{ O };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "Commodity move_commodity_as_much_as_possible single "
    "ship" ) {
  World W;
  UnitId ship_id =
      W.add_unit_on_map( e_unit_type::merchantman, Coord{} )
          .id();
  CargoHold& ship = W.units().unit_for( ship_id ).cargo();
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
    REQUIRE( ship.slots_occupied() == 0 );
    REQUIRE( ship.slots_occupied() == 0 );
    add_commodity_to_cargo( W.units(), food_full, ship, 0,
                            /*try_other_slots=*/false );
    REQUIRE( ship.slots_occupied() == 1 );
    REQUIRE( ship.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 100 );
    REQUIRE( ship.slots_occupied() == 1 );
  }

  SECTION( "one commodity" ) {
    REQUIRE( ship.slots_occupied() == 0 );
    add_commodity_to_cargo( W.units(), food_part, ship, 0,
                            /*try_other_slots=*/false );
    REQUIRE( ship.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( ship.slots_occupied() == 1 );
    REQUIRE( ship.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( ship.slots_occupied() == 1 );
    REQUIRE( ship.slots_occupied() == 1 );
    add_commodity_to_cargo( W.units(), food_part, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( ship.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 60 );
    REQUIRE( ship.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 60 );
    REQUIRE( ship.slots_occupied() == 1 );
    add_commodity_to_cargo( W.units(), food_full, ship, 0,
                            /*try_other_slots=*/false );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 40 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 40 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( ship.slots_occupied() == 2 );
    add_commodity_to_cargo( W.units(), food_part, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 10 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 10 );
    REQUIRE( ship.slots_occupied() == 2 );
    add_commodity_to_cargo( W.units(), food_10, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 0 );
    REQUIRE( ship.slots_occupied() == 2 );
  }

  SECTION( "two commodities" ) {
    REQUIRE( ship.slots_occupied() == 0 );
    add_commodity_to_cargo( W.units(), food_part, ship, 0,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( W.units(), sugar_part, ship, 1,
                            /*try_other_slots=*/false );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 2,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 2, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 3, ship, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( ship.slots_occupied() == 2 );
    add_commodity_to_cargo( W.units(), sugar_full, ship, 2,
                            /*try_other_slots=*/false );
    REQUIRE( ship.slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 2, ship, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 100 );
    REQUIRE( ship.slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 2, ship, 3,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( ship.slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 2,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 100 );
    REQUIRE( ship.slots_occupied() == 3 );
  }

  SECTION( "max quantity" ) {
    REQUIRE( ship.slots_occupied() == 0 );
    add_commodity_to_cargo( W.units(), food_part, ship, 0,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( W.units(), sugar_part, ship, 1,
                            /*try_other_slots=*/false );
    // | f30 | s30 |    |    |
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 0, ship, 1,
                 /*max_quantity=*/40,
                 /*try_other_dst_slots=*/true ) == 30 );
    // |     | s30 | f30  |    |
    REQUIRE( ship.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 2,
                 /*max_quantity=*/10,
                 /*try_other_dst_slots=*/true ) == 10 );
    // |     | s20 | f30  | s10 |
    REQUIRE( ship.slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 2, ship, 0,
                 /*max_quantity=*/100,
                 /*try_other_dst_slots=*/true ) == 30 );
    // | f30 | s20 |      | s10 |
    REQUIRE( ship.slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 3, ship, 1,
                 /*max_quantity=*/5,
                 /*try_other_dst_slots=*/true ) == 5 );
    // | f30 | s25 |      | s05 |
    REQUIRE( ship.slots_occupied() == 3 );
    add_commodity_to_cargo( W.units(), sugar_full, ship, 2,
                            /*try_other_slots=*/false );
    // | f30 | s25 | s100 | s05 |
    REQUIRE( ship.slots_occupied() == 4 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 2, ship, 0,
                 /*max_quantity=*/50,
                 /*try_other_dst_slots=*/true ) == 50 );
    // | f30 | s75 | s50 | s05 |
    REQUIRE( ship.slots_occupied() == 4 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 2, ship, 3,
                 /*max_quantity=*/50,
                 /*try_other_dst_slots=*/true ) == 50 );
    // | f30 | s75 |     | s55 |
    REQUIRE( ship.slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship, 1, ship, 2,
                 /*max_quantity=*/1,
                 /*try_other_dst_slots=*/true ) == 1 );
    // | f30 | s74 | s01 | s55 |
    REQUIRE( ship.slots_occupied() == 4 );
  }
}

TEST_CASE(
    "Commodity move_commodity_as_much_as_possible two "
    "ships" ) {
  World W;
  UnitId const ship1_id =
      W.add_unit_on_map( e_unit_type::merchantman, Coord{} )
          .id();
  UnitId const ship2_id =
      W.add_unit_on_map( e_unit_type::merchantman, Coord{} )
          .id();
  CargoHold& ship1_cargo =
      W.units().unit_for( ship1_id ).cargo();
  CargoHold& ship2_cargo =
      W.units().unit_for( ship2_id ).cargo();
  auto sugar_full = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/100 };
  auto food_part  = Commodity{ /*type=*/e_commodity::food,
                              /*quantity=*/30 };
  auto sugar_part = Commodity{ /*type=*/e_commodity::sugar,
                               /*quantity=*/30 };

  SECTION( "no max quantity" ) {
    REQUIRE( ship1_cargo.slots_occupied() == 0 );
    REQUIRE( ship2_cargo.slots_occupied() == 0 );
    add_commodity_to_cargo( W.units(), food_part, ship1_cargo, 1,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( W.units(), sugar_part, ship2_cargo,
                            0,
                            /*try_other_slots=*/false );
    REQUIRE( ship1_cargo.slots_occupied() == 1 );
    REQUIRE( ship2_cargo.slots_occupied() == 1 );
    REQUIRE( ship1_cargo.slots_occupied() == 1 );
    REQUIRE( ship2_cargo.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship1_cargo, 1, ship2_cargo, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 30 );
    REQUIRE( ship1_cargo.slots_occupied() == 0 );
    REQUIRE( ship2_cargo.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship2_cargo, 1, ship1_cargo, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( ship1_cargo.slots_occupied() == 1 );
    REQUIRE( ship2_cargo.slots_occupied() == 1 );
    add_commodity_to_cargo( W.units(), sugar_full, ship1_cargo,
                            1,
                            /*try_other_slots=*/false );
    REQUIRE( ship1_cargo.slots_occupied() == 2 );
    REQUIRE( ship2_cargo.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship1_cargo, 1, ship2_cargo, 0,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/true ) == 100 );
    REQUIRE( ship1_cargo.slots_occupied() == 1 );
    REQUIRE( ship2_cargo.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship2_cargo, 0, ship1_cargo, 1,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 100 );
    REQUIRE( ship1_cargo.slots_occupied() == 2 );
    REQUIRE( ship2_cargo.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship2_cargo, 1, ship1_cargo, 2,
                 /*max_quantity=*/nothing,
                 /*try_other_dst_slots=*/false ) == 30 );
    REQUIRE( ship1_cargo.slots_occupied() == 3 );
    REQUIRE( ship2_cargo.slots_occupied() == 0 );
  }

  SECTION( "max quantity" ) {
    // ship1 |     |     |     |     |
    // ship2 |     |     |     |     |
    REQUIRE( ship1_cargo.slots_occupied() == 0 );
    add_commodity_to_cargo( W.units(), food_part, ship1_cargo, 1,
                            /*try_other_slots=*/false );
    add_commodity_to_cargo( W.units(), sugar_part, ship2_cargo,
                            0,
                            /*try_other_slots=*/false );
    // ship1 |     | f30 |     |     |
    // ship2 | s30 |     |     |     |
    REQUIRE( ship1_cargo.slots_occupied() == 1 );
    REQUIRE( ship2_cargo.slots_occupied() == 1 );
    REQUIRE( ship1_cargo.slots_occupied() == 1 );
    REQUIRE( ship2_cargo.slots_occupied() == 1 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship1_cargo, 1, ship2_cargo, 0,
                 /*max_quantity=*/99,
                 /*try_other_dst_slots=*/true ) == 30 );
    // ship1 |     |     |     |     |
    // ship2 | s30 | f30 |     |     |
    REQUIRE( ship1_cargo.slots_occupied() == 0 );
    REQUIRE( ship2_cargo.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship2_cargo, 1, ship1_cargo, 0,
                 /*max_quantity=*/5,
                 /*try_other_dst_slots=*/false ) == 5 );
    // ship1 | f05 |     |     |     |
    // ship2 | s30 | f25 |     |     |
    REQUIRE( ship1_cargo.slots_occupied() == 1 );
    REQUIRE( ship2_cargo.slots_occupied() == 2 );
    add_commodity_to_cargo( W.units(), sugar_full, ship1_cargo,
                            1,
                            /*try_other_slots=*/false );
    // ship1 | f05 | s100|     |     |
    // ship2 | s30 | f25 |     |     |
    REQUIRE( ship1_cargo.slots_occupied() == 2 );
    REQUIRE( ship2_cargo.slots_occupied() == 2 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship1_cargo, 1, ship2_cargo, 0,
                 /*max_quantity=*/80,
                 /*try_other_dst_slots=*/true ) == 80 );
    // ship1 | f05 | s20 |     |     |
    // ship2 | s100| f25 | s10 |     |
    REQUIRE( ship1_cargo.slots_occupied() == 2 );
    REQUIRE( ship2_cargo.slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship2_cargo, 0, ship1_cargo, 1,
                 /*max_quantity=*/81,
                 /*try_other_dst_slots=*/true ) == 81 );
    // ship1 | f05 | s100| s01 |     |
    // ship2 | s19 | f25 | s10 |     |
    REQUIRE( ship1_cargo.slots_occupied() == 3 );
    REQUIRE( ship2_cargo.slots_occupied() == 3 );
    REQUIRE( move_commodity_as_much_as_possible(
                 W.units(), ship2_cargo, 1, ship1_cargo, 3,
                 /*max_quantity=*/25,
                 /*try_other_dst_slots=*/false ) == 25 );
    // ship1 | f05 | s100| s01 | f25 |
    // ship2 | s19 |     | s10 |     |
    REQUIRE( ship1_cargo.slots_occupied() == 4 );
    REQUIRE( ship2_cargo.slots_occupied() == 2 );
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

TEST_CASE( "[commodity] tile_for_commodity_16" ) {
  auto const f = []( e_commodity const c ) {
    return tile_for_commodity_16( c );
  };

  using enum e_tile;
  using enum e_commodity;

  REQUIRE( f( food ) == commodity_food_16 );
  REQUIRE( f( sugar ) == commodity_sugar_16 );
  REQUIRE( f( tobacco ) == commodity_tobacco_16 );
  REQUIRE( f( cotton ) == commodity_cotton_16 );
  REQUIRE( f( furs ) == commodity_furs_16 );
  REQUIRE( f( lumber ) == commodity_lumber_16 );
  REQUIRE( f( ore ) == commodity_ore_16 );
  REQUIRE( f( silver ) == commodity_silver_16 );
  REQUIRE( f( horses ) == commodity_horses_16 );
  REQUIRE( f( rum ) == commodity_rum_16 );
  REQUIRE( f( cigars ) == commodity_cigars_16 );
  REQUIRE( f( cloth ) == commodity_cloth_16 );
  REQUIRE( f( coats ) == commodity_coats_16 );
  REQUIRE( f( trade_goods ) == commodity_trade_goods_16 );
  REQUIRE( f( tools ) == commodity_tools_16 );
  REQUIRE( f( muskets ) == commodity_muskets_16 );
}

TEST_CASE( "[commodity] tile_for_commodity_20" ) {
  auto const f = []( e_commodity const c ) {
    return tile_for_commodity_20( c );
  };

  using enum e_tile;
  using enum e_commodity;

  REQUIRE( f( food ) == commodity_food_20 );
  REQUIRE( f( sugar ) == commodity_sugar_20 );
  REQUIRE( f( tobacco ) == commodity_tobacco_20 );
  REQUIRE( f( cotton ) == commodity_cotton_20 );
  REQUIRE( f( furs ) == commodity_furs_20 );
  REQUIRE( f( lumber ) == commodity_lumber_20 );
  REQUIRE( f( ore ) == commodity_ore_20 );
  REQUIRE( f( silver ) == commodity_silver_20 );
  REQUIRE( f( horses ) == commodity_horses_20 );
  REQUIRE( f( rum ) == commodity_rum_20 );
  REQUIRE( f( cigars ) == commodity_cigars_20 );
  REQUIRE( f( cloth ) == commodity_cloth_20 );
  REQUIRE( f( coats ) == commodity_coats_20 );
  REQUIRE( f( trade_goods ) == commodity_trade_goods_20 );
  REQUIRE( f( tools ) == commodity_tools_20 );
  REQUIRE( f( muskets ) == commodity_muskets_20 );
}

} // namespace
} // namespace rn
