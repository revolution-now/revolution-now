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

TEST_CASE( "[to-str] primitive" ) {
  // bool
  REQUIRE( to_str( true ) == "true" );
  REQUIRE( to_str( false ) == "false" );

  // char
  REQUIRE( to_str( 'a' ) == "a" );
  REQUIRE( to_str( 'A' ) == "A" );
  REQUIRE( to_str( '\0' ) == string( 1, 0 ) );
  REQUIRE( to_str( '\1' ) == "\x01" );
  REQUIRE( to_str( ' ' ) == " " );

  // int8_t
  REQUIRE( to_str( int8_t{ -1 } ) == "-1" );
  REQUIRE( to_str( int8_t{ 0 } ) == "0" );
  REQUIRE( to_str( int8_t{ 5 } ) == "5" );
  REQUIRE( to_str( int8_t{ 127 } ) == "127" );
  REQUIRE( to_str( int8_t{ -128 } ) == "-128" );

  // uint8_t
  REQUIRE( to_str( uint8_t{ 1 } ) == "1" );
  REQUIRE( to_str( uint8_t{ 0 } ) == "0" );
  REQUIRE( to_str( uint8_t{ 5 } ) == "5" );
  REQUIRE( to_str( uint8_t{ 255 } ) == "255" );

  // int16_t
  REQUIRE( to_str( int16_t{ -1 } ) == "-1" );
  REQUIRE( to_str( int16_t{ 0 } ) == "0" );
  REQUIRE( to_str( int16_t{ 5 } ) == "5" );
  REQUIRE( to_str( int16_t{ 1234 } ) == "1234" );
  REQUIRE( to_str( int16_t{ 32767 } ) == "32767" );
  REQUIRE( to_str( int16_t{ -32768 } ) == "-32768" );

  // uint16_t
  REQUIRE( to_str( uint16_t{ 1 } ) == "1" );
  REQUIRE( to_str( uint16_t{ 0 } ) == "0" );
  REQUIRE( to_str( uint16_t{ 5 } ) == "5" );
  REQUIRE( to_str( uint16_t{ 1234 } ) == "1234" );
  REQUIRE( to_str( uint16_t{ 65535 } ) == "65535" );

  // int
  REQUIRE( to_str( int{ -1 } ) == "-1" );
  REQUIRE( to_str( int{ 0 } ) == "0" );
  REQUIRE( to_str( int{ 5 } ) == "5" );
  REQUIRE( to_str( int{ 1234 } ) == "1234" );

  // int32_t
  REQUIRE( to_str( int32_t{ -1 } ) == "-1" );
  REQUIRE( to_str( int32_t{ 0 } ) == "0" );
  REQUIRE( to_str( int32_t{ 5 } ) == "5" );
  REQUIRE( to_str( int32_t{ 1234 } ) == "1234" );

  // uint32_t
  REQUIRE( to_str( uint32_t{ 0 } ) == "0" );
  REQUIRE( to_str( uint32_t{ 5 } ) == "5" );
  REQUIRE( to_str( uint32_t{ 1234 } ) == "1234" );

  // int64_t
  REQUIRE( to_str( int64_t{ -1 } ) == "-1" );
  REQUIRE( to_str( int64_t{ 0 } ) == "0" );
  REQUIRE( to_str( int64_t{ 5 } ) == "5" );
  REQUIRE( to_str( int64_t{ 1234 } ) == "1234" );
  REQUIRE( to_str( int64_t{ 0x7FFFFFFFFFFFFFFF } ) ==
           "9223372036854775807" );

  // uint64_t
  REQUIRE( to_str( uint64_t{ 0 } ) == "0" );
  REQUIRE( to_str( uint64_t{ 5 } ) == "5" );
  REQUIRE( to_str( uint64_t{ 1234 } ) == "1234" );
  REQUIRE( to_str( uint64_t{ 0xFFFFFFFFFFFFFFFF } ) ==
           "18446744073709551615" );

  // size_t
  REQUIRE( to_str( size_t{ 0 } ) == "0" );
  REQUIRE( to_str( size_t{ 5 } ) == "5" );
  REQUIRE( to_str( size_t{ 1234 } ) == "1234" );

  // long
  REQUIRE( to_str( long{ -1L } ) == "-1" );
  REQUIRE( to_str( long{ 0L } ) == "0" );
  REQUIRE( to_str( long{ 5L } ) == "5" );
  REQUIRE( to_str( long{ 1234L } ) == "1234" );

  // float
  REQUIRE( to_str( float{ -1.0f } ) == "-1" );
  REQUIRE( to_str( float{ 0.0f } ) == "0" );
  REQUIRE( to_str( float{ 5.0f } ) == "5" );
  REQUIRE( to_str( float{ 1234.0f } ) == "1234" );
  REQUIRE( to_str( float{ -1.3f } ) == "-1.3" );
  REQUIRE( to_str( float{ 0.3f } ) == "0.3" );
  REQUIRE( to_str( float{ 5.3f } ) == "5.3" );
  REQUIRE( to_str( float{ 1234.3f } ) == "1234.3" );

  // double
  REQUIRE( to_str( double{ -1.0 } ) == "-1" );
  REQUIRE( to_str( double{ 0.0 } ) == "0" );
  REQUIRE( to_str( double{ 5.0 } ) == "5" );
  REQUIRE( to_str( double{ 1234.0 } ) == "1234" );
  REQUIRE( to_str( double{ -1.3 } ) == "-1.3" );
  REQUIRE( to_str( double{ 0.3 } ) == "0.3" );
  REQUIRE( to_str( double{ 5.3 } ) == "5.3" );
  REQUIRE( to_str( double{ 1234.3 } ) == "1234.3" );

  // double
  using LD = long double;
  REQUIRE( to_str( LD{ -1.0 } ) == "-1" );
  REQUIRE( to_str( LD{ 0.0 } ) == "0" );
  REQUIRE( to_str( LD{ 5.0 } ) == "5" );
  REQUIRE( to_str( LD{ 1234.0 } ) == "1234" );
  REQUIRE( to_str( LD{ -1.25 } ) == "-1.25" );
  REQUIRE( to_str( LD{ 0.5 } ) == "0.5" );
  REQUIRE( to_str( LD{ 5.125125125125125 } ) ==
           "5.1251251251251250807" );
  REQUIRE( to_str( LD{ 1234.75 } ) == "1234.75" );

  // char const[N]
  REQUIRE( to_str( "hello" ) == "hello" );
  char const arr[7]{ 'a', 'b', 0, 'c', 'd', 0 };
  REQUIRE( to_str( arr ) == "ab" );
}

} // namespace
} // namespace base
