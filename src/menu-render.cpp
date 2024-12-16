/****************************************************************
**menu-render.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-15.
*
* Description: Renders menus.
*
*****************************************************************/
#include "menu-render.hpp"

// render
#include "error.hpp"
#include "render/painter.hpp"
#include "render/renderer.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** MenuRenderer
*****************************************************************/
MenuRenderer::MenuRenderer( MenuRenderState const& state )
  : state_( state ), bounds_( compute_bounding_rect( state ) ) {}

rect MenuRenderer::bounding_rect() const { return bounds_; }

size MenuRenderer::compute_bounding_size(
    MenuRenderState const& ) {
  // TODO
  return { .w = 32, .h = 64 };
}

rect MenuRenderer::compute_bounding_rect(
    MenuRenderState const& state ) {
  size s             = compute_bounding_size( state );
  point const corner = state.position.where;
  point nw;
  switch( state.position.corner ) {
    case e_direction::se: {
      size delta = corner.moved( reverse_direction(
                       state.position.corner ) ) -
                   corner;
      delta.w *= s.w;
      delta.h *= s.h;
      nw = corner + delta;
      break;
    }
    case e_direction::ne:
      NOT_IMPLEMENTED;
    case e_direction::sw:
      NOT_IMPLEMENTED;
    case e_direction::nw:
      nw = corner;
      break;
    default:
      FATAL( "direction {} not supported here.",
             state.position.corner );
  }
  return rect{ .origin = nw, .size = s }.normalized();
}

void MenuRenderer::render_body( rr::Renderer& renderer ) const {
  rect const r        = bounding_rect();
  rr::Painter painter = renderer.painter();
  painter.draw_solid_rect( r, pixel::white() );
}

} // namespace rn
