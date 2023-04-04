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
#include "base/cc-specific.hpp"
#include "base/error.hpp"

// stb
#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Weverything"
#endif
#ifdef COMPILER_GCC
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#ifdef COMPILER_GCC
#  pragma GCC diagnostic pop
#endif
#ifdef __clang__
#  pragma clang diagnostic pop
#endif

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace stb {

namespace {

string get_stbi_error() {
  char const* msg = stbi_failure_reason();
  if( msg != nullptr ) return msg;
  return "stb_image encountered error but did not supply a "
         "reason.";
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
base::expect<gfx::image> load_image( fs::path const& p ) {
  int width_pixels = 0, height_pixels = 0;
  // Normally the value here should be 4, meaning RGBA. But it
  // may be less if the image file does not contain e.g. alpha.
  // But either way, the result we get will have four bytes per
  // pixel because we are requesting it below.
  int            num_channels = 0;
  unsigned char* data =
      ::stbi_load( p.c_str(), &width_pixels, &height_pixels,
                   &num_channels, gfx::image::kBytesPerPixel );
  if( data == nullptr )
    return fmt::format( "failed to open file {}: {}.", p,
                        get_stbi_error() );
  return gfx::image(
      gfx::size{ .w = width_pixels, .h = height_pixels }, data );
}

base::valid_or<std::string> save_image(
    fs::path const& p, gfx::image const& image ) {
  int const comp   = 4; // RGBA.
  int const result = ::stbi_write_png(
      p.c_str(), image.size_pixels().w, image.size_pixels().h,
      comp, image.data(), /*stride_bytes=*/0 );
  if( result == 0 )
    return fmt::format( "failed to write PNG file to {}.", p );
  return base::valid;
}

} // namespace stb
