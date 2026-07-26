/****************************************************************
**perlin-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-24.
*
* Description: Unit tests for the rng/perlin module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand/perlin.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rng {
namespace {

using namespace std;

using ::base::valid;
using ::Catch::Contains;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rng/perlin] PerlinFractalOptions" ) {
  PerlinFractalOptions o{
    .n_octaves   = 1,
    .persistence = 1.0,
    .lacunarity  = 1.0,
  };

  REQUIRE( o.validate() == valid );

  o.n_octaves = 0;
  REQUIRE_THAT( o.validate().error(),
                Contains( "n_octaves must be >= 1"s ) );
  o.n_octaves = 1;

  o.persistence = -0.01;
  REQUIRE_THAT( o.validate().error(),
                Contains( "persistence must be >= 0"s ) );

  o.persistence = 0.0;
  REQUIRE( o.validate() == valid );

  o.persistence = 0.0001;
  REQUIRE( o.validate() == valid );

  o.lacunarity = -0.01;
  REQUIRE_THAT( o.validate().error(),
                Contains( "lacunarity must be >= 0"s ) );

  o.lacunarity = 0.0;
  REQUIRE( o.validate() == valid );

  o.lacunarity = 0.0001;
  REQUIRE( o.validate() == valid );
}

// We don't really need ot test this exhaustively because it is
// fully covered by the perlin-map tests, but we'll just do a few
// basic tests with random inputs that cover all the code and
// should be sufficient to lock it in.
TEST_CASE( "[rng/perlin] perlin_noise_2d" ) {
  PerlinVec2 point;
  PerlinFractalOptions fractal_options;
  PerlinVec2 seamless_repeat;
  PerlinInt base = {};

  auto const f = [&] [[clang::noinline]] {
    return perlin_noise_2d( point, fractal_options,
                            seamless_repeat, base );
  };

  point           = { .x = 2.3, .y = 5.5 };
  fractal_options = {
    .n_octaves = 0, .persistence = 0.5, .lacunarity = 2.0 };
  seamless_repeat = { .x = 1000, .y = 1000 };
  base            = 0;
  REQUIRE( f() == 0 );

  point           = { .x = 2.3, .y = 5.5 };
  fractal_options = {
    .n_octaves = 5, .persistence = 0.5, .lacunarity = 2.0 };
  seamless_repeat = { .x = 1000, .y = 1000 };
  base            = 0;
  REQUIRE( f() == Approx( -0.0625450181 ).epsilon( 1e-9 ) );

  point           = { .x = 4.3, .y = 1.5 };
  fractal_options = {
    .n_octaves = 3, .persistence = 0.6, .lacunarity = 2.2 };
  seamless_repeat = { .x = 3.3, .y = 1.2 };
  base            = 1;
  REQUIRE( f() == Approx( -0.0152986129 ).epsilon( 1e-8 ) );
}

} // namespace
} // namespace rng
