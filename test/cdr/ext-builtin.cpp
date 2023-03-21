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

static_assert( Canonical<char> );
static_assert( Canonical<int> );
static_assert( Canonical<double> );
static_assert( Canonical<bool> );
static_assert( !Canonical<long> );
static_assert( !Canonical<float> );

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
  REQUIRE( conv.from<char>( value{ 5 } ) ==
           conv.err( "cannot convert value of type integer to "
                     "character." ) );
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
