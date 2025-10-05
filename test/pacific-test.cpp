/****************************************************************
**pacific-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-28.
*
* Description: Unit tests for the pacific module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/pacific.hpp"

// Testing.
#include "test/fake/world.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const _ = make_ocean();
    static MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,X,X,X,X,X,_, /*0
      1*/ _,X,X,X,X,X,X,_, /*1
      2*/ _,X,X,X,X,X,X,_, /*2
      3*/ _,X,X,X,X,X,X,_, /*3
      4*/ _,X,X,X,X,X,X,_, /*4
      5*/ _,X,X,X,X,X,X,_, /*5
      6*/ _,X,X,X,X,X,X,_, /*6
      7*/ _,X,X,X,X,X,X,_, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[pacific] is_atlantic_side_of_map / "
    "is_pacific_side_of_map" ) {
  world w;

  auto const atlantic =
      [&] [[clang::noinline]] ( point const p ) {
        return is_atlantic_side_of_map( w.terrain(), p );
      };

  auto const pacific =
      [&] [[clang::noinline]] ( point const p ) {
        return is_pacific_side_of_map( w.terrain(), p );
      };

  REQUIRE( !atlantic( { .x = 0, .y = 2 } ) );
  REQUIRE( !atlantic( { .x = 1, .y = 2 } ) );
  REQUIRE( !atlantic( { .x = 2, .y = 2 } ) );
  REQUIRE( !atlantic( { .x = 3, .y = 2 } ) );
  REQUIRE( atlantic( { .x = 4, .y = 2 } ) );
  REQUIRE( atlantic( { .x = 5, .y = 2 } ) );
  REQUIRE( atlantic( { .x = 6, .y = 2 } ) );
  REQUIRE( atlantic( { .x = 7, .y = 2 } ) );
  REQUIRE( pacific( { .x = 0, .y = 2 } ) );
  REQUIRE( pacific( { .x = 1, .y = 2 } ) );
  REQUIRE( pacific( { .x = 2, .y = 2 } ) );
  REQUIRE( pacific( { .x = 3, .y = 2 } ) );
  REQUIRE( !pacific( { .x = 4, .y = 2 } ) );
  REQUIRE( !pacific( { .x = 5, .y = 2 } ) );
  REQUIRE( !pacific( { .x = 6, .y = 2 } ) );
  REQUIRE( !pacific( { .x = 7, .y = 2 } ) );

  REQUIRE( !atlantic( { .x = 0, .y = 7 } ) );
  REQUIRE( !atlantic( { .x = 1, .y = 7 } ) );
  REQUIRE( !atlantic( { .x = 2, .y = 7 } ) );
  REQUIRE( !atlantic( { .x = 3, .y = 7 } ) );
  REQUIRE( atlantic( { .x = 4, .y = 7 } ) );
  REQUIRE( atlantic( { .x = 5, .y = 7 } ) );
  REQUIRE( atlantic( { .x = 6, .y = 7 } ) );
  REQUIRE( atlantic( { .x = 7, .y = 7 } ) );
  REQUIRE( pacific( { .x = 0, .y = 7 } ) );
  REQUIRE( pacific( { .x = 1, .y = 7 } ) );
  REQUIRE( pacific( { .x = 2, .y = 7 } ) );
  REQUIRE( pacific( { .x = 3, .y = 7 } ) );
  REQUIRE( !pacific( { .x = 4, .y = 7 } ) );
  REQUIRE( !pacific( { .x = 5, .y = 7 } ) );
  REQUIRE( !pacific( { .x = 6, .y = 7 } ) );
  REQUIRE( !pacific( { .x = 7, .y = 7 } ) );
}

} // namespace
} // namespace rn
