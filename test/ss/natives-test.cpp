/****************************************************************
**natives.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-21.
*
* Description: Unit tests for the src/ss/natives.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/natives.hpp"

// Testing
#include "test/fake/world.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

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
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        L, L, L, L, L, //
        L, L, L, L, L, //
        L, L, L, L, L, //
        L, L, L, L, L, //
        L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 5 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[ss/natives] dwellings_for_tribe / destroy_dwelling" ) {
  World W;
  W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois );
  W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::iroquois );
  REQUIRE_FALSE( W.natives()
                     .dwellings_for_tribe( e_tribe::cherokee )
                     .has_value() );
  REQUIRE( W.natives()
               .dwellings_for_tribe( e_tribe::iroquois )
               .has_value() );
  REQUIRE(
      W.natives().dwellings_for_tribe( e_tribe::iroquois ) ==
      unordered_set<DwellingId>{ DwellingId{ 1 },
                                 DwellingId{ 2 } } );
  REQUIRE( W.natives()
               .maybe_dwelling_from_coord( { .x = 1, .y = 1 } )
               .has_value() );
  REQUIRE( W.natives()
               .maybe_dwelling_from_coord( { .x = 2, .y = 1 } )
               .has_value() );
  W.natives().destroy_dwelling( DwellingId{ 1 } );
  REQUIRE(
      W.natives().dwellings_for_tribe( e_tribe::iroquois ) ==
      unordered_set<DwellingId>{ DwellingId{ 2 } } );
  REQUIRE_FALSE(
      W.natives()
          .maybe_dwelling_from_coord( { .x = 1, .y = 1 } )
          .has_value() );
  REQUIRE( W.natives()
               .maybe_dwelling_from_coord( { .x = 2, .y = 1 } )
               .has_value() );
  W.natives().destroy_dwelling( DwellingId{ 2 } );
  REQUIRE(
      W.natives().dwellings_for_tribe( e_tribe::iroquois ) ==
      unordered_set<DwellingId>{} );
  REQUIRE_FALSE(
      W.natives()
          .maybe_dwelling_from_coord( { .x = 1, .y = 1 } )
          .has_value() );
  REQUIRE_FALSE(
      W.natives()
          .maybe_dwelling_from_coord( { .x = 2, .y = 1 } )
          .has_value() );
}

TEST_CASE( "[ss/natives] destroy_tribe_last_step" ) {
  World W;
  REQUIRE_FALSE( W.natives().tribe_exists( e_tribe::arawak ) );
  REQUIRE( W.natives().dwellings_for_tribe( e_tribe::arawak ) ==
           nothing );
  W.add_tribe( e_tribe::arawak );
  REQUIRE( W.natives().tribe_exists( e_tribe::arawak ) );
  REQUIRE( W.natives().dwellings_for_tribe( e_tribe::arawak ) !=
           nothing );
  W.natives().destroy_tribe_last_step( e_tribe::arawak );
  REQUIRE_FALSE( W.natives().tribe_exists( e_tribe::arawak ) );
  REQUIRE( W.natives().dwellings_for_tribe( e_tribe::arawak ) ==
           nothing );
}

TEST_CASE( "[ss/natives] mark_land_unowned_for_dwellings" ) {
  World W;
  W.add_tribe( e_tribe::arawak );
  DwellingId const dwelling1_id =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois ).id;
  DwellingId const dwelling2_id =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::iroquois ).id;
  DwellingId const dwelling3_id =
      W.add_dwelling( { .x = 3, .y = 1 }, e_tribe::cherokee ).id;
  DwellingId const dwelling4_id =
      W.add_dwelling( { .x = 4, .y = 1 }, e_tribe::cherokee ).id;
  W.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 0 } );
  W.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 0 } );
  W.natives().mark_land_owned( dwelling3_id,
                               { .x = 3, .y = 0 } );
  W.natives().mark_land_owned( dwelling4_id,
                               { .x = 4, .y = 0 } );

  auto read = [&]( Coord coord ) {
    return base::lookup(
        W.natives().testing_only_owned_land_without_minuit(),
        coord );
  };

  REQUIRE( read( { .x = 1, .y = 0 } ) == dwelling1_id );
  REQUIRE( read( { .x = 2, .y = 0 } ) == dwelling2_id );
  REQUIRE( read( { .x = 3, .y = 0 } ) == dwelling3_id );
  REQUIRE( read( { .x = 4, .y = 0 } ) == dwelling4_id );

  W.natives().mark_land_unowned_for_tribe( e_tribe::iroquois );

  REQUIRE( read( { .x = 1, .y = 0 } ) == nothing );
  REQUIRE( read( { .x = 2, .y = 0 } ) == nothing );
  REQUIRE( read( { .x = 3, .y = 0 } ) == dwelling3_id );
  REQUIRE( read( { .x = 4, .y = 0 } ) == dwelling4_id );

  W.natives().mark_land_unowned_for_dwellings(
      { dwelling3_id } );

  REQUIRE( read( { .x = 1, .y = 0 } ) == nothing );
  REQUIRE( read( { .x = 2, .y = 0 } ) == nothing );
  REQUIRE( read( { .x = 3, .y = 0 } ) == nothing );
  REQUIRE( read( { .x = 4, .y = 0 } ) == dwelling4_id );

  W.natives().mark_land_unowned_for_tribe( e_tribe::cherokee );

  REQUIRE( read( { .x = 1, .y = 0 } ) == nothing );
  REQUIRE( read( { .x = 2, .y = 0 } ) == nothing );
  REQUIRE( read( { .x = 3, .y = 0 } ) == nothing );
  REQUIRE( read( { .x = 4, .y = 0 } ) == nothing );
}

} // namespace
} // namespace rn
