/****************************************************************
**root-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-23.
*
* Description: Unit tests for the ss/root module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/root.hpp"

// Testing
#include "test/fake/world.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;
using ::Catch::Matchers::Contains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_player( e_player::english );
    add_player( e_player::french );
    set_default_player_type( e_player::french );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
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
TEST_CASE( "[ss/root] validation" ) {
  world w;

  valid_or<string> v = valid;

  REQUIRE( w.root().validate() == valid );

  w.add_colony( { .x = 1, .y = 1 } );
  REQUIRE( w.root().validate() == valid );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 1 } );
  REQUIRE( w.root().validate() == valid );

  SECTION( "invalid european unit" ) {
    w.add_unit_on_map( e_unit_type::free_colonist,
                       { .x = 1, .y = 1 }, e_player::english );
    v = w.root().validate();
    REQUIRE( v != valid );
    REQUIRE_THAT(
        v.error(),
        Contains( "is owned by the french but contains "
                  "a unit owned by the english." ) );
  }

  SECTION( "invalid native unit" ) {
    Dwelling const& dwelling =
        w.add_dwelling( { .x = 0, .y = 1 }, e_tribe::apache );
    w.add_native_unit_on_map( e_native_unit_type::brave,
                              { .x = 1, .y = 1 }, dwelling.id );
    v = w.root().validate();
    REQUIRE( v != valid );
    REQUIRE_THAT( v.error(),
                  Contains( "contains native units." ) );
  }
}

TEST_CASE( "[ss/root] missing player_terrain" ) {
  world w;
}

} // namespace
} // namespace rn
