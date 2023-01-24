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
TEST_CASE( "[ss/natives] dwellings_for_tribe" ) {
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
  W.natives().destroy_dwelling( DwellingId{ 1 } );
  REQUIRE(
      W.natives().dwellings_for_tribe( e_tribe::iroquois ) ==
      unordered_set<DwellingId>{ DwellingId{ 2 } } );
  W.natives().destroy_dwelling( DwellingId{ 2 } );
  REQUIRE(
      W.natives().dwellings_for_tribe( e_tribe::iroquois ) ==
      unordered_set<DwellingId>{} );
}

} // namespace
} // namespace rn
