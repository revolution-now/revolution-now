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
  // NOTE: this generator won't be randomly seeded, so the output
  // will be deterministic, but that is what we want here, that
  // way we get something that looks like random noise but it
  // will actually be consistent across runs (and platforms as
  // well, since the marsenne twister that we use should have
  // that property).
  rng::random rd; // unseeded
  for( pixel& p : sp ) {
    p.r = rd.uniform<uint8_t>();
    p.g = rd.uniform<uint8_t>();
    p.b = rd.uniform<uint8_t>();
    p.a = 255;
  }
  return res;
}

} // namespace rr
