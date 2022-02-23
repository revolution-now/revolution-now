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
#include "rds/testing.rds.hpp"

// refl
#include "refl/to-str.hpp"

// cdr
#include "src/cdr/ext-std.hpp"
#include "src/cdr/ext.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::Catch::Matches;
using ::cdr::testing::conv_from_bt;

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

ExhaustiveEnumMap<e_color, string> const native_colors1{
    { e_color::red, "one" },
    /*{ e_color::green, "" },*/
    { e_color::blue, "three" },
};

cdr::value const cdr_colors1 = cdr::table{
    "red"_key   = "one",
    "green"_key = "",
    "blue"_key  = "three",
};

cdr::value const cdr_colors1_extra_field = cdr::table{
    "red"_key    = "one",
    "green"_key  = "",
    "blue"_key   = "three",
    "purple"_key = "four",
};

cdr::value const cdr_colors1_wrong_type = cdr::table{
    "red"_key   = "one",
    "green"_key = 5,
    "blue"_key  = "three",
};

cdr::value const cdr_colors1_missing_field = cdr::table{
    "red"_key  = "one",
    "blue"_key = "three",
};

TEST_CASE( "[enum-map] cdr/strict" ) {
  using M = ExhaustiveEnumMap<e_color, string>;
  cdr::converter conv{ {
      .write_fields_with_default_value  = true,
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = false,
  } };
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( native_colors1 ) == cdr_colors1 );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<M>( conv, cdr_colors1 ) ==
             native_colors1 );
    REQUIRE( conv.from<M>( 5 ) ==
             conv.err( "expected type table, instead found type "
                       "integer." ) );
    REQUIRE( conv.from<M>( cdr_colors1_wrong_type ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
    REQUIRE( conv.from<M>( cdr_colors1_extra_field ) ==
             conv.err( "unrecognized key 'purple' in table." ) );
    REQUIRE( conv.from<M>( cdr_colors1_missing_field ) ==
             conv.err( "key 'green' not found in table." ) );
  }
}

TEST_CASE( "[enum-map] cdr/no-defaults" ) {
  using M = ExhaustiveEnumMap<e_color, string>;
  cdr::converter conv{ {
      .write_fields_with_default_value  = false,
      .allow_unrecognized_fields        = true,
      .default_construct_missing_fields = true,
  } };
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( native_colors1 ) ==
             cdr_colors1_missing_field );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<M>( conv, cdr_colors1 ) ==
             native_colors1 );
    REQUIRE( conv.from<M>( 5 ) ==
             conv.err( "expected type table, instead found type "
                       "integer." ) );
    REQUIRE( conv.from<M>( cdr_colors1_wrong_type ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
    REQUIRE( conv.from<M>( cdr_colors1_extra_field ) ==
             native_colors1 );
    REQUIRE( conv.from<M>( cdr_colors1_missing_field ) ==
             native_colors1 );
  }
}

} // namespace
} // namespace rn
