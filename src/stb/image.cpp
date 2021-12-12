/****************************************************************
**image.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-04.
*
* Description: C++ wrapper around stb_image.
*
*****************************************************************/
#include "image.hpp"

// base
#include "base/error.hpp"

// stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

namespace stb {

namespace {

[[noreturn]] void die_with_stbi_error() {
  char const* msg = stbi_failure_reason();
  if( msg == nullptr )
    msg =
        "stb_image encountered error but did not supply a "
        "reason.";
  FATAL( "{}", msg );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
gfx::image load_image( fs::path const& p ) {
  int width_pixels = 0, height_pixels = 0;
  // Normally the value here should be 4, meaning RGBA. But it
  // may be less if the image file does not contain e.g. alpha.
  // But either way, the result we get will have four bytes per
  // pixel because we are requesting it below.
  int            num_channels = 0;
  unsigned char* data =
      ::stbi_load( p.c_str(), &width_pixels, &height_pixels,
                   &num_channels, gfx::image::kBytesPerPixel );
  if( data == nullptr ) die_with_stbi_error();
  return gfx::image( width_pixels, height_pixels, data );
}

} // namespace stb
