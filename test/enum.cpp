/****************************************************************
**enum.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-22.
*
* Description: Unit tests for enums and enum reflection.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "enum.hpp"
#include "fmt-helper.hpp"

// magic_enum
#include "magic_enum.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

enum class e_test_enum { red, green, blue };

TEST_CASE( "[enum] magic_enum config." ) {
  static_assert( MAGIC_ENUM_RANGE_MIN <= 0 );
  static_assert( MAGIC_ENUM_RANGE_MAX == 128 );
}

TEST_CASE( "[enum] magic_enum compile-time stuff." ) {
  using namespace magic_enum;
  // Value to name.
  static_assert( enum_name( e_test_enum::green ) == "green" );

  // Name to value.
  static_assert( enum_cast<e_test_enum>( "blue" ) ==
                 e_test_enum::blue );
  static_assert(
      !enum_cast<e_test_enum>( "bluex" ).has_value() );

  // Values.
  constexpr auto values = enum_values<e_test_enum>();
  static_assert( values.size() == 3 );
  static_assert( values[0] == e_test_enum::red );
  static_assert( values[1] == e_test_enum::green );
  static_assert( values[2] == e_test_enum::blue );

  // Size.
  static_assert( enum_count<e_test_enum>() == 3 );
}

// The below enum is supposed to contain more elements than the
// max amount that magic_enum can process, and this should be
// verified by one of the unit tests below.
// clang-format off
enum class e_large_enum {
  e001,e002,e003,e004,e005,e006,e007,e008,e009,e010,e011,e012,e013,e014,e015,e016,e017,e018,e019,e020,
  e021,e022,e023,e024,e025,e026,e027,e028,e029,e030,e031,e032,e033,e034,e035,e036,e037,e038,e039,e040,
  e041,e042,e043,e044,e045,e046,e047,e048,e049,e050,e051,e052,e053,e054,e055,e056,e057,e058,e059,e060,
  e061,e062,e063,e064,e065,e066,e067,e068,e069,e070,e071,e072,e073,e074,e075,e076,e077,e078,e079,e080,
  e081,e082,e083,e084,e085,e086,e087,e088,e089,e090,e091,e092,e093,e094,e095,e096,e097,e098,e099,e100,
  e101,e102,e103,e104,e105,e106,e107,e108,e109,e110,e111,e112,e113,e114,e115,e116,e117,e118,e119,e120,
  e121,e122,e123,e124,e125,e126,e127,e128,e129,e130,e131,e132,e133,e134,e135,e136,e137,e138,e139,e140,
};
// clang-format on

TEST_CASE( "[enum] magic_enum large enum." ) {
  using namespace magic_enum;
  // Value to name.
  static_assert( enum_name( e_large_enum::e001 ) == "e001" );
  static_assert( enum_name( e_large_enum::e128 ) == "e128" );
  static_assert( enum_name( e_large_enum::e129 ) == "e129" );
  // The point at which names stop working is set by the compiler
  // define MAGIC_ENUM_RANGE_MAX.
  static_assert( enum_name( e_large_enum::e130 ) == "" );

  // Name to value.
  static_assert( enum_cast<e_large_enum>( "e100" ) ==
                 e_large_enum::e100 );
  static_assert(
      !enum_cast<e_large_enum>( "xxxx" ).has_value() );

  // Values.
  constexpr auto values =
      magic_enum::enum_values<e_large_enum>();
  static_assert( values.size() == MAGIC_ENUM_RANGE_MAX + 1 );
  static_assert( values[0] == e_large_enum::e001 );
  static_assert( values[1] == e_large_enum::e002 );
  static_assert( values[126] == e_large_enum::e127 );
  static_assert( values[127] == e_large_enum::e128 );
  static_assert( values[128] == e_large_enum::e129 );

  // Size.
  static_assert( magic_enum::enum_count<e_large_enum>() ==
                 MAGIC_ENUM_RANGE_MAX + 1 );
}

// This simulates a flatbuffers enum. We want to make sure that
// magic_enum handles it in the way we expect. The way it handles
// it happens to be convenient for us, since it effectively
// erases the MIN/MAX elements, which is exactly what we want
// since we don't need them anyway when we are using magic_enum.
enum class e_repeated {
  red   = 0,
  green = 1,
  blue  = 2,
  // Repeated values.
  MIN = 0,
  MAX = 2
};

TEST_CASE( "[enum] magic_enum repeated values." ) {
  using namespace magic_enum;
  // Value to name.
  static_assert( enum_name( e_repeated::red ) == "red" );
  static_assert( enum_name( e_repeated::green ) == "green" );
  static_assert( enum_name( e_repeated::blue ) == "blue" );
  static_assert( enum_name( e_repeated::MIN ) == "red" );
  static_assert( enum_name( e_repeated::MAX ) == "blue" );

  // Name to value.
  static_assert( enum_cast<e_repeated>( "red" ) ==
                 e_repeated::red );
  static_assert( enum_cast<e_repeated>( "green" ) ==
                 e_repeated::green );
  static_assert( enum_cast<e_repeated>( "blue" ) ==
                 e_repeated::blue );
  static_assert( !enum_cast<e_repeated>( "MIN" ).has_value() );
  static_assert( !enum_cast<e_repeated>( "MAX" ).has_value() );

  // Values.
  constexpr auto values = magic_enum::enum_values<e_repeated>();
  static_assert( values.size() == 3 );
  static_assert( values[0] == e_repeated::red );
  static_assert( values[1] == e_repeated::green );
  static_assert( values[2] == e_repeated::blue );

  // Size.
  static_assert( magic_enum::enum_count<e_repeated>() == 3 );
}

TEST_CASE( "[enum] fmt" ) {
  REQUIRE( fmt::format( "{}", e_test_enum::red ) == "red" );
  REQUIRE( fmt::format( "{}", e_test_enum::green ) == "green" );
  REQUIRE( fmt::format( "{}", e_test_enum::blue ) == "blue" );

  REQUIRE( fmt::format( "{}", e_large_enum::e001 ) == "e001" );
  REQUIRE( fmt::format( "{}", e_large_enum::e002 ) == "e002" );
  REQUIRE( fmt::format( "{}", e_large_enum::e128 ) == "e128" );
  REQUIRE( fmt::format( "{}", e_large_enum::e129 ) == "e129" );
}

#define TEST_NS( input, expected )                         \
  {                                                        \
    constexpr string_view sv = remove_namespaces( input ); \
    static_assert( sv == expected );                       \
  }

TEST_CASE( "[mining] remove_namespaces" ) {
  using internal::remove_namespaces;
  TEST_NS( "", "" );
  TEST_NS( "a", "a" );
  TEST_NS( "aaa", "aaa" );
  TEST_NS( "::", "" );
  TEST_NS( "ab::", "" );
  TEST_NS( "::ab", "ab" );
  TEST_NS( "::ab::cd", "cd" );
  TEST_NS( "ab::cd", "cd" );
  TEST_NS( "ab::cd::", "" );
  TEST_NS( "ab::cd::ef", "ef" );
}

} // namespace
} // namespace rn
