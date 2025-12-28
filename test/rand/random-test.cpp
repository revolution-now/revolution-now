/****************************************************************
**random-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-24.
*
* Description: Unit tests for the rand/random module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand/random.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rng {
namespace {

using namespace std;

using ::base::nothing;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rand/random] reseed" ) {
  random r;

  r.reseed( 42 );
  int const n1 = r.uniform( 0, 1000 );
  r.reseed( 42 );
  int const n2 = r.uniform( 0, 1000 );
  r.reseed( 42 );
  int const n3 = r.uniform( 0, 1000 );

  REQUIRE( n1 == n2 );
  REQUIRE( n2 == n3 );
}

TEST_CASE( "[rand/random] pick_one_safe" ) {
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

TEST_CASE( "[rand/random] pick_one" ) {
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

TEST_CASE( "[rand/random] bernoulli" ) {
  random r;
  bool const b = r.bernoulli( .7 );
  REQUIRE( ( b == true || b == false ) );
}

TEST_CASE( "[rand/random] uniform( range )" ) {
  random r;
  int const i = r.uniform( 5, 8 );
  REQUIRE( ( i >= 5 && i <= 8 ) );
  double const d = r.uniform( 5.5, 8.3 );
  REQUIRE( ( d >= 5.5 && d <= 8.3 ) );
}

TEST_CASE( "[rand/random] uniform<T>" ) {
  random r;

  SECTION( "short" ) {
    int const i = r.uniform<short>();
    REQUIRE( i >= numeric_limits<short>::min() );
    REQUIRE( i <= numeric_limits<short>::max() );
  }

  SECTION( "unsigned short" ) {
    int const i = r.uniform<unsigned short>();
    REQUIRE( i >= numeric_limits<unsigned short>::min() );
    REQUIRE( i <= numeric_limits<unsigned short>::max() );
  }
}

} // namespace
} // namespace rng
