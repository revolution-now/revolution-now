/****************************************************************
**vec-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-07-13.
*
* Description: Unit tests for the rand/vec module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand/vec.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rng {
namespace {

using namespace std;

using ::Catch::Detail::Approx;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rand/vec] operator*" ) {
  vec2 v{ .x = 3.3, .y = 5.2 };
  vec2 ex;

  v  = v * 1.2;
  ex = { .x = 3.96, .y = 6.24 };
  REQUIRE( v.x == Approx( ex.x ) );
  REQUIRE( v.y == Approx( ex.y ) );
}

TEST_CASE( "[rand/vec] operator/" ) {
  vec2 v{ .x = 3.3, .y = 5.2 };
  vec2 ex;

  v  = v / 1.2;
  ex = { .x = 2.75, .y = 4.333333 };
  REQUIRE( v.x == Approx( ex.x ) );
  REQUIRE( v.y == Approx( ex.y ) );
}

TEST_CASE( "[rand/vec] to_str" ) {
  vec2 v{ .x = 3.3, .y = 5.2 };
  string ex;

  ex = "vec2{x=3.3,y=5.2}";
  REQUIRE( base::to_str( v ) == ex );
}

} // namespace
} // namespace rng
