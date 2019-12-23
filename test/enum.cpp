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
#include "fmt-helper.hpp"

// magic_enum
#include "magic_enum.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;
using namespace rn;

enum class e_test_enum { red, green, blue };

TEST_CASE( "[enum] magic_enum config." ) {
  static_assert( MAGIC_ENUM_RANGE_MIN <= 0 );
  static_assert( MAGIC_ENUM_RANGE_MAX == 512 );
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
  e141,e142,e143,e144,e145,e146,e147,e148,e149,e150,e151,e152,e153,e154,e155,e156,e157,e158,e159,e160,
  e161,e162,e163,e164,e165,e166,e167,e168,e169,e170,e171,e172,e173,e174,e175,e176,e177,e178,e179,e180,
  e181,e182,e183,e184,e185,e186,e187,e188,e189,e190,e191,e192,e193,e194,e195,e196,e197,e198,e199,e200,
  e201,e202,e203,e204,e205,e206,e207,e208,e209,e210,e211,e212,e213,e214,e215,e216,e217,e218,e219,e220,
  e221,e222,e223,e224,e225,e226,e227,e228,e229,e230,e231,e232,e233,e234,e235,e236,e237,e238,e239,e240,
  e241,e242,e243,e244,e245,e246,e247,e248,e249,e250,e251,e252,e253,e254,e255,e256,e257,e258,e259,e260,
  e261,e262,e263,e264,e265,e266,e267,e268,e269,e270,e271,e272,e273,e274,e275,e276,e277,e278,e279,e280,
  e281,e282,e283,e284,e285,e286,e287,e288,e289,e290,e291,e292,e293,e294,e295,e296,e297,e298,e299,e300,
  e301,e302,e303,e304,e305,e306,e307,e308,e309,e310,e311,e312,e313,e314,e315,e316,e317,e318,e319,e320,
  e321,e322,e323,e324,e325,e326,e327,e328,e329,e330,e331,e332,e333,e334,e335,e336,e337,e338,e339,e340,
  e341,e342,e343,e344,e345,e346,e347,e348,e349,e350,e351,e352,e353,e354,e355,e356,e357,e358,e359,e360,
  e361,e362,e363,e364,e365,e366,e367,e368,e369,e370,e371,e372,e373,e374,e375,e376,e377,e378,e379,e380,
  e381,e382,e383,e384,e385,e386,e387,e388,e389,e390,e391,e392,e393,e394,e395,e396,e397,e398,e399,e400,
  e401,e402,e403,e404,e405,e406,e407,e408,e409,e410,e411,e412,e413,e414,e415,e416,e417,e418,e419,e420,
  e421,e422,e423,e424,e425,e426,e427,e428,e429,e430,e431,e432,e433,e434,e435,e436,e437,e438,e439,e440,
  e441,e442,e443,e444,e445,e446,e447,e448,e449,e450,e451,e452,e453,e454,e455,e456,e457,e458,e459,e460,
  e461,e462,e463,e464,e465,e466,e467,e468,e469,e470,e471,e472,e473,e474,e475,e476,e477,e478,e479,e480,
  e481,e482,e483,e484,e485,e486,e487,e488,e489,e490,e491,e492,e493,e494,e495,e496,e497,e498,e499,e500,
  e501,e502,e503,e504,e505,e506,e507,e508,e509,e510,e511,e512,e513,e514,e515,e516,e517,e518,e519,e520
};
// clang-format on

TEST_CASE( "[enum] magic_enum large enum." ) {
  using namespace magic_enum;
  // Value to name.
  static_assert( enum_name( e_large_enum::e001 ) == "e001" );
  static_assert( enum_name( e_large_enum::e512 ) == "e512" );
  static_assert( enum_name( e_large_enum::e513 ) == "e513" );
  // The point at which names stop working is set by the compiler
  // define MAGIC_ENUM_RANGE_MAX.
  static_assert( enum_name( e_large_enum::e514 ) == "" );

  // Name to value.
  static_assert( enum_cast<e_large_enum>( "e349" ) ==
                 e_large_enum::e349 );
  static_assert(
      !enum_cast<e_large_enum>( "xxxx" ).has_value() );

  // Values.
  constexpr auto values =
      magic_enum::enum_values<e_large_enum>();
  static_assert( values.size() == MAGIC_ENUM_RANGE_MAX + 1 );
  static_assert( values[0] == e_large_enum::e001 );
  static_assert( values[1] == e_large_enum::e002 );
  static_assert( values[511] == e_large_enum::e512 );
  static_assert( values[512] == e_large_enum::e513 );

  // Size.
  static_assert( magic_enum::enum_count<e_large_enum>() ==
                 MAGIC_ENUM_RANGE_MAX + 1 );
}

TEST_CASE( "[enum] fmt" ) {
  REQUIRE( fmt::format( "{}", e_test_enum::red ) == "red" );
  REQUIRE( fmt::format( "{}", e_test_enum::green ) == "green" );
  REQUIRE( fmt::format( "{}", e_test_enum::blue ) == "blue" );

  REQUIRE( fmt::format( "{}", e_large_enum::e001 ) == "e001" );
  REQUIRE( fmt::format( "{}", e_large_enum::e002 ) == "e002" );
  REQUIRE( fmt::format( "{}", e_large_enum::e512 ) == "e512" );
  REQUIRE( fmt::format( "{}", e_large_enum::e513 ) == "e513" );
}

} // namespace
} // namespace rn
