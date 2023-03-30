/****************************************************************
**unsentry.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-26.
*
* Description: Unit tests for the src/unsentry.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/unsentry.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/unit-composer.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

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
  World() : Base() {
    add_player( e_nation::english );
    add_player( e_nation::french );
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, L, L, //
        L, L, L, L, L, L, //
        _, L, L, L, L, L, //
        _, L, L, L, L, L, //
        _, L, L, L, L, L, //
        _, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 6 );
  }

  void create_units() {
    // Capital means unsentried, lowercase sentried.
    //   Positions  |  IDs (hex)
    //  . . . . . . | . . . . . .
    //  . e e E . . | . 3 4 5 . .
    //  e . f f F . | 2 . 6 8 9 .
    //  . . f . . . | . . 7 . . .
    //  . . . . f . | . . . . a .
    //  e . . . . . | 1 . . . . .
    //
    // We need to create the units on the map first before sen-
    // try'ing them otherwise, as we place them, existing units
    // will be unsentried.
    Unit& unit1 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 0, .y = 5 }, e_nation::english );
    Unit& unit2 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 0, .y = 2 }, e_nation::english );
    Unit& unit3 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_nation::english );
    Unit& unit4 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 2, .y = 1 }, e_nation::english );
    Unit& unit5 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 3, .y = 1 }, e_nation::english );
    Unit& unit6 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 2, .y = 2 }, e_nation::french );
    Unit& unit7 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 2, .y = 3 }, e_nation::french );
    Unit& unit8 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 3, .y = 2 }, e_nation::french );
    Unit& unit9 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 4, .y = 2 }, e_nation::french );
    Unit& unit10 =
        add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 4, .y = 4 }, e_nation::french );

    unit1.sentry();
    unit2.sentry();
    unit3.sentry();
    unit4.sentry();
    (void)unit5;
    unit6.sentry();
    unit7.sentry();
    unit8.sentry();
    (void)unit9;
    unit10.sentry();

    REQUIRE( unit3.id() == UnitId{ 3 } );
    REQUIRE( unit3.orders().holds<unit_orders::sentry>() );
  }

  bool sentried( UnitId unit_id ) const {
    return units()
        .unit_for( unit_id )
        .orders()
        .holds<unit_orders::sentry>();
  };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[unsentry] unsentry_units_next_to_foreign_units" ) {
  World W;
  W.create_units();
  e_nation nation = {};

  auto f = [&] {
    unsentry_units_next_to_foreign_units( W.ss(), nation );
  };

  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  nation = e_nation::english;
  f();
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( !W.sentried( UnitId{ 3 } ) );
  REQUIRE( !W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  nation = e_nation::english;
  f();
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( !W.sentried( UnitId{ 3 } ) );
  REQUIRE( !W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  nation = e_nation::french;
  f();
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( !W.sentried( UnitId{ 3 } ) );
  REQUIRE( !W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( !W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );
}

TEST_CASE(
    "[unsentry] unsentry_foreign_units_next_to_euro_unit" ) {
  World W;
  W.create_units();

  auto f = [&]( UnitId unit_id ) {
    Unit const& unit = W.units().unit_for( unit_id );
    unsentry_foreign_units_next_to_euro_unit( W.ss(), unit );
  };

  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 1 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 3 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 5 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( !W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 7 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( !W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 6 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( !W.sentried( UnitId{ 3 } ) );
  REQUIRE( !W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( !W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );
}

TEST_CASE( "[unsentry] unsentry_units_next_to_tile" ) {
  World W;
  W.create_units();

  auto f = [&]( UnitId unit_id ) {
    Coord const coord = W.units().coord_for( unit_id );
    unsentry_units_next_to_tile( W.ss(), coord );
  };

  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 1 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 3 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( !W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( !W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 5 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( !W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( !W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( !W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 7 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( !W.sentried( UnitId{ 2 } ) );
  REQUIRE( W.sentried( UnitId{ 3 } ) );
  REQUIRE( !W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( W.sentried( UnitId{ 7 } ) );
  REQUIRE( !W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );

  f( UnitId{ 6 } );
  REQUIRE( W.sentried( UnitId{ 1 } ) );
  REQUIRE( !W.sentried( UnitId{ 2 } ) );
  REQUIRE( !W.sentried( UnitId{ 3 } ) );
  REQUIRE( !W.sentried( UnitId{ 4 } ) );
  REQUIRE( !W.sentried( UnitId{ 5 } ) );
  REQUIRE( !W.sentried( UnitId{ 6 } ) );
  REQUIRE( !W.sentried( UnitId{ 7 } ) );
  REQUIRE( !W.sentried( UnitId{ 8 } ) );
  REQUIRE( !W.sentried( UnitId{ 9 } ) );
  REQUIRE( W.sentried( UnitId{ 10 } ) );
}

} // namespace
} // namespace rn
