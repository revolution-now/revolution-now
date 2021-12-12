/****************************************************************
**image.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-11.
*
* Description: Unit tests for the src/stb/image.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/stb/image.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace stb {
namespace {

using namespace std;

using ::testing::data_dir;

TEST_CASE( "[image] load png" ) {
  gfx::image im =
      load_image( data_dir() / "images" / "64w_x_32h.png" );

  REQUIRE( im.size_bytes() == 64 * 32 * 4 );
  REQUIRE( im.total_pixels() == 64 * 32 );
  REQUIRE( im.width_pixels() == 64 );
  REQUIRE( im.height_pixels() == 32 );

  // Check that there are some pixels with alpha == 0 and others
  // with alpha > 0.
  bool found_zero_alpha    = false;
  bool found_nonzero_alpha = false;
  for( int y = 0; y < im.height_pixels(); ++y ) {
    for( int x = 0; x < im.width_pixels(); ++x ) {
      gfx::pixel p = im.get( y, x );
      if( p.a == 0 )
        found_zero_alpha = true;
      else
        found_nonzero_alpha = true;
      if( found_zero_alpha && found_nonzero_alpha )
        goto finished;
    }
  }
finished:
  REQUIRE( found_zero_alpha );
  REQUIRE( found_nonzero_alpha );
}

} // namespace
} // namespace stb
