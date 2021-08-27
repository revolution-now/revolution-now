/****************************************************************
**ext-std.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-25.
*
* Description: Unit tests for the src/rcl/ext-std.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/ext-builtin.hpp"
#include "src/rcl/ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::rcl::error );

namespace rcl {
namespace {

using namespace std;

using Catch::Contains;
using Catch::Equals;

TEST_CASE( "[ext-std] string" ) {
  REQUIRE(
      convert_to<string>( value{ null } ) ==
      error( "cannot convert value of type null to string."s ) );
  REQUIRE(
      convert_to<string>( value{ 5 } ) ==
      error( "cannot convert value of type int to string."s ) );
  REQUIRE(
      convert_to<string>( value{ true } ) ==
      error( "cannot convert value of type bool to string."s ) );
  REQUIRE(
      convert_to<string>( value{ 5.5 } ) ==
      error(
          "cannot convert value of type double to string."s ) );
  REQUIRE( convert_to<string>( value{ string( "hello" ) } ) ==
           "hello" );
  REQUIRE(
      convert_to<string>( value{ make_unique<table>() } ) ==
      error(
          "cannot convert value of type table to string."s ) );
  REQUIRE(
      convert_to<string>( value{ make_unique<list>() } ) ==
      error( "cannot convert value of type list to string."s ) );
}

TEST_CASE( "[ext-std] string_view" ) {
  REQUIRE(
      convert_to<string_view>( value{ null } ) ==
      error(
          "cannot convert value of type null to string_view."s ) );
  REQUIRE(
      convert_to<string_view>( value{ 5 } ) ==
      error(
          "cannot convert value of type int to string_view."s ) );
  REQUIRE(
      convert_to<string_view>( value{ true } ) ==
      error(
          "cannot convert value of type bool to string_view."s ) );
  REQUIRE(
      convert_to<string_view>( value{ 5.5 } ) ==
      error(
          "cannot convert value of type double to string_view."s ) );
  REQUIRE( convert_to<string_view>(
               value{ string( "hello" ) } ) == "hello" );
  REQUIRE(
      convert_to<string_view>( value{ make_unique<table>() } ) ==
      error(
          "cannot convert value of type table to string_view."s ) );
  REQUIRE(
      convert_to<string_view>( value{ make_unique<list>() } ) ==
      error(
          "cannot convert value of type list to string_view."s ) );
}

TEST_CASE( "[ext-std] fs::path" ) {
  REQUIRE(
      convert_to<fs::path>( value{ null } ) ==
      error(
          "cannot convert value of type null to file path."s ) );
  REQUIRE(
      convert_to<fs::path>( value{ 5 } ) ==
      error(
          "cannot convert value of type int to file path."s ) );
  REQUIRE(
      convert_to<fs::path>( value{ true } ) ==
      error(
          "cannot convert value of type bool to file path."s ) );
  REQUIRE(
      convert_to<fs::path>( value{ 5.5 } ) ==
      error(
          "cannot convert value of type double to file path."s ) );
  REQUIRE( convert_to<fs::path>( value{ string( "a/b/c" ) } ) ==
           fs::path( "a/b/c" ) );
  REQUIRE(
      convert_to<fs::path>( value{ make_unique<table>() } ) ==
      error(
          "cannot convert value of type table to file path."s ) );
  REQUIRE(
      convert_to<fs::path>( value{ make_unique<list>() } ) ==
      error(
          "cannot convert value of type list to file path."s ) );
}

TEST_CASE( "[ext-builtin] chrono::seconds" ) {
  REQUIRE(
      convert_to<chrono::seconds>( value{ null } ) ==
      error(
          "cannot convert value of type null to chrono::seconds."s ) );
  REQUIRE( convert_to<chrono::seconds>( value{ 5 } ) ==
           chrono::seconds{ 5 } );
  REQUIRE(
      convert_to<chrono::seconds>( value{ true } ) ==
      error(
          "cannot convert value of type bool to chrono::seconds."s ) );
  REQUIRE(
      convert_to<chrono::seconds>( value{ 5.5 } ) ==
      error(
          "cannot convert value of type double to chrono::seconds."s ) );
  REQUIRE(
      convert_to<chrono::seconds>(
          value{ string( "hello" ) } ) ==
      error(
          "cannot convert value of type string to chrono::seconds."s ) );
  REQUIRE(
      convert_to<chrono::seconds>(
          value{ make_unique<table>() } ) ==
      error(
          "cannot convert value of type table to chrono::seconds."s ) );
  REQUIRE(
      convert_to<chrono::seconds>(
          value{ make_unique<list>() } ) ==
      error(
          "cannot convert value of type list to chrono::seconds."s ) );
}

TEST_CASE( "[ext-builtin] std::pair" ) {
  using KV = table::value_type;
  UNWRAP_CHECK( tbl, run_postprocessing( make_table(
                         KV{ "key", 1.0 },
                         KV{ "val", string( "hello" ) } ) ) );
  value v{ std::make_unique<table>( std::move( tbl ) ) };

  // Test.
  REQUIRE( convert_to<pair<double, string>>( v ) ==
           pair<double, string>{ 1.0, "hello" } );
}

TEST_CASE( "[ext-builtin] std::vector" ) {
  using KV = table::value_type;
  list  l  = make_list( "hello", "world", "test" );
  value v{ std::make_unique<list>( std::move( l ) ) };

  // Test.
  vector<string> expected{ "hello", "world", "test" };
  REQUIRE( convert_to<vector<string>>( v ) == expected );
}

TEST_CASE( "[ext-builtin] std::vector with error" ) {
  using KV = table::value_type;
  list  l  = make_list( "hello", 5, "test" );
  value v{ std::make_unique<list>( std::move( l ) ) };

  // Test.
  REQUIRE(
      convert_to<vector<string>>( v ) ==
      error( "cannot convert value of type int to string." ) );
}

TEST_CASE( "[ext-builtin] std::vector<std::pair<...>>" ) {
  using KV = table::value_type;
  list l   = make_list(
        make_table_val( KV{ "key", "one" }, KV{ "val", 1 } ),
        make_table_val( KV{ "key", "two" }, KV{ "val", 2 } ) );
  UNWRAP_CHECK( ppl, run_postprocessing( std::move( l ) ) );
  value v{ std::make_unique<list>( std::move( ppl ) ) };

  // Test.
  vector<pair<string_view, int>> expected{ { "one", 1 },
                                           { "two", 2 } };
  REQUIRE( convert_to<vector<pair<string_view, int>>>( v ) ==
           expected );
}

TEST_CASE( "[ext-builtin] std::unordered_map" ) {
  using KV = table::value_type;
  list l   = make_list(
        make_table_val( KV{ "key", "one" }, KV{ "val", 1 } ),
        make_table_val( KV{ "key", "two" }, KV{ "val", 2 } ) );
  UNWRAP_CHECK( ppl, run_postprocessing( std::move( l ) ) );
  value v{ std::make_unique<list>( std::move( ppl ) ) };

  // Test.
  unordered_map<string, int> expected{ { "one", 1 },
                                       { "two", 2 } };
  REQUIRE( convert_to<unordered_map<string, int>>( v ) ==
           expected );
}

TEST_CASE( "[ext-builtin] std::unordered_map dupe key" ) {
  using KV = table::value_type;
  list l   = make_list(
        make_table_val( KV{ "key", "one" }, KV{ "val", 1 } ),
        make_table_val( KV{ "key", "two" }, KV{ "val", 2 } ),
        make_table_val( KV{ "key", "two" }, KV{ "val", 2 } ) );
  UNWRAP_CHECK( ppl, run_postprocessing( std::move( l ) ) );
  value v{ std::make_unique<list>( std::move( ppl ) ) };

  // Test.
  REQUIRE( convert_to<unordered_map<string, int>>( v ) ==
           error( "dictionary contains duplicate key `two'." ) );
}

} // namespace
} // namespace rcl
