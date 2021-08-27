/****************************************************************
**ext-builtin.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-25.
*
* Description: Unit tests for the src/rcl/ext-builtin.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/ext-builtin.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::rcl::error );

namespace rcl {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[ext-builtin] int" ) {
  REQUIRE(
      convert_to<int>( value{ null } ) ==
      error( "cannot convert value of type null to int."s ) );
  REQUIRE( convert_to<int>( value{ 5 } ) == 5 );
  REQUIRE(
      convert_to<int>( value{ true } ) ==
      error( "cannot convert value of type bool to int."s ) );
  REQUIRE(
      convert_to<int>( value{ 5.5 } ) ==
      error( "cannot convert value of type double to int."s ) );
  REQUIRE(
      convert_to<int>( value{ string( "hello" ) } ) ==
      error( "cannot convert value of type string to int."s ) );
  REQUIRE(
      convert_to<int>( value{ make_unique<table>() } ) ==
      error( "cannot convert value of type table to int."s ) );
  REQUIRE(
      convert_to<int>( value{ make_unique<list>() } ) ==
      error( "cannot convert value of type list to int."s ) );
}

TEST_CASE( "[ext-builtin] double" ) {
  REQUIRE(
      convert_to<double>( value{ null } ) ==
      error( "cannot convert value of type null to double."s ) );
  REQUIRE( convert_to<double>( value{ 5 } ) == 5.0 );
  REQUIRE(
      convert_to<double>( value{ true } ) ==
      error( "cannot convert value of type bool to double."s ) );
  REQUIRE( convert_to<double>( value{ 5.5 } ) == 5.5 );
  REQUIRE(
      convert_to<double>( value{ string( "hello" ) } ) ==
      error(
          "cannot convert value of type string to double."s ) );
  REQUIRE(
      convert_to<double>( value{ make_unique<table>() } ) ==
      error(
          "cannot convert value of type table to double."s ) );
  REQUIRE(
      convert_to<double>( value{ make_unique<list>() } ) ==
      error( "cannot convert value of type list to double."s ) );
}

TEST_CASE( "[ext-builtin] bool" ) {
  REQUIRE(
      convert_to<bool>( value{ null } ) ==
      error( "cannot convert value of type null to bool."s ) );
  REQUIRE(
      convert_to<bool>( value{ 5 } ) ==
      error( "cannot convert value of type int to bool."s ) );
  REQUIRE( convert_to<bool>( value{ true } ) == true );
  REQUIRE( convert_to<bool>( value{ false } ) == false );
  REQUIRE(
      convert_to<bool>( value{ 5.5 } ) ==
      error( "cannot convert value of type double to bool."s ) );
  REQUIRE(
      convert_to<bool>( value{ string( "hello" ) } ) ==
      error( "cannot convert value of type string to bool."s ) );
  REQUIRE(
      convert_to<bool>( value{ make_unique<table>() } ) ==
      error( "cannot convert value of type table to bool."s ) );
  REQUIRE(
      convert_to<bool>( value{ make_unique<list>() } ) ==
      error( "cannot convert value of type list to bool."s ) );
}

} // namespace
} // namespace rcl
