/****************************************************************
**noise.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-21.
*
* Description: Creates the noise texture.
*
*****************************************************************/
#include "noise.hpp"

// rand
#include "rand/random.hpp"

using namespace std;

namespace rr {

namespace {

using ::gfx::image;
using ::gfx::pixel;
using ::gfx::size;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
image create_noise_image( size const sz ) {
  int const num_pixels = sz.area();
  int const num_bytes  = num_pixels * image::kBytesPerPixel;
  auto* const data =
      static_cast<unsigned char*>( ::malloc( num_bytes ) );
  image res( sz, data );
  span<pixel> const sp = res;
  rng::random rd;
  for( pixel& p : sp ) {
    p.r = rd.uniform( 0, 255 );
    p.g = rd.uniform( 0, 255 );
    p.b = rd.uniform( 0, 255 );
    p.a = 255;
  }
  return res;
}

} // namespace rr
