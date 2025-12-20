/****************************************************************
**vocab.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: Unit tests for the src/vocab.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/vocab.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

TEST_CASE( "[vocab] NoCopy int" ) {
  NoCopy<int> nc( 5 );
  REQUIRE( nc == 5 );
  // NoCopy<int> nc2 = nc;
  static_assert( !is_copy_constructible_v<NoCopy<int>> );
  NoCopy<int> nc2 = std::move( nc );
  REQUIRE( nc2 == 5 );
  // int x = nc2;
  static_assert( is_convertible_v<NoCopy<int> const&, int> );
  static_assert( is_convertible_v<NoCopy<int>&, int> );
  static_assert( is_convertible_v<NoCopy<int> const, int> );
  static_assert( is_convertible_v<NoCopy<int> const&&, int> );
  int x = std::move( nc2 );
  static_assert( is_convertible_v<NoCopy<int>, int> );
  static_assert( is_convertible_v<NoCopy<int>&&, int> );
  REQUIRE( x == 5 );
}

TEST_CASE( "[vocab] NoCopy string" ) {
  NoCopy<string> nc( "5" );
  REQUIRE( nc == "5" );
  // NoCopy<string> nc2 = nc;
  static_assert( !is_copy_constructible_v<NoCopy<string>> );
  NoCopy<string> nc2 = std::move( nc );
  REQUIRE( nc2 == "5" );
  // string x = nc2;
  static_assert(
      is_convertible_v<NoCopy<string> const&, string> );
  static_assert( is_convertible_v<NoCopy<string>&, string> );
  static_assert(
      is_convertible_v<NoCopy<string> const, string> );
  static_assert(
      is_convertible_v<NoCopy<string> const&&, string> );
  string x = std::move( nc2 );
  static_assert( is_convertible_v<NoCopy<string>, string> );
  static_assert( is_convertible_v<NoCopy<string>&&, string> );
  REQUIRE( x == "5" );
}

TEST_CASE( "[vocab] NoCopy NoCopy" ) {
  NoCopy<NoCopy<string>> nc( string( "5" ) );
  REQUIRE( nc == "5" );
  // NoCopy<NoCopy<string>> nc2 = nc;
  static_assert(
      !is_copy_constructible_v<NoCopy<NoCopy<string>>> );
  NoCopy<NoCopy<string>> nc2 = std::move( nc );
  REQUIRE( nc2 == "5" );
  // NoCopy<string> x = nc2;
  static_assert( !is_convertible_v<NoCopy<NoCopy<string>> const&,
                                   NoCopy<string>> );
  static_assert( !is_convertible_v<NoCopy<NoCopy<string>>&,
                                   NoCopy<string>> );
  static_assert( !is_convertible_v<NoCopy<NoCopy<string>> const,
                                   NoCopy<string>> );
  static_assert(
      !is_convertible_v<NoCopy<NoCopy<string>> const&&,
                        NoCopy<string>> );
  NoCopy<string> x = std::move( nc2 );
  static_assert(
      is_convertible_v<NoCopy<NoCopy<string>>, NoCopy<string>> );
  static_assert( is_convertible_v<NoCopy<NoCopy<string>>&&,
                                  NoCopy<string>> );
  REQUIRE( x == "5" );
}

} // namespace
} // namespace base
