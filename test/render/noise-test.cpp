/****************************************************************
**noise-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-21.
*
* Description: Unit tests for the render/noise module.
*
*****************************************************************/
#include "noise.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/render/noise.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rr {
namespace {

using namespace std;

using ::gfx::pixel;
using ::gfx::size;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[render/noise] create_noise_image" ) {
  auto const img = create_noise_image( { .w = 2, .h = 3 } );

  REQUIRE( img.size_pixels() == size{ .w = 2, .h = 3 } );
  REQUIRE( img.height_pixels() == 3 );
  REQUIRE( img.width_pixels() == 2 );
  REQUIRE( img.size_bytes() == 6 * 4 );
  REQUIRE( img.total_pixels() == 6 );

  span<pixel const> const sp = img;
  for( pixel const& p : sp ) { REQUIRE( p.a == 255 ); }
}

} // namespace
} // namespace rr
