/****************************************************************
**enum-map.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-10.
*
* Description: Unit tests for the src/enum-map.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/enum-map.hpp"

// Rds
#include "rds/helper/rcl.hpp"
#include "rds/testing.hpp"

// Rcl
#include "rcl/ext-std.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::Catch::Matches;

// If this changes then the below tests may need to be updated.
static_assert( refl::enum_count<e_color> == 3 );

static_assert( is_default_constructible_v<
               ExhaustiveEnumMap<e_color, int>> );
static_assert(
    is_move_constructible_v<ExhaustiveEnumMap<e_color, int>> );
static_assert(
    is_move_assignable_v<ExhaustiveEnumMap<e_color, int>> );
static_assert( is_nothrow_move_constructible_v<
               ExhaustiveEnumMap<e_color, int>> );
static_assert( is_nothrow_move_assignable_v<
               ExhaustiveEnumMap<e_color, int>> );

TEST_CASE( "[enum-map] ExhaustiveEnumMap empty" ) {
  ExhaustiveEnumMap<e_empty, int> m;
  static_assert( m.kSize == 0 );
}

TEST_CASE(
    "[enum-map] ExhaustiveEnumMap non-primitive construction" ) {
  ExhaustiveEnumMap<e_color, string> m;
  static_assert( m.kSize == 3 );
  REQUIRE( m[e_color::red] == "" );
  REQUIRE( m[e_color::green] == "" );
  REQUIRE( m[e_color::blue] == "" );
}

TEST_CASE(
    "[enum-map] ExhaustiveEnumMap primitive initialization" ) {
  ExhaustiveEnumMap<e_color, int> m;
  static_assert( m.kSize == 3 );
  REQUIRE( m[e_color::red] == 0 );
  REQUIRE( m[e_color::green] == 0 );
  REQUIRE( m[e_color::blue] == 0 );
}

TEST_CASE( "[enum-map] ExhaustiveEnumMap indexing" ) {
  ExhaustiveEnumMap<e_color, int> m;
  m.at( e_color::red ) = 5;
  REQUIRE( m[e_color::red] == 5 );
  m[e_color::green] = 7;
  REQUIRE( m.at( e_color::green ) == 7 );
  ExhaustiveEnumMap<e_color, int> const m_const;
  REQUIRE( m_const[e_color::red] == 0 );
  REQUIRE( m_const.at( e_color::green ) == 0 );
}

TEST_CASE( "[enum-map] ExhaustiveEnumMap find" ) {
  ExhaustiveEnumMap<e_color, int> m;
  m[e_color::green] = 7;
  auto it           = m.find( e_color::green );
  REQUIRE( it != m.end() );
  REQUIRE( it->first == e_color::green );
  REQUIRE( it->second == 7 );
}

TEST_CASE( "[enum-map] ExhaustiveEnumMap equality" ) {
  ExhaustiveEnumMap<e_color, int> m1;
  ExhaustiveEnumMap<e_color, int> m2;
  REQUIRE( m1 == m2 );
  REQUIRE( m1[e_color::blue] == 0 );
  REQUIRE( m1 == m2 );
  m1[e_color::blue] = 5;
  REQUIRE( m1 != m2 );
  m1[e_color::blue] = 0;
  REQUIRE( m1 == m2 );
  m1[e_color::red] = 2;
  REQUIRE( m1 != m2 );
  m2 = m1;
  REQUIRE( m1 == m2 );
  REQUIRE( m1[e_color::red] == 2 );
  REQUIRE( m1[e_color::green] == 0 );
  REQUIRE( m1[e_color::blue] == 0 );
}

TEST_CASE( "[enum-map] ExhaustiveEnumMap Rcl" ) {
  using KV = rcl::table::value_type;
  rcl::table t =
      rcl::make_table( KV{ "red", "one" }, KV{ "green", "two" },
                       KV{ "blue", "three" } );
  UNWRAP_CHECK( ppt, rcl::run_postprocessing( std::move( t ) ) );
  rcl::value v{
      std::make_unique<rcl::table>( std::move( ppt ) ) };

  // Test.
  ExhaustiveEnumMap<e_color, string> expected{
      { e_color::red, "one" },
      { e_color::green, "two" },
      { e_color::blue, "three" } };
  REQUIRE( rcl::convert_to<ExhaustiveEnumMap<e_color, string>>(
               v ) == expected );
}

TEST_CASE( "[enum-map] ExhaustiveEnumMap Rcl e_empty" ) {
  rcl::table t = rcl::make_table();
  UNWRAP_CHECK( ppt, rcl::run_postprocessing( std::move( t ) ) );
  rcl::value v{
      std::make_unique<rcl::table>( std::move( ppt ) ) };

  // Test.
  ExhaustiveEnumMap<e_empty, string> expected;
  REQUIRE( ( rcl::convert_to<ExhaustiveEnumMap<e_empty, string>>(
                 v ) == expected ) );
}

TEST_CASE( "[enum-map] ExhaustiveEnumMap Rcl bad enum" ) {
  using KV = rcl::table::value_type;
  rcl::table t =
      rcl::make_table( KV{ "red", "one" }, KV{ "greenx", "two" },
                       KV{ "blue", "three" } );
  UNWRAP_CHECK( ppt, rcl::run_postprocessing( std::move( t ) ) );
  rcl::value v{
      std::make_unique<rcl::table>( std::move( ppt ) ) };

  // Test.
  REQUIRE(
      rcl::convert_to<ExhaustiveEnumMap<e_color, string>>( v ) ==
      rcl::error(
          "unrecognized value for enum e_color: \"greenx\"" ) );
}

} // namespace
} // namespace rn
