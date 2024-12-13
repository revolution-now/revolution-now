/****************************************************************
**extra.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-03.
*
* Description: Higher level rendering utilities.
*
*****************************************************************/
#include "extra.hpp"

// render
#include "painter.hpp"

using namespace std;

namespace rr {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void draw_empty_rect_no_corners( rr::Painter& painter,
                                 rect const box,
                                 pixel const color ) {
  // Left.
  {
    point const start = box.nw();
    point const end   = box.sw();
    painter.draw_vertical_line(
        start + size{ .h = 1 },
        std::max( 0, end.y - start.y - 2 + 1 ), color );
  }
  // Right.
  {
    point const start = box.ne();
    point const end   = box.se();
    painter.draw_vertical_line(
        start + size{ .h = 1 },
        std::max( 0, end.y - start.y - 2 + 1 ), color );
  }
  // Top.
  {
    point const start = box.nw();
    point const end   = box.ne();
    painter.draw_horizontal_line(
        start + size{ .w = 1 },
        std::max( 0, end.x - start.x - 2 + 1 ), color );
  }
  // Bottom.
  {
    point const start = box.sw();
    point const end   = box.se();
    painter.draw_horizontal_line(
        start + size{ .w = 1 },
        std::max( 0, end.x - start.x - 2 + 1 ), color );
  }
}

} // namespace rr
