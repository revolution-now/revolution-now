/****************************************************************
**random-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-24.
*
* Description: Unit tests for the base/random module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/random.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/random] pick_one_safe" ) {
  random r;

  vector<int> v;

  SECTION( "empty" ) {
    v        = {};
    auto val = r.pick_one_safe( v );
    REQUIRE( val == nothing );
  }

  SECTION( "single" ) {
    v        = { 111 };
    auto val = r.pick_one_safe( v );
    REQUIRE( val == 111 );
  }

  SECTION( "double" ) {
    v        = { 111, 222 };
    auto val = r.pick_one_safe( v );
    REQUIRE( ( val == 111 || val == 222 ) );
  }

  SECTION( "triple" ) {
    v        = { 111, 222, 333 };
    auto val = r.pick_one_safe( v );
    REQUIRE( ( val == 111 || val == 222 || val == 333 ) );
  }
}

TEST_CASE( "[base/random] pick_one" ) {
  random r;

  vector<int> v;

  SECTION( "single" ) {
    v             = { 111 };
    int const val = r.pick_one( v );
    REQUIRE( val == 111 );
  }

  SECTION( "double" ) {
    v             = { 111, 222 };
    int const val = r.pick_one( v );
    REQUIRE( ( val == 111 || val == 222 ) );
  }

  SECTION( "triple" ) {
    v             = { 111, 222, 333 };
    int const val = r.pick_one( v );
    REQUIRE( ( val == 111 || val == 222 || val == 333 ) );
  }
}

TEST_CASE( "[base/random] bernoulli" ) {
  random r;
  bool const b = r.bernoulli( .7 );
  REQUIRE( ( b == true || b == false ) );
}

TEST_CASE( "[base/random] uniform" ) {
  random r;
  int const i = r.uniform( 5, 8 );
  REQUIRE( ( i >= 5 && i <= 8 ) );
  double const d = r.uniform( 5.5, 8.3 );
  REQUIRE( ( d >= 5.5 && d <= 8.3 ) );
}

} // namespace
} // namespace base
