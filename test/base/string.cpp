/****************************************************************
**string.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-28.
*
* Description: Unit tests for the src/base/string.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/string.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[string] trim" ) {
  REQUIRE( trim( "" ) == "" );
  REQUIRE( trim( " " ) == "" );
  REQUIRE( trim( "x" ) == "x" );
  REQUIRE( trim( " x" ) == "x" );
  REQUIRE( trim( "x " ) == "x" );
  REQUIRE( trim( " x " ) == "x" );
  REQUIRE( trim( " bbx " ) == "bbx" );
  REQUIRE( trim( " hello world " ) == "hello world" );
  REQUIRE( trim( "hello world" ) == "hello world" );
  REQUIRE( trim( "one    two three " ) == "one    two three" );
}

TEST_CASE( "[string] capitalize_initials" ) {
  REQUIRE( capitalize_initials( "" ) == "" );
  REQUIRE( capitalize_initials( " " ) == " " );
  REQUIRE( capitalize_initials( "   " ) == "   " );
  REQUIRE( capitalize_initials( "  a" ) == "  A" );
  REQUIRE( capitalize_initials( "  A" ) == "  A" );
  REQUIRE( capitalize_initials( "a" ) == "A" );
  REQUIRE( capitalize_initials( "A" ) == "A" );
  REQUIRE( capitalize_initials( "A" ) == "A" );
  REQUIRE( capitalize_initials( "a " ) == "A " );
  REQUIRE( capitalize_initials( "A " ) == "A " );
  REQUIRE( capitalize_initials( "A " ) == "A " );
  REQUIRE( capitalize_initials( "this is a test" ) ==
           "This Is A Test" );
  REQUIRE( capitalize_initials( "this Is" ) == "This Is" );
  REQUIRE( capitalize_initials( " this Is" ) == " This Is" );
}

TEST_CASE( "[string] str_replace" ) {
  SECTION( "input.size() == 0" ) {
    string const input = "";
    auto         f = [&]( string_view from, string_view to ) {
      return str_replace( input, from, to );
    };

    REQUIRE( f( "", "" ) == "" );
    REQUIRE( f( "a", "" ) == "" );
    REQUIRE( f( "aaa", "" ) == "" );
    REQUIRE( f( "bbb", "aaa" ) == "" );
    REQUIRE( f( "", "aaa" ) == "" );
  }
  SECTION( "input.size() == 1" ) {
    string const input = "x";
    auto         f = [&]( string_view from, string_view to ) {
      return str_replace( input, from, to );
    };

    REQUIRE( f( "a", "" ) == "x" );
    REQUIRE( f( "aaa", "" ) == "x" );
    REQUIRE( f( "x", "" ) == "" );
    REQUIRE( f( "x", "abc" ) == "abc" );
    REQUIRE( f( "xx", "abc" ) == "x" );
    REQUIRE( f( "x", "x" ) == "x" );
  }
  SECTION( "input.size() == 2" ) {
    string const input = "xx";
    auto         f = [&]( string_view from, string_view to ) {
      return str_replace( input, from, to );
    };

    REQUIRE( f( "a", "" ) == "xx" );
    REQUIRE( f( "aaa", "" ) == "xx" );
    REQUIRE( f( "x", "" ) == "" );
    REQUIRE( f( "x", "abc" ) == "abcabc" );
    REQUIRE( f( "xx", "abc" ) == "abc" );
    REQUIRE( f( "xxx", "y" ) == "xx" );
  }
  SECTION( "input.size() == 5" ) {
    string const input = "abcbc";
    auto         f = [&]( string_view from, string_view to ) {
      return str_replace( input, from, to );
    };

    REQUIRE( f( "abc", "a" ) == "abc" );
    REQUIRE( f( "", "a" ) == "abcbc" );
    REQUIRE( f( "bc", "y" ) == "ayy" );
  }
  SECTION( "input.size() == 20" ) {
    string const input = "this is a test";
    auto         f = [&]( string_view from, string_view to ) {
      return str_replace( input, from, to );
    };

    REQUIRE( f( "testa", "y" ) == "this is a test" );
    REQUIRE( f( "thisa", "y" ) == "this is a test" );
    REQUIRE( f( "xthis", "y" ) == "this is a test" );
    REQUIRE( f( "this ", "y" ) == "yis a test" );
    REQUIRE( f( "this is a tes", "y" ) == "yt" );
    REQUIRE( f( "his is a test", "y" ) == "ty" );
    REQUIRE( f( "is a ", "yyy" ) == "this yyytest" );
  }
}

TEST_CASE( "[string] str_replace_all" ) {
  string const input = "this is a test bbaabb";

  REQUIRE( str_replace_all( input, {} ) == input );
  REQUIRE( str_replace_all( input, { { " ", "_" } } ) ==
           "this_is_a_test_bbaabb" );
  REQUIRE( str_replace_all(
               input, { { " ", "_" }, { "aabb", "xx" } } ) ==
           "this_is_a_test_bbxx" );
  REQUIRE( str_replace_all( input, { { " ", "_" },
                                     { "aabb", "xx" },
                                     { "bbxx", "bbxx" } } ) ==
           "this_is_a_test_bbxx" );
  REQUIRE( str_replace_all( input, { { " ", "_" },
                                     { "aabb", "xx" },
                                     { "bbxx", " " } } ) ==
           "this_is_a_test_ " );
}

} // namespace
} // namespace base
