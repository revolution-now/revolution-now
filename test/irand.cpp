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

TEST_CASE( "[irand] shuffle" ) {
  MockIRand impl;
  IRand&    irand = impl;

  vector<string>   v, expected;
  e_interval const interval = e_interval::closed;

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
    impl.EXPECT__between_ints( 0, 1, interval ).returns( 0 );
    irand.shuffle( v );
    expected = { "hello", "world" };
    REQUIRE( v == expected );
    impl.EXPECT__between_ints( 0, 1, interval ).returns( 1 );
    irand.shuffle( v );
    expected = { "world", "hello" };
    REQUIRE( v == expected );
  }

  SECTION( "three" ) {
    v = { "hello", "world", "again" };
    impl.EXPECT__between_ints( 0, 2, interval ).returns( 1 );
    impl.EXPECT__between_ints( 1, 2, interval ).returns( 1 );
    irand.shuffle( v );
    expected = { "world", "hello", "again" };
    REQUIRE( v == expected );
  }

  SECTION( "four" ) {
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__between_ints( 0, 3, interval ).returns( 0 );
    impl.EXPECT__between_ints( 1, 3, interval ).returns( 2 );
    impl.EXPECT__between_ints( 2, 3, interval ).returns( 3 );
    irand.shuffle( v );
    expected = { "hello", "again", "!!", "world" };
    REQUIRE( v == expected );
  }

  SECTION( "four unchanged" ) {
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__between_ints( 0, 3, interval ).returns( 0 );
    impl.EXPECT__between_ints( 1, 3, interval ).returns( 1 );
    impl.EXPECT__between_ints( 2, 3, interval ).returns( 2 );
    irand.shuffle( v );
    expected = { "hello", "world", "again", "!!" };
    REQUIRE( v == expected );
  }

  SECTION( "four reversed" ) {
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__between_ints( 0, 3, interval ).returns( 3 );
    impl.EXPECT__between_ints( 1, 3, interval ).returns( 2 );
    impl.EXPECT__between_ints( 2, 3, interval ).returns( 2 );
    irand.shuffle( v );
    expected = { "!!", "again", "world", "hello" };
    REQUIRE( v == expected );
  }
}

TEST_CASE( "[irand] expect_shuffle" ) {
  MockIRand impl;
  IRand&    irand = impl;

  vector<string> to_be_shuffled{ "0", "1", "2", "3", "4",
                                 "5", "6", "7", "8", "9" };
  vector<int>    final_indices{ 9, 0, 4, 7, 6, 8, 3, 1, 2, 5 };
  vector<string> expected{ "9", "0", "4", "7", "6",
                           "8", "3", "1", "2", "5" };

  expect_shuffle( impl, final_indices );

  REQUIRE( to_be_shuffled == vector<string>{ "0", "1", "2", "3",
                                             "4", "5", "6", "7",
                                             "8", "9" } );
  irand.shuffle( to_be_shuffled );
  REQUIRE( to_be_shuffled == vector<string>{ "9", "0", "4", "7",
                                             "6", "8", "3", "1",
                                             "2", "5" } );
}

} // namespace
} // namespace rn
