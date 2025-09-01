/****************************************************************
**generator-combinators-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-01.
*
* Description: Unit tests for the base/generator-combinators
*module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/generator-combinators.hpp"

// C++ standard library
#include <list>

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/generator-combinators] range_concat" ) {
  SECTION( "single" ) {
    vector<string> const rg = { "hello", "world" };

    vector<string> out;
    for( auto const& s : range_concat( rg ) ) out.push_back( s );

    REQUIRE( out == vector<string>{ "hello", "world" } );
  }

  SECTION( "multiple" ) {
    array<string, 3> const rg1{ "one", "two", "three" };
    vector<string> const rg2 = { "hello", "world" };
    list<string> const rg3   = { "one", "two", "three" };

    vector<string> out;
    for( auto const& s : range_concat( rg1, rg2, rg3 ) )
      out.push_back( s );

    REQUIRE( out == vector<string>{ "one", "two", "three",
                                    "hello", "world", "one",
                                    "two", "three" } );
  }

  SECTION( "double use" ) {
    array<string, 3> const rg{ "one", "two", "three" };

    vector<string> out;
    for( auto const& s : range_concat( rg, rg ) )
      out.push_back( s );

    REQUIRE( out == vector<string>{ "one", "two", "three", "one",
                                    "two", "three" } );
  }
}

} // namespace
} // namespace base
