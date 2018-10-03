/****************************************************************
* window.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-30.
*
* Description: Handles windowing system for user interaction.
*
*****************************************************************/
#include "window.hpp"

#include "globals.hpp"

#include "util/util.hpp"

#include <algorithm>
#include <numeric>

using namespace std;

namespace rn {
namespace gui {

namespace {

} // namespace

bool SolidRectView::needs_redraw() const {
  return true;
}

void SolidRectView::draw( Texture const& tx, Coord coord ) const {
  set_render_target( tx );
  set_render_draw_color( color_ );
  auto rect = to_SDL( Rect{coord.x,coord.y,delta_.w,delta_.h} );
  ::SDL_RenderFillRect( g_renderer, &rect );
}

Delta SolidRectView::size() const {
  return delta_;
}

bool CompositeView::needs_redraw() const {
  return any_of( views_.begin(), views_.end(), L(
    _.view->needs_redraw();
  ));
}

void CompositeView::draw( Texture const& tx, Coord coord ) const {
  // Draw each of the sub views, by augment its origin (which is
  // relative to the origin of the parent by the origin that we
  // have been given.
  for( auto const& view_desc : views_ )
    view_desc.view->draw( tx, coord+(view_desc.coord-Coord()) );
}

Delta CompositeView::size() const {
  if( views_.empty() )
    return {};
  auto uni0n = L2( _1.uni0n( _2.bounds() ) );
  auto res = accumulate(
      views_.begin(), views_.end(), views_[0].bounds(), uni0n );
  return {res.w, res.h};
}

void message_box( string_view msg, RenderFunc render_bg ) {
  (void)msg;
  (void)render_bg;
}

} // namespace gui
} // namespace rn
