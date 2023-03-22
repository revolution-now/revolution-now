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

/****************************************************************
** rect_iterator
*****************************************************************/
using RI = rect_iterator;

RI::const_iterator::const_iterator( rect r, rn::Coord cursor )
  : r_( r ), cursor_( cursor ) {
  if( r.size.area() == 0 )
    cursor_ = rn::Coord::from_gfx( r_.sw() );
}

RI::const_iterator& RI::const_iterator::operator++() {
  CHECK_LT( cursor_.y, r_.bottom() );
  ++cursor_.x;
  if( cursor_.x == r_.right() ) {
    cursor_.x = r_.left();
    ++cursor_.y;
  }
  CHECK_GE( cursor_.x, r_.left() );
  CHECK_GE( cursor_.y, r_.top() );
  CHECK_LE( cursor_.y, r_.bottom() );
  if( cursor_.y == r_.bottom() ) {
    CHECK_EQ( cursor_.x, r_.left() );
    CHECK_EQ( cursor_.y, r_.bottom() );
  }
  return *this;
};

RI::const_iterator RI::const_iterator::operator++( int ) {
  auto res = *this;
  ++( *this );
  return res;
}

} // namespace gfx
