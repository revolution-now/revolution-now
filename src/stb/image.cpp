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
** image
*****************************************************************/
image::image( int width_pixels, int height_pixels,
              unsigned char* data )
  : base::zero<image, unsigned char*>( data ),
    width_pixels_( width_pixels ),
    height_pixels_( height_pixels ) {}

pixel image::get( int y, int x ) const {
  unsigned char const* pixel_start =
      data() + ( y * width_pixels_ + x );
  return pixel{
      .r = pixel_start[0],
      .g = pixel_start[1],
      .b = pixel_start[2],
      .a = pixel_start[3],
  };
}

int image::height_pixels() const { return height_pixels_; }

int image::width_pixels() const { return width_pixels_; }

int image::size_bytes() const {
  return total_pixels() * kBytesPerPixel;
}

int image::total_pixels() const {
  return width_pixels_ * height_pixels_;
}

unsigned char* image::data() const { return resource(); }

image::operator span<byte const>() const {
  // Seems like this probably needs to hold to do what we're
  // about to do.
  static_assert( sizeof( std::byte ) ==
                 sizeof( unsigned char ) );
  byte const* p = reinterpret_cast<byte const*>( data() );
  return span<byte const>( p, size_bytes() );
}

image::operator span<char const>() const {
  char const* p = reinterpret_cast<char const*>( data() );
  return span<char const>( p, size_bytes() );
}

image::operator span<unsigned char const>() const {
  return span<unsigned char const>( data(), size_bytes() );
}

void image::free_resource() { stbi_image_free( resource() ); }

/****************************************************************
** Public API
*****************************************************************/
image load_image( fs::path const& p ) {
  int width_pixels = 0, height_pixels = 0;
  // Normally the value here should be 4, meaning RGBA. But it
  // may be less if the image file does not contain e.g. alpha.
  // But either way, the result we get will have four bytes per
  // pixel because we are requesting it below.
  int            num_channels = 0;
  unsigned char* data =
      stbi_load( p.c_str(), &width_pixels, &height_pixels,
                 &num_channels, image::kBytesPerPixel );
  if( data == nullptr ) die_with_stbi_error();
  return image( width_pixels, height_pixels, data );
}

} // namespace stb
