/****************************************************************
**variant.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-19.
*
* Description: Tests for variant-handling utilities.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/variant.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[variant] holds" ) {
  variant<int, string> v1{ 5 };
  variant<int, string> v2{ "hello" };

  REQUIRE( holds<int>( v1 ) );
  REQUIRE( !holds<string>( v1 ) );
  REQUIRE( holds<string>( v2 ) );
  REQUIRE( !holds<int>( v2 ) );
  REQUIRE( holds( v1, 5 ) );
  REQUIRE( !holds( v1, 6 ) );
  REQUIRE( !holds( v1, string( "world" ) ) );
}

TEST_CASE( "[variant] if_get" ) {
  variant<int, string> v1{ 5 };
  variant<int, string> v2{ "hello" };

  bool is_int = false;
  if_get( v2, int, p ) {
    (void)p;
    is_int = true;
  }
  bool is_string = false;
  if_get( v2, string, p ) {
    REQUIRE( p == "hello" );
    is_string = true;
  }

  REQUIRE( !is_int );
  REQUIRE( is_string );
}

} // namespace
} // namespace rn