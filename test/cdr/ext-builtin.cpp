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

// Must be last.
#include "test/catch-common.hpp"

namespace cdr {
namespace {

using namespace std;

static_assert( Canonical<int> );
static_assert( Canonical<double> );
static_assert( Canonical<bool> );
static_assert( !Canonical<long> );
static_assert( !Canonical<float> );

TEST_CASE( "[cdr/ext-builtin] int" ) {
  int n = 4;

  REQUIRE( to_canonical( n ) == 4 );
  REQUIRE( to_canonical( n ).holds<int>() );
  REQUIRE( from_canonical<int>( value{ n } ) == n );
  REQUIRE( from_canonical<double>( value{ n } ) ==
           error::builder{ "test" }(
               "failed to convert cdr value of type integer "
               "to double." ) );
  REQUIRE( from_canonical<bool>( value{ n } ) ==
           error::builder{ "test" }(
               "failed to convert cdr value of type integer "
               "to bool." ) );
}

TEST_CASE( "[cdr/ext-builtin] bool" ) {
  bool b = true;

  REQUIRE( to_canonical( b ) == true );
  REQUIRE( to_canonical( b ).holds<bool>() );
  REQUIRE( from_canonical<bool>( value{ b } ) == b );
  REQUIRE( from_canonical<double>( value{ b } ) ==
           error::builder{ "test" }(
               "failed to convert cdr value of type boolean "
               "to double." ) );
  REQUIRE( from_canonical<int>( value{ b } ) ==
           error::builder{ "test" }(
               "failed to convert cdr value of type boolean "
               "to int." ) );
}

TEST_CASE( "[cdr/ext-builtin] double" ) {
  double d = 5.5;

  REQUIRE( to_canonical( d ) == 5.5 );
  REQUIRE( to_canonical( d ).holds<double>() );
  REQUIRE( from_canonical<double>( value{ d } ) == d );
  REQUIRE( from_canonical<bool>( value{ d } ) ==
           error::builder{ "test" }(
               "failed to convert cdr value of type floating "
               "to bool." ) );
  REQUIRE( from_canonical<int>( value{ d } ) ==
           error::builder{ "test" }(
               "failed to convert cdr value of type floating "
               "to int." ) );
}

} // namespace
} // namespace cdr
