/****************************************************************
**tribe-evolve-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-24.
*
* Description: Unit tests for the tribe-evolve module.
*
*****************************************************************/
#include "native-enums.rds.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/tribe-evolve.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "src/unit-mgr.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[tribe-evolve] evolve_dwellings_for_tribe: population "
    "growth" ) {
  World W;

  e_tribe tribe_type = {};

  auto f = [&] {
    evolve_dwellings_for_tribe( W.ss(), tribe_type );
  };

  auto unit_count = [&]() -> int {
    return units_for_tribe_unordered( W.ss(), tribe_type )
        .size();
  };

  SECTION( "tupi" ) {
    tribe_type = e_tribe::tupi;
    Dwelling& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, tribe_type );

    REQUIRE( dwelling.population == 3 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 0 );

    dwelling.population = 1;

    f();
    REQUIRE( dwelling.population == 1 );
    REQUIRE( dwelling.growth_counter == 1 );
    REQUIRE( unit_count() == 0 );

    f();
    REQUIRE( dwelling.population == 1 );
    REQUIRE( dwelling.growth_counter == 2 );
    REQUIRE( unit_count() == 0 );

    f();
    REQUIRE( dwelling.population == 1 );
    REQUIRE( dwelling.growth_counter == 3 );
    REQUIRE( unit_count() == 0 );

    for( int i = 1; i <= 16; ++i ) {
      INFO( fmt::format( "i: {}", i ) );
      f();
      REQUIRE( dwelling.population == 1 );
      REQUIRE( dwelling.growth_counter == 3 + i );
      REQUIRE( unit_count() == 0 );
    }

    f();
    REQUIRE( dwelling.population == 1 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );
    NativeUnit const& native_unit =
        W.units().unit_for( NativeUnitId{ 1 } );
    REQUIRE( native_unit.type == e_native_unit_type::brave );
    REQUIRE( native_unit.movement_points == 1 );
    REQUIRE( tribe_type_for_unit( W.ss(), native_unit ) ==
             tribe_type );
    REQUIRE( W.units().coord_for( native_unit.id ) ==
             Coord{ .x = 1, .y = 1 } );

    f();
    REQUIRE( dwelling.population == 1 );
    REQUIRE( dwelling.growth_counter == 1 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 1 );
    REQUIRE( dwelling.growth_counter == 2 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 1 );
    REQUIRE( dwelling.growth_counter == 3 );
    REQUIRE( unit_count() == 1 );

    for( int i = 1; i <= 16; ++i ) {
      INFO( fmt::format( "i: {}", i ) );
      f();
      REQUIRE( dwelling.population == 1 );
      REQUIRE( dwelling.growth_counter == 3 + i );
      REQUIRE( unit_count() == 1 );
    }

    f();
    REQUIRE( dwelling.population == 2 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 2 );
    REQUIRE( dwelling.growth_counter == 2 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 2 );
    REQUIRE( dwelling.growth_counter == 4 );
    REQUIRE( unit_count() == 1 );

    for( int i = 1; i <= 7; ++i ) {
      INFO( fmt::format( "i: {}", i ) );
      f();
      REQUIRE( dwelling.population == 2 );
      REQUIRE( dwelling.growth_counter == 4 + i * 2 );
      REQUIRE( unit_count() == 1 );
    }

    f();
    REQUIRE( dwelling.population == 3 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    // No changes.
    f();
    REQUIRE( dwelling.population == 3 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    dwelling.is_capital = true;
    f();
    REQUIRE( dwelling.population == 3 );
    REQUIRE( dwelling.growth_counter == 3 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 3 );
    REQUIRE( dwelling.growth_counter == 6 );
    REQUIRE( unit_count() == 1 );

    for( int i = 1; i <= 4; ++i ) {
      INFO( fmt::format( "i: {}", i ) );
      f();
      REQUIRE( dwelling.population == 3 );
      REQUIRE( dwelling.growth_counter == 6 + i * 3 );
      REQUIRE( unit_count() == 1 );
    }

    f();
    REQUIRE( dwelling.population == 4 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    // No changes.
    f();
    REQUIRE( dwelling.population == 4 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    // Test behavior when population is above the max.
    dwelling.is_capital = false;
    f();
    REQUIRE( dwelling.population == 4 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    // Test behavior when growth_counter starts positive but no
    // growth is needed.
    dwelling.growth_counter = 10;
    f();
    REQUIRE( dwelling.population == 4 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );
  }

  SECTION( "aztec" ) {
    tribe_type = e_tribe::aztec;
    Dwelling& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, tribe_type );

    REQUIRE( dwelling.population == 7 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 0 );

    dwelling.population = 5;

    f();
    REQUIRE( dwelling.population == 5 );
    REQUIRE( dwelling.growth_counter == 5 );
    REQUIRE( unit_count() == 0 );

    f();
    REQUIRE( dwelling.population == 5 );
    REQUIRE( dwelling.growth_counter == 10 );
    REQUIRE( unit_count() == 0 );

    f();
    REQUIRE( dwelling.population == 5 );
    REQUIRE( dwelling.growth_counter == 15 );
    REQUIRE( unit_count() == 0 );

    f();
    REQUIRE( dwelling.population == 5 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );
    NativeUnit const& native_unit =
        W.units().unit_for( NativeUnitId{ 1 } );
    REQUIRE( native_unit.type == e_native_unit_type::brave );
    REQUIRE( native_unit.movement_points == 1 );
    REQUIRE( tribe_type_for_unit( W.ss(), native_unit ) ==
             tribe_type );
    REQUIRE( W.units().coord_for( native_unit.id ) ==
             Coord{ .x = 1, .y = 1 } );

    f();
    REQUIRE( dwelling.population == 5 );
    REQUIRE( dwelling.growth_counter == 5 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 5 );
    REQUIRE( dwelling.growth_counter == 10 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 5 );
    REQUIRE( dwelling.growth_counter == 15 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 6 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 6 );
    REQUIRE( dwelling.growth_counter == 6 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 6 );
    REQUIRE( dwelling.growth_counter == 12 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 6 );
    REQUIRE( dwelling.growth_counter == 18 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 7 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    // No changes.
    f();
    REQUIRE( dwelling.population == 7 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    dwelling.is_capital = true;
    f();
    REQUIRE( dwelling.population == 7 );
    REQUIRE( dwelling.growth_counter == 7 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 7 );
    REQUIRE( dwelling.growth_counter == 14 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 8 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 8 );
    REQUIRE( dwelling.growth_counter == 8 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 8 );
    REQUIRE( dwelling.growth_counter == 16 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 9 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 9 );
    REQUIRE( dwelling.growth_counter == 9 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 9 );
    REQUIRE( dwelling.growth_counter == 18 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 10 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    // No changes.
    f();
    REQUIRE( dwelling.population == 10 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    f();
    REQUIRE( dwelling.population == 10 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    // Test behavior when population is above the max.
    dwelling.is_capital = false;
    f();
    REQUIRE( dwelling.population == 10 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );

    // Test behavior when growth_counter starts positive but no
    // growth is needed.
    dwelling.growth_counter = 10;
    f();
    REQUIRE( dwelling.population == 10 );
    REQUIRE( dwelling.growth_counter == 0 );
    REQUIRE( unit_count() == 1 );
  }
}

} // namespace
} // namespace rn
