/****************************************************************
**conv.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-02.
*
* Description: Unit tests for the src/base/conv.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "base/conv.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

TEST_CASE( "[conv] from string" ) {
  REQUIRE( !base::stoi( "" ).has_value() );
  REQUIRE( base::stoi( "0" ) == 0 );
  REQUIRE( base::stoi( "1" ) == 1 );
  REQUIRE( base::stoi( "222" ) == 222 );
  REQUIRE( base::stoi( "0", 16 ) == 0 );
  REQUIRE( base::stoi( "10", 16 ) == 16 );
  REQUIRE( base::stoi( "-10" ) == -10 );
  REQUIRE( base::stoi( "-0" ) == 0 );
  REQUIRE( !base::stoi( "1 a" ).has_value() );

  REQUIRE( !base::from_chars<int>( "" ).has_value() );
  REQUIRE( base::from_chars<int>( "0" ) == 0 );
  REQUIRE( base::from_chars<int>( "1" ) == 1 );
  REQUIRE( base::from_chars<int>( "222" ) == 222 );
  REQUIRE( base::from_chars<int>( "0", 16 ) == 0 );
  REQUIRE( base::from_chars<int>( "10", 16 ) == 16 );
  REQUIRE( base::from_chars<int>( "-10" ) == -10 );
  REQUIRE( base::from_chars<int>( "-0" ) == 0 );
  REQUIRE( !base::from_chars<int>( "1 a" ).has_value() );
}

TEST_CASE( "[conv] from_chars floating" ) {
  REQUIRE( !base::from_chars<double>( "" ).has_value() );
  REQUIRE( base::from_chars<double>( "0" ) == 0.0 );
  REQUIRE( base::from_chars<double>( "0." ) == 0.0 );
  REQUIRE( base::from_chars<double>( "1" ) == 1.0 );
  REQUIRE( base::from_chars<double>( "1.0" ) == 1.0 );
  REQUIRE( base::from_chars<double>( "22.2" ) == 22.2 );
  REQUIRE( base::from_chars<double>( ".0" ) == 0.0 );
  REQUIRE( base::from_chars<double>( ".10" ) == 0.1 );
  REQUIRE( base::from_chars<double>( "-10" ) == -10.0 );
  REQUIRE( base::from_chars<double>( "-10." ) == -10.0 );
  REQUIRE( base::from_chars<double>( "-10.123" ) == -10.123 );
  REQUIRE( base::from_chars<double>( "-0" ) == 0.0 );
  REQUIRE( !base::from_chars<double>( "1 a" ).has_value() );
}

TEST_CASE( "[conv] int_to_string_literary" ) {
  REQUIRE( int_to_string_literary( 0 ) == "zero" );
  REQUIRE( int_to_string_literary( 1 ) == "one" );
  REQUIRE( int_to_string_literary( 2 ) == "two" );
  REQUIRE( int_to_string_literary( 3 ) == "three" );
  REQUIRE( int_to_string_literary( 4 ) == "four" );
  REQUIRE( int_to_string_literary( 5 ) == "five" );
  REQUIRE( int_to_string_literary( 6 ) == "six" );
  REQUIRE( int_to_string_literary( 7 ) == "seven" );
  REQUIRE( int_to_string_literary( 8 ) == "eight" );
  REQUIRE( int_to_string_literary( 9 ) == "nine" );
  REQUIRE( int_to_string_literary( 10 ) == "10" );
  REQUIRE( int_to_string_literary( 11 ) == "11" );
  REQUIRE( int_to_string_literary( 12 ) == "12" );
  REQUIRE( int_to_string_literary( 20 ) == "20" );
  REQUIRE( int_to_string_literary( 1'000 ) == "1000" );
}

} // namespace
} // namespace base
