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
image::image( size size_pixels, unsigned char* data )
  : base::zero<image, unsigned char*>( data ),
    size_pixels_( size_pixels ) {}

gfx::pixel const& image::at( point p ) const {
  DCHECK( rect_pixels().contains( p ) );
  // This reinterpret_cast should be ok because data() has type
  // `unsigned char*` which is allowed to alias anything.
  return *reinterpret_cast<pixel const*>(
      data() + ( p.y * size_pixels_.w + p.x ) * kBytesPerPixel );
}

gfx::pixel& image::at( point p ) {
  DCHECK( rect_pixels().contains( p ) );
  // This reinterpret_cast should be ok because data() has type
  // `unsigned char*` which is allowed to alias anything.
  return *reinterpret_cast<pixel*>(
      data() + ( p.y * size_pixels_.w + p.x ) * kBytesPerPixel );
}

gfx::pixel const& image::operator[]( point p ) const {
  return at( p );
}

gfx::pixel& image::operator[]( point p ) { return at( p ); }

size image::size_pixels() const { return size_pixels_; }

int image::height_pixels() const { return size_pixels_.h; }

int image::width_pixels() const { return size_pixels_.w; }

int image::size_bytes() const {
  return total_pixels() * kBytesPerPixel;
}

int image::total_pixels() const { return size_pixels_.area(); }

unsigned char* image::data() const { return resource(); }

image::operator span<byte const>() const {
  // Seems like this probably needs to hold to do what we're
  // about to do.
  static_assert( sizeof( std::byte ) ==
                 sizeof( unsigned char ) );
  // This reinterpret_cast should be ok because data() has type
  // `unsigned char*` which is allowed to alias anything.
  byte const* p = reinterpret_cast<byte const*>( data() );
  return span<byte const>( p, size_bytes() );
}

image::operator span<char const>() const {
  // This reinterpret_cast should be ok because data() has type
  // `unsigned char*` which is allowed to alias anything.
  char const* p = reinterpret_cast<char const*>( data() );
  return span<char const>( p, size_bytes() );
}

image::operator span<unsigned char const>() const {
  return span<unsigned char const>( data(), size_bytes() );
}

image::operator span<pixel const>() const {
  // This reinterpret_cast should be ok because data() has type
  // `unsigned char*` which is allowed to alias anything.
  pixel const* p = reinterpret_cast<pixel const*>( data() );
  static_assert( sizeof( pixel ) == kBytesPerPixel );
  return span<pixel const>( p, total_pixels() );
}

void image::free_resource() { ::free( resource() ); }

/****************************************************************
** Helpers
*****************************************************************/
image empty_image( size size_pixels ) {
  CHECK( !size_pixels.negative() );
  int size_bytes = size_pixels.area() * image::kBytesPerPixel;
  unsigned char* data = (unsigned char*)::malloc( size_bytes );
  ::memset( data, 0, size_bytes );
  return image( size_pixels, data );
}

} // namespace gfx
