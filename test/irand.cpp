/****************************************************************
**irand.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-19.
*
* Description: Unit tests for the src/irand.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/irand.hpp"

// Testing
#include "test/mocks/irand.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[test/irand] shuffle" ) {
  MockIRand impl;
  IRand&    irand = impl;

  vector<string>          v, expected;
  IRand::e_interval const interval = IRand::e_interval::closed;

  SECTION( "empty" ) {
    v = {};
    irand.shuffle( v );
    expected = {};
    REQUIRE( v == expected );
  }

  SECTION( "one" ) {
    v = { "hello" };
    irand.shuffle( v );
    expected = { "hello" };
    REQUIRE( v == expected );
  }

  SECTION( "two" ) {
    v = { "hello", "world" };
    EXPECT_CALL( impl, between_ints( 0, 1, interval ) )
        .returns( 0 );
    irand.shuffle( v );
    expected = { "hello", "world" };
    REQUIRE( v == expected );
    EXPECT_CALL( impl, between_ints( 0, 1, interval ) )
        .returns( 1 );
    irand.shuffle( v );
    expected = { "world", "hello" };
    REQUIRE( v == expected );
  }

  SECTION( "three" ) {
    v = { "hello", "world", "again" };
    EXPECT_CALL( impl, between_ints( 0, 2, interval ) )
        .returns( 1 );
    EXPECT_CALL( impl, between_ints( 1, 2, interval ) )
        .returns( 1 );
    irand.shuffle( v );
    expected = { "world", "hello", "again" };
    REQUIRE( v == expected );
  }

  SECTION( "four" ) {
    v = { "hello", "world", "again", "!!" };
    EXPECT_CALL( impl, between_ints( 0, 3, interval ) )
        .returns( 0 );
    EXPECT_CALL( impl, between_ints( 1, 3, interval ) )
        .returns( 2 );
    EXPECT_CALL( impl, between_ints( 2, 3, interval ) )
        .returns( 3 );
    irand.shuffle( v );
    expected = { "hello", "again", "!!", "world" };
    REQUIRE( v == expected );
  }

  SECTION( "four unchanged" ) {
    v = { "hello", "world", "again", "!!" };
    EXPECT_CALL( impl, between_ints( 0, 3, interval ) )
        .returns( 0 );
    EXPECT_CALL( impl, between_ints( 1, 3, interval ) )
        .returns( 1 );
    EXPECT_CALL( impl, between_ints( 2, 3, interval ) )
        .returns( 2 );
    irand.shuffle( v );
    expected = { "hello", "world", "again", "!!" };
    REQUIRE( v == expected );
  }

  SECTION( "four reversed" ) {
    v = { "hello", "world", "again", "!!" };
    EXPECT_CALL( impl, between_ints( 0, 3, interval ) )
        .returns( 3 );
    EXPECT_CALL( impl, between_ints( 1, 3, interval ) )
        .returns( 2 );
    EXPECT_CALL( impl, between_ints( 2, 3, interval ) )
        .returns( 2 );
    irand.shuffle( v );
    expected = { "!!", "again", "world", "hello" };
    REQUIRE( v == expected );
  }
}

} // namespace
} // namespace rn
