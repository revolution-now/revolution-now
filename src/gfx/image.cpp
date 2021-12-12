/****************************************************************
**image.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-11.
*
* Description: Image representation.
*
*****************************************************************/
#include "image.hpp"

using namespace std;

namespace gfx {

/****************************************************************
** image
*****************************************************************/
image::image( int width_pixels, int height_pixels,
              unsigned char* data )
  : base::zero<image, unsigned char*>( data ),
    width_pixels_( width_pixels ),
    height_pixels_( height_pixels ) {}

gfx::pixel image::get( int y, int x ) const {
  unsigned char const* pixel_start =
      data() + ( y * width_pixels_ + x ) * kBytesPerPixel;
  return gfx::pixel(
      /*r=*/pixel_start[0],
      /*g=*/pixel_start[1],
      /*b=*/pixel_start[2],
      /*a=*/pixel_start[3] );
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

image::operator span<pixel const>() const {
  pixel const* p = reinterpret_cast<pixel const*>( data() );
  return span<pixel const>( p, total_pixels() );
}

void image::free_resource() { ::free( resource() ); }

/****************************************************************
** Helpers
*****************************************************************/
image empty_image( int width_pixels, int height_pixels ) {
  CHECK_GE( width_pixels, 1 );
  CHECK_GE( height_pixels, 1 );
  int size_bytes =
      width_pixels * height_pixels * image::kBytesPerPixel;
  unsigned char* data = (unsigned char*)::malloc( size_bytes );
  ::memset( data, 0, size_bytes );
  return image( width_pixels, height_pixels, data );
}

} // namespace gfx
