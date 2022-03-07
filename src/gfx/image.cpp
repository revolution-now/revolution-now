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

// C++ standard library
#include <cstring>

using namespace std;

namespace gfx {

using ::base::maybe;

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

rect image::rect_pixels() const {
  return rect{ .origin = {}, .size = size_pixels_ };
}

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

image::operator span<pixel>() {
  // This reinterpret_cast should be ok because data() has type
  // `unsigned char*` which is allowed to alias anything.
  pixel* p = reinterpret_cast<pixel*>( data() );
  static_assert( sizeof( pixel ) == kBytesPerPixel );
  return span<pixel>( p, total_pixels() );
}

void image::free_resource() { ::free( resource() ); }

unsigned char* image::data_for( point const p ) const {
  unsigned char* ptr =
      data() + kBytesPerPixel * ( p.y * size_pixels_.w + p.x );
  DCHECK( ptr > data() );
  DCHECK( ptr < data() + size_bytes() );
  return ptr;
}

void image::blit_from( image const& other,
                       rect const   src_unclipped,
                       point const  dst_origin ) {
  maybe<rect> src =
      src_unclipped.clipped_by( other.rect_pixels() );
  if( !src.has_value() ) return;
  rect dst_unclipped =
      rect{ .origin = dst_origin, .size = src->size };
  maybe<rect> dst = dst_unclipped.clipped_by( rect_pixels() );
  if( !dst.has_value() ) return;
  size const copied_size =
      size{ .w = std::min( src->size.w, dst->size.w ),
            .h = std::min( src->size.h, dst->size.h ) };
  src->size = copied_size;
  dst->size = copied_size;
  if( copied_size.area() == 0 ) return;

  point     src_point = src->origin;
  point     dst_point = dst->origin;
  int const num_rows  = copied_size.h;
  for( int i = 0; i < num_rows; ++i ) {
    memcpy( data_for( dst_point ), other.data_for( src_point ),
            kBytesPerPixel * copied_size.w );
    ++src_point.y;
    ++dst_point.y;
  }
}

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
