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
#include "src/base/to-str-ext-std.hpp"
#include "src/base/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

template<typename T>
string call_to_str( T const& o ) {
  string out;
  to_str( o, out, ADL );
  return out;
}

TEST_CASE( "[to-str] int" ) {
  REQUIRE( call_to_str( -1 ) == "-1" );
  REQUIRE( call_to_str( 0 ) == "0" );
  REQUIRE( call_to_str( 5 ) == "5" );
  REQUIRE( call_to_str( 1234 ) == "1234" );
}

TEST_CASE( "[to-str] uint32_t" ) {
  REQUIRE( call_to_str( uint32_t{ 0 } ) == "0" );
  REQUIRE( call_to_str( uint32_t{ 5 } ) == "5" );
  REQUIRE( call_to_str( uint32_t{ 1234 } ) == "1234" );
}

TEST_CASE( "[to-str] size_t" ) {
  REQUIRE( call_to_str( size_t{ 0 } ) == "0" );
  REQUIRE( call_to_str( size_t{ 5 } ) == "5" );
  REQUIRE( call_to_str( size_t{ 1234 } ) == "1234" );
}

TEST_CASE( "[to-str] long" ) {
  REQUIRE( call_to_str( -1L ) == "-1" );
  REQUIRE( call_to_str( 0L ) == "0" );
  REQUIRE( call_to_str( 5L ) == "5" );
  REQUIRE( call_to_str( 1234L ) == "1234" );
}

TEST_CASE( "[to-str] double" ) {
  REQUIRE( call_to_str( -1.0 ) == "-1" );
  REQUIRE( call_to_str( 0.0 ) == "0" );
  REQUIRE( call_to_str( 5.0 ) == "5" );
  REQUIRE( call_to_str( 1234.0 ) == "1234" );
  REQUIRE( call_to_str( -1.3 ) == "-1.3" );
  REQUIRE( call_to_str( 0.3 ) == "0.3" );
  REQUIRE( call_to_str( 5.3 ) == "5.3" );
  REQUIRE( call_to_str( 1234.3 ) == "1234.3" );
}

TEST_CASE( "[to-str] bool" ) {
  REQUIRE( call_to_str( true ) == "true" );
  REQUIRE( call_to_str( false ) == "false" );
}

TEST_CASE( "[to-str] string" ) {
  REQUIRE( call_to_str( "" ) == "" );
  REQUIRE( call_to_str( "x" ) == "x" );
  REQUIRE( call_to_str( "hello world" ) == "hello world" );
}

} // namespace
} // namespace base
