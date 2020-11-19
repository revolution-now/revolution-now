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

  REQUIRE( holds( v1, 5 ) );
  REQUIRE( !holds( v1, 6 ) );
  REQUIRE( !holds( v1, string( "world" ) ) );
}

TEST_CASE( "[variant] get if" ) {
  variant<int, string> v1{ 5 };
  variant<int, string> v2{ "hello" };

  bool is_int = false;
  GET_IF( v2, int, p ) { is_int = true; }
  bool is_string = false;
  GET_IF( v2, string, p ) {
    REQUIRE( *p == "hello" );
    is_string = true;
  }

  REQUIRE( !is_int );
  REQUIRE( is_string );
}

} // namespace
} // namespace rn