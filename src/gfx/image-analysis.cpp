/****************************************************************
**image-analysis.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-17.
*
* Description: Performs analysis and precomputations on/of images
*              and sprites.
*
*****************************************************************/
#include "image-analysis.hpp"

// gfx
#include "image.hpp"
#include "iter.hpp"

// base
#include "base/maybe.hpp"

// C++ standard library
#include <array>

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;
using ::base::nothing;

void compute_burrowed_sprite_column( image const& input,
                                     image& out,
                                     point const start,
                                     int const height ) {
  static array<uint8_t, 3> constexpr kDecay{
    160,
    130,
    100,
  };
  point p;
  for( p = start; p.y >= start.y - height + 1; --p.y ) {
    pixel const in = input[p];
    if( in.a ) break;
  }
  auto it = kDecay.begin();
  for( ; p.y >= start.y - height + 1; --p.y ) {
    pixel const in = input[p];
    if( !in.a ) break;
    out[p] = pixel{ .r = *it++, .a = 255 };
    if( it == kDecay.end() ) break;
  }
}

void compute_burrowed_sprite( image const& input, image& out,
                              rect const r ) {
  point const kLowerLeftPixel = r.sw().moved_up( 1 );
  point const kLowerRightPixel =
      r.se().moved_up( 1 ).moved_left();
  int const height = r.size.h;
  for( int x = kLowerLeftPixel.x; x <= kLowerRightPixel.x; ++x )
    compute_burrowed_sprite_column(
        input, out, point{ .x = x, .y = kLowerLeftPixel.y },
        height );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
rect find_trimmed_bounds_in( image const& img, rect const r ) {
  CHECK( r.is_inside( img.rect_pixels() ) );
  maybe<point> p_min = nothing;
  maybe<point> p_max = nothing;
  for( auto const p : rect_iterator( r ) ) {
    if( img.at( p ).a == 0 ) continue;
    if( !p_min.has_value() ) {
      CHECK( !p_max.has_value() );
      p_min = p;
      p_max = p;
    }
    CHECK( p_min.has_value() );
    CHECK( p_max.has_value() );
    p_min->x = std::min( p_min->x, p.x );
    p_min->y = std::min( p_min->y, p.y );
    p_max->x = std::max( p_max->x, p.x + 1 );
    p_max->y = std::max( p_max->y, p.y + 1 );
  }
  if( !p_min.has_value() ) {
    CHECK( !p_max.has_value() );
    return rect{ .origin = r.center() };
  }
  return rect::from( *p_min, *p_max );
}

image compute_burrowed_sprites( image const& input,
                                vector<rect> const& sprites ) {
  image out = new_empty_image( input.size_pixels() );
  for( rect const r : sprites )
    compute_burrowed_sprite( input, out, r.normalized() );
  return out;
}

} // namespace gfx
