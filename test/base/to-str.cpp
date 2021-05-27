/****************************************************************
**to-str.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-07.
*
* Description: Unit tests for the src/base/to-str.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

template<typename T>
string call_to_str( T const& o ) {
  string out;
  to_str( o, out );
  return out;
}

TEST_CASE( "[to-str] int" ) {
  REQUIRE( call_to_str( -1 ) == "-1" );
  REQUIRE( call_to_str( 0 ) == "0" );
  REQUIRE( call_to_str( 5 ) == "5" );
  REQUIRE( call_to_str( 1234 ) == "1234" );
}

TEST_CASE( "[to-str] string" ) {
  REQUIRE( call_to_str( "" ) == "" );
  REQUIRE( call_to_str( "x" ) == "x" );
  REQUIRE( call_to_str( "hello world" ) == "hello world" );
}

} // namespace
} // namespace base
