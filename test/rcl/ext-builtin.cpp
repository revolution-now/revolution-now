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

namespace rcl {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[ext-builtin] int" ) {
  REQUIRE(
      convert_to<int>( value{ null } ) ==
      error( "cannot produce int from value of type null."s ) );
  REQUIRE( convert_to<int>( value{ 5 } ) == 5 );
  REQUIRE(
      convert_to<int>( value{ true } ) ==
      error( "cannot produce int from value of type bool."s ) );
  REQUIRE(
      convert_to<int>( value{ 5.5 } ) ==
      error(
          "cannot produce int from value of type double."s ) );
  REQUIRE(
      convert_to<int>( value{ string( "hello" ) } ) ==
      error(
          "cannot produce int from value of type string."s ) );
  REQUIRE(
      convert_to<int>( value{ make_unique<table>() } ) ==
      error( "cannot produce int from value of type table."s ) );
  REQUIRE(
      convert_to<int>( value{ make_unique<list>() } ) ==
      error( "cannot produce int from value of type list."s ) );
}

TEST_CASE( "[ext-builtin] double" ) {
  REQUIRE(
      convert_to<double>( value{ null } ) ==
      error(
          "cannot produce double from value of type null."s ) );
  REQUIRE( convert_to<double>( value{ 5 } ) == 5.0 );
  REQUIRE(
      convert_to<double>( value{ true } ) ==
      error(
          "cannot produce double from value of type bool."s ) );
  REQUIRE( convert_to<double>( value{ 5.5 } ) == 5.5 );
  REQUIRE(
      convert_to<double>( value{ string( "hello" ) } ) ==
      error(
          "cannot produce double from value of type string."s ) );
  REQUIRE(
      convert_to<double>( value{ make_unique<table>() } ) ==
      error(
          "cannot produce double from value of type table."s ) );
  REQUIRE(
      convert_to<double>( value{ make_unique<list>() } ) ==
      error(
          "cannot produce double from value of type list."s ) );
}

TEST_CASE( "[ext-builtin] bool" ) {
  REQUIRE(
      convert_to<bool>( value{ null } ) ==
      error( "cannot produce bool from value of type null."s ) );
  REQUIRE(
      convert_to<bool>( value{ 5 } ) ==
      error( "cannot produce bool from value of type int."s ) );
  REQUIRE( convert_to<bool>( value{ true } ) == true );
  REQUIRE( convert_to<bool>( value{ false } ) == false );
  REQUIRE(
      convert_to<bool>( value{ 5.5 } ) ==
      error(
          "cannot produce bool from value of type double."s ) );
  REQUIRE(
      convert_to<bool>( value{ string( "hello" ) } ) ==
      error(
          "cannot produce bool from value of type string."s ) );
  REQUIRE(
      convert_to<bool>( value{ make_unique<table>() } ) ==
      error(
          "cannot produce bool from value of type table."s ) );
  REQUIRE(
      convert_to<bool>( value{ make_unique<list>() } ) ==
      error( "cannot produce bool from value of type list."s ) );
}

} // namespace
} // namespace rcl
