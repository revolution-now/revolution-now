/****************************************************************
**iter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-16.
*
* Description: Useful generators.
*
*****************************************************************/
#include "iter.hpp"

using namespace std;

namespace gfx {

/****************************************************************
** Public API
*****************************************************************/
base::generator<rn::Rect> subrects( rn::Rect  rect,
                                    rn::Delta chunk ) {
  for( int y = rect.top_edge(); y < rect.bottom_edge();
       y += chunk.h )
    for( int x = rect.left_edge(); x < rect.right_edge();
         x += chunk.w )
      co_yield rn::Rect{
          .x = x, .y = y, .w = chunk.w, .h = chunk.h }
          .clamp( rect );
}

base::generator<rect> subrects( rect r, size chunk ) {
  for( int y = r.top(); y < r.bottom(); y += chunk.h )
    for( int x = r.left(); x < r.right(); x += chunk.w )
      co_yield rect{ .origin = { .x = x, .y = y },
                     .size   = { .w = chunk.w, .h = chunk.h } }
          .clamped( r );
}

} // namespace gfx
