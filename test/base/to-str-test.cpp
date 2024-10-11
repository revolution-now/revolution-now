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

TEST_CASE( "[to-str] char" ) {
  REQUIRE( to_str( 'a' ) == "a" );
  REQUIRE( to_str( 'A' ) == "A" );
  REQUIRE( to_str( '\0' ) == string( 1, 0 ) );
  REQUIRE( to_str( '\1' ) == "\x01" );
  REQUIRE( to_str( ' ' ) == " " );
}

TEST_CASE( "[to-str] int8_t" ) {
  REQUIRE( to_str( int8_t{ -1 } ) == "-1" );
  REQUIRE( to_str( int8_t{ 0 } ) == "0" );
  REQUIRE( to_str( int8_t{ 5 } ) == "5" );
  REQUIRE( to_str( int8_t{ 127 } ) == "127" );
  REQUIRE( to_str( int8_t{ -128 } ) == "-128" );
}

TEST_CASE( "[to-str] uint8_t" ) {
  REQUIRE( to_str( uint8_t{ 1 } ) == "1" );
  REQUIRE( to_str( uint8_t{ 0 } ) == "0" );
  REQUIRE( to_str( uint8_t{ 5 } ) == "5" );
  REQUIRE( to_str( uint8_t{ 255 } ) == "255" );
}

TEST_CASE( "[to-str] int16_t" ) {
  REQUIRE( to_str( int16_t{ -1 } ) == "-1" );
  REQUIRE( to_str( int16_t{ 0 } ) == "0" );
  REQUIRE( to_str( int16_t{ 5 } ) == "5" );
  REQUIRE( to_str( int16_t{ 1'234 } ) == "1234" );
  REQUIRE( to_str( int16_t{ 32'767 } ) == "32767" );
  REQUIRE( to_str( int16_t{ -32'768 } ) == "-32768" );
}

TEST_CASE( "[to-str] uint16_t" ) {
  REQUIRE( to_str( uint16_t{ 1 } ) == "1" );
  REQUIRE( to_str( uint16_t{ 0 } ) == "0" );
  REQUIRE( to_str( uint16_t{ 5 } ) == "5" );
  REQUIRE( to_str( uint16_t{ 1'234 } ) == "1234" );
  REQUIRE( to_str( uint16_t{ 65'535 } ) == "65535" );
}

TEST_CASE( "[to-str] int" ) {
  REQUIRE( to_str( -1 ) == "-1" );
  REQUIRE( to_str( 0 ) == "0" );
  REQUIRE( to_str( 5 ) == "5" );
  REQUIRE( to_str( 1'234 ) == "1234" );
}

TEST_CASE( "[to-str] uint32_t" ) {
  REQUIRE( to_str( uint32_t{ 0 } ) == "0" );
  REQUIRE( to_str( uint32_t{ 5 } ) == "5" );
  REQUIRE( to_str( uint32_t{ 1'234 } ) == "1234" );
}

TEST_CASE( "[to-str] size_t" ) {
  REQUIRE( to_str( size_t{ 0 } ) == "0" );
  REQUIRE( to_str( size_t{ 5 } ) == "5" );
  REQUIRE( to_str( size_t{ 1'234 } ) == "1234" );
}

TEST_CASE( "[to-str] long" ) {
  REQUIRE( to_str( -1L ) == "-1" );
  REQUIRE( to_str( 0L ) == "0" );
  REQUIRE( to_str( 5L ) == "5" );
  REQUIRE( to_str( 1'234L ) == "1234" );
}

TEST_CASE( "[to-str] float" ) {
  REQUIRE( to_str( -1.0 ) == "-1" );
  REQUIRE( to_str( 0.0 ) == "0" );
  REQUIRE( to_str( 5.0 ) == "5" );
  REQUIRE( to_str( 1234.0 ) == "1234" );
  REQUIRE( to_str( -1.3 ) == "-1.3" );
  REQUIRE( to_str( 0.3 ) == "0.3" );
  REQUIRE( to_str( 5.3 ) == "5.3" );
  REQUIRE( to_str( 1234.3 ) == "1234.3" );
}

TEST_CASE( "[to-str] double" ) {
  REQUIRE( to_str( -1.0 ) == "-1" );
  REQUIRE( to_str( 0.0 ) == "0" );
  REQUIRE( to_str( 5.0 ) == "5" );
  REQUIRE( to_str( 1234.0 ) == "1234" );
  REQUIRE( to_str( -1.3 ) == "-1.3" );
  REQUIRE( to_str( 0.3 ) == "0.3" );
  REQUIRE( to_str( 5.3 ) == "5.3" );
  REQUIRE( to_str( 1234.3 ) == "1234.3" );
}

TEST_CASE( "[to-str] bool" ) {
  REQUIRE( to_str( true ) == "true" );
  REQUIRE( to_str( false ) == "false" );
}

TEST_CASE( "[to-str] string" ) {
  REQUIRE( to_str( "" ) == "" );
  REQUIRE( to_str( "x" ) == "x" );
  REQUIRE( to_str( "hello world" ) == "hello world" );
}

TEST_CASE( "[to-str] nullptr_t" ) {
  REQUIRE( to_str( nullptr ) == "nullptr" );
}

TEST_CASE( "[to-str] char const[N]" ) {
  REQUIRE( to_str( "hello" ) == "hello" );
  char const arr[7]{ 'a', 'b', 0, 'c', 'd', 0 };
  REQUIRE( to_str( arr ) == "ab" );
}

} // namespace
} // namespace base
