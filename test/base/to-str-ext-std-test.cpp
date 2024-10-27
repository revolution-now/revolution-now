/****************************************************************
**to-str-ext-std-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-27.
*
* Description: Unit tests for the base/to-str-ext-std module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[to-str-ext-std] vocab types" ) {
  // source_location
  REQUIRE_THAT( to_str( source_location::current() ),
                Catch::Matches(
                    ".*/test/base/"
                    "to-str-ext-std-test.cpp:[0-9]+:[0-9]+" ) );

  // string
  REQUIRE( to_str( ""s ) == "" );
  REQUIRE( to_str( "x"s ) == "x" );
  REQUIRE( to_str( "hello world"s ) == "hello world" );

  // string_view
  REQUIRE( to_str( string_view{ "" } ) == "" );
  REQUIRE( to_str( string_view{ "x" } ) == "x" );
  REQUIRE( to_str( string_view{ "hello world" } ) ==
           "hello world" );

  // fs::path
  REQUIRE( to_str( fs::path{ "" } ) == "" );
  REQUIRE( to_str( fs::path{ "a" } ) == "a" );
  REQUIRE( to_str( fs::path{ "/" } ) == "/" );
  REQUIRE( to_str( fs::path{ "///" } ) == "///" );
  REQUIRE( to_str( fs::path{ "/a//" } ) == "/a//" );
  REQUIRE( to_str( fs::path{ "/aaa/bb/" } ) == "/aaa/bb/" );
  REQUIRE( to_str( fs::path{ "/aaa/bb/ccc" } ) ==
           "/aaa/bb/ccc" );
  REQUIRE( to_str( fs::path{ "/aaa/../ccc" } ) ==
           "/aaa/../ccc" );

  // nullptr_t
  REQUIRE( to_str( nullptr ) == "nullptr" );

  // monostate
  REQUIRE( to_str( monostate{} ) == "monostate" );
}

} // namespace
} // namespace base
