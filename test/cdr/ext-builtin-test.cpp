/****************************************************************
**ext-builtin.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-27.
*
* Description: Unit tests for the src/cdr/ext-builtin.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/cdr/ext-builtin.hpp"

// cdr
#include "src/cdr/converter.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace cdr {
namespace {

using namespace std;

using ::cdr::testing::conv_from_bt;

static_assert( is_same_v<integer_type, int64_t> );

static_assert( Canonical<int8_t> );
static_assert( Canonical<int16_t> );
static_assert( Canonical<int32_t> );
static_assert( Canonical<int64_t> );

static_assert( Canonical<uint8_t> );
static_assert( Canonical<uint16_t> );
static_assert( Canonical<uint32_t> );
static_assert( Canonical<uint64_t> );

static_assert( Canonical<bool> );
static_assert( Canonical<char> );
static_assert( Canonical<int> );
static_assert( Canonical<long> );

static_assert( !Canonical<float> );
static_assert( Canonical<double> );

converter conv;

TEST_CASE( "[cdr/ext-builtin] char" ) {
  char c = 'S';

  REQUIRE( conv.to( c ) == "S" );
  REQUIRE( conv.to( c ).holds<string>() );
  REQUIRE( conv_from_bt<char>( conv, value{ "S" } ) == c );
  REQUIRE(
      conv.from<char>( value{ "SS" } ) ==
      conv.err(
          "expected character but found string of length 2." ) );
  REQUIRE( conv.from<char>( value{ -128 } ) == char{ -128 } );
  REQUIRE( conv.from<char>( value{ -1 } ) == char{ -1 } );
  REQUIRE( conv.from<char>( value{ 0 } ) == char{ 0 } );
  REQUIRE( conv.from<char>( value{ 1 } ) == char{ 1 } );
  REQUIRE( conv.from<char>( value{ 127 } ) == char{ 127 } );
  REQUIRE( conv.from<char>( value{ -129 } ) ==
           conv.err( "received out-of-range integral "
                     "representation of char: -129" ) );
  REQUIRE( conv.from<char>( value{ 128 } ) ==
           conv.err( "received out-of-range integral "
                     "representation of char: 128" ) );

  REQUIRE( conv.from<char>( value{ 5.5 } ) ==
           conv.err( "cannot convert value of type floating to "
                     "character." ) );
}

TEST_CASE( "[cdr/ext-builtin] int8_t" ) {
  int8_t n = 4;

  REQUIRE( conv.to( n ) == 4 );
  REQUIRE( conv.to( n ).holds<integer_type>() );
  REQUIRE( conv_from_bt<int8_t>( conv, value{ n } ) == n );
  REQUIRE( conv_from_bt<int8_t>( conv, value{ -128 } ) == -128 );
  REQUIRE( conv_from_bt<int8_t>( conv, value{ 0 } ) == 0 );
  REQUIRE( conv_from_bt<int8_t>( conv, value{ 127 } ) == 127 );
  REQUIRE(
      conv.from<bool>( value{ n } ) ==
      conv.err(
          "failed to convert value of type integer to bool." ) );
  REQUIRE( conv.from<int8_t>( value{ -129 } ) ==
           conv.err( "number out of range for conversion to "
                     "signed 8 bit integer: -129" ) );
  REQUIRE( conv.from<int8_t>( value{ 128 } ) ==
           conv.err( "number out of range for conversion to "
                     "signed 8 bit integer: 128" ) );
}

TEST_CASE( "[cdr/ext-builtin] int16_t" ) {
  int16_t n = 4;

  REQUIRE( conv.to( n ) == 4 );
  REQUIRE( conv.to( n ).holds<integer_type>() );
  REQUIRE( conv_from_bt<int16_t>( conv, value{ n } ) == n );
  REQUIRE( conv_from_bt<int16_t>( conv, value{ -32768 } ) ==
           -32768 );
  REQUIRE( conv_from_bt<int16_t>( conv, value{ 0 } ) == 0 );
  REQUIRE( conv_from_bt<int16_t>( conv, value{ 32767 } ) ==
           32767 );
  REQUIRE(
      conv.from<bool>( value{ n } ) ==
      conv.err(
          "failed to convert value of type integer to bool." ) );
  REQUIRE( conv.from<int16_t>( value{ -32769 } ) ==
           conv.err( "number out of range for conversion to "
                     "signed 16 bit integer: -32769" ) );
  REQUIRE( conv.from<int16_t>( value{ 32768 } ) ==
           conv.err( "number out of range for conversion to "
                     "signed 16 bit integer: 32768" ) );
}

TEST_CASE( "[cdr/ext-builtin] int64_t" ) {
  int64_t n = 4;

  REQUIRE( conv.to( n ) == 4 );
  REQUIRE( conv.to( n ).holds<integer_type>() );
  REQUIRE( conv_from_bt<int64_t>( conv, value{ n } ) == n );
  REQUIRE( conv_from_bt<int64_t>(
               conv, value{ -9223372036854775807 - 1 } ) ==
           -9223372036854775807 - 1 );
  REQUIRE( conv_from_bt<int64_t>( conv, value{ 0 } ) == 0 );
  REQUIRE( conv_from_bt<int64_t>(
               conv, value{ 9223372036854775807 } ) ==
           9223372036854775807 );
  REQUIRE(
      conv.from<bool>( value{ n } ) ==
      conv.err(
          "failed to convert value of type integer to bool." ) );
}

TEST_CASE( "[cdr/ext-builtin] uint8_t" ) {
  uint8_t n = 4;

  REQUIRE( conv.to( n ) == 4 );
  REQUIRE( conv.to( n ).holds<integer_type>() );
  REQUIRE( conv_from_bt<uint8_t>( conv, value{ n } ) == n );
  REQUIRE( conv_from_bt<uint8_t>( conv, value{ 0 } ) == 0 );
  REQUIRE( conv_from_bt<uint8_t>( conv, value{ 255 } ) == 255 );
  REQUIRE(
      conv.from<bool>( value{ n } ) ==
      conv.err(
          "failed to convert value of type integer to bool." ) );
  REQUIRE( conv.from<uint8_t>( value{ -1 } ) ==
           conv.err( "number out of range for conversion to "
                     "unsigned 8 bit integer: -1" ) );
  REQUIRE( conv.from<uint8_t>( value{ 256 } ) ==
           conv.err( "number out of range for conversion to "
                     "unsigned 8 bit integer: 256" ) );
}

TEST_CASE( "[cdr/ext-builtin] uint16_t" ) {
  uint16_t n = 4;

  REQUIRE( conv.to( n ) == 4 );
  REQUIRE( conv.to( n ).holds<integer_type>() );
  REQUIRE( conv_from_bt<uint16_t>( conv, value{ n } ) == n );
  REQUIRE( conv_from_bt<uint16_t>( conv, value{ 0 } ) == 0 );
  REQUIRE( conv_from_bt<uint16_t>( conv, value{ 65535 } ) ==
           65535 );
  REQUIRE(
      conv.from<bool>( value{ n } ) ==
      conv.err(
          "failed to convert value of type integer to bool." ) );
  REQUIRE( conv.from<uint16_t>( value{ -1 } ) ==
           conv.err( "number out of range for conversion to "
                     "unsigned 16 bit integer: -1" ) );
  REQUIRE( conv.from<uint16_t>( value{ 65536 } ) ==
           conv.err( "number out of range for conversion to "
                     "unsigned 16 bit integer: 65536" ) );
}

TEST_CASE( "[cdr/ext-builtin] uint32_t" ) {
  uint32_t n = 4;

  REQUIRE( conv.to( n ) == 4 );
  REQUIRE( conv.to( n ).holds<integer_type>() );
  REQUIRE( conv_from_bt<uint32_t>( conv, value{ n } ) == n );
  REQUIRE( conv_from_bt<uint32_t>( conv, value{ 0 } ) == 0u );
  REQUIRE( conv_from_bt<uint32_t>( conv, value{ 4294967295 } ) ==
           4294967295 );
  REQUIRE(
      conv.from<bool>( value{ n } ) ==
      conv.err(
          "failed to convert value of type integer to bool." ) );
  REQUIRE( conv.from<uint32_t>( value{ -1 } ) ==
           conv.err( "number out of range for conversion to "
                     "unsigned 32 bit integer: -1" ) );
  REQUIRE( conv.from<uint32_t>( value{ 4294967296 } ) ==
           conv.err( "number out of range for conversion to "
                     "unsigned 32 bit integer: 4294967296" ) );
}

TEST_CASE( "[cdr/ext-builtin] uint64_t" ) {
  unsigned int n = 4;

  REQUIRE( conv.to( n ) == 4 );
  REQUIRE( conv.to( n ).holds<integer_type>() );
  REQUIRE( conv_from_bt<uint64_t>( conv, value{ n } ) == n );
  REQUIRE( conv_from_bt<uint64_t>( conv, value{ 0 } ) == 0u );
  REQUIRE(
      conv.from<bool>( value{ n } ) ==
      conv.err(
          "failed to convert value of type integer to bool." ) );
  REQUIRE( conv.from<uint64_t>( value{ -1 } ) ==
           conv.err( "number out of range for conversion to "
                     "unsigned 64 bit integer: -1" ) );
}

TEST_CASE( "[cdr/ext-builtin] int" ) {
  int n = 4;

  REQUIRE( conv.to( n ) == 4 );
  REQUIRE( conv.to( n ).holds<integer_type>() );
  REQUIRE( conv_from_bt<int>( conv, value{ n } ) == n );
  REQUIRE(
      conv.from<bool>( value{ n } ) ==
      conv.err(
          "failed to convert value of type integer to bool." ) );
}

TEST_CASE( "[cdr/ext-builtin] bool" ) {
  bool b = true;

  REQUIRE( conv.to( b ) == true );
  REQUIRE( conv.to( b ).holds<bool>() );
  REQUIRE( conv_from_bt<bool>( conv, value{ b } ) == b );
  REQUIRE( conv.from<double>( value{ b } ) ==
           conv.err( "failed to convert value of type boolean "
                     "to double." ) );
  REQUIRE(
      conv.from<int>( value{ b } ) ==
      conv.err(
          "failed to convert value of type boolean to int." ) );
}

TEST_CASE( "[cdr/ext-builtin] double" ) {
  double d = 5.5;

  REQUIRE( conv.to( d ) == 5.5 );
  REQUIRE( conv.to( d ).holds<double>() );
  REQUIRE( conv_from_bt<double>( conv, value{ d } ) == d );
  REQUIRE( conv.from<bool>( value{ d } ) ==
           conv.err( "failed to convert value of type floating "
                     "to bool." ) );
  REQUIRE(
      conv.from<int>( value{ d } ) ==
      conv.err(
          "failed to convert value of type floating to int." ) );
}

TEST_CASE( "[cdr/ext-builtin] double from int" ) {
  integer_type n = 5;
  double       d = 5.0;
  REQUIRE( conv_from_bt<double>( conv, value{ n } ) == d );
}

} // namespace
} // namespace cdr
