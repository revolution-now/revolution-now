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

TEST_CASE( "[rng/perlin] perlin_noise_2d" ) {
  // TODO
}

} // namespace
} // namespace rng
