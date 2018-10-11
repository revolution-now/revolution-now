/****************************************************************
**window.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-30.
*
* Description: Handles windowing system for user interaction.
*
*****************************************************************/
#include "window.hpp"

#include "fonts.hpp"
#include "globals.hpp"
#include "macros.hpp"
#include "typed-int.hpp"

#include "util/util.hpp"

#include <algorithm>
#include <numeric>

using namespace std;

namespace rn {
namespace gui {

namespace {} // namespace

bool SolidRectView::needs_redraw() const { return true; }

void SolidRectView::draw( Texture const& tx,
                          Coord          coord ) const {
  auto rect = Rect{coord.x, coord.y, delta_.w, delta_.h};
  render_fill_rect( tx, color_, rect );
}

Delta SolidRectView::size() const { return delta_; }

bool CompositeView::needs_redraw() const {
  return any_of( views_.begin(), views_.end(),
                 L( _.view->needs_redraw(); ) );
}

void CompositeView::draw( Texture const& tx,
                          Coord          coord ) const {
  // Draw each of the sub views, by augment its origin (which is
  // relative to the origin of the parent by the origin that we
  // have been given.
  for( auto const& view_desc : views_ )
    view_desc.view->draw(
        tx, coord + ( view_desc.coord - Coord() ) );
}

Delta CompositeView::size() const {
  if( views_.empty() ) return {};
  auto uni0n = L2( _1.uni0n( _2.bounds() ) );
  auto res   = accumulate( views_.begin(), views_.end(),
                         views_[0].bounds(), uni0n );
  return {res.w, res.h};
}

OneLineStringView::OneLineStringView( string msg, W size ) {
  msg_           = std::move( msg );
  auto text_size = font_rendered_width( fonts::standard, msg_ );
  background_    = SolidRectView(
      Color::white(), Delta{max( size, W( text_size.w + 4 ) ),
                            text_size.h + 4} );
  tx = render_line_shadow( fonts::standard, msg_ );
}

bool OneLineStringView::needs_redraw() const { return true; }

void OneLineStringView::draw( Texture const& tx,
                              Coord          coord ) const {
  background_.draw( tx, coord );
  CHECK( copy_texture( this->tx, tx,
                       coord + Delta{W( 2 ), H( 2 )} ) );
}

Delta OneLineStringView::size() const {
  return background_.size();
}

bool Window::needs_redraw() const {
  return view_->needs_redraw();
}

void Window::draw( Texture const& tx, Coord coord ) const {
  view_->draw( tx, coord );
}

Delta Window::size() const { return view_->size(); }

WindowManager::WindowManager( WinPtr window, Coord position )
  : window_( std::move( window ) ),
    title_bar_( window_->title(), window_->size().w ),
    position_( position ) {}

Delta WindowManager::window_size() const {
  Delta res;
  res.w = max( title_bar_.size().w, window_->size().w );
  res.h += title_bar_.size().h + window_->size().h;
  res.w += 2;
  res.h += 2;
  return res;
}

void WindowManager::draw_layout( Texture const& tx ) const {
  auto inside_border = position_ + Delta{W( 1 ), H( 1 )};
  render_fill_rect( tx, Color::black(),
                    {position_.x, position_.y, window_size().w,
                     window_size().h} );
  title_bar_.draw( tx, inside_border );
  window_->draw( tx, inside_border + title_bar_.size().h );
  render_rect( tx, Color::black(),
               {position_.x, position_.y, window_size().w,
                window_size().h} );
}

bool WindowManager::accept_input( SDL_Event event ) {
  switch( event.type ) {
    case ::SDL_MOUSEBUTTONDOWN:
      switch( event.button.button ) {
        case SDL_BUTTON_LEFT:
          // LOG( "SDL_BUTTON_LEFT" );
          break;
        case SDL_BUTTON_RIGHT:
          // LOG( "SDL_BUTTON_RIGHT" );
          break;
      }
    default: break;
  }
  return false;
}

void test_window() {
  auto             square = Delta{W( 32 ), H( 32 )};
  vector<ViewDesc> squares;
  squares.emplace_back( ViewDesc{
      Coord{Y( 0 ), X( 0 )},
      make_unique<SolidRectView>( Color::red(), square )} );
  squares.emplace_back( ViewDesc{
      Coord{Y( 16 ), X( 16 )},
      make_unique<SolidRectView>( Color::blue(), square )} );
  squares.emplace_back( ViewDesc{
      Coord{Y( 0 ), X( 32 )},
      make_unique<SolidRectView>( Color::green(), square )} );
  squares.emplace_back( ViewDesc{
      Coord{Y( 20 ), X( 0 )},
      make_unique<SolidRectView>( Color::white(), square )} );
  auto window = make_unique<Window>(
      "First Window",
      make_unique<CompositeView>( move( squares ) ) );
  WindowManager wm( move( window ), {Y( 200 ), X( 200 )} );
  wm.run( [] {} );
}

void WindowManager::run( RenderFunc ) {
  ::SDL_Event event;
  while( true ) {
    draw_layout( Texture() );
    ::SDL_RenderPresent( g_renderer );
    if( ::SDL_PollEvent( &event ) ) {
      if( event.type == SDL_KEYDOWN &&
          event.key.keysym.sym == ::SDLK_q )
        break;
      (void)accept_input( event );
    }
    ::SDL_Delay( 10 );
  }
}

void message_box( string_view msg, RenderFunc render_bg ) {
  (void)msg;
  (void)render_bg;
}

} // namespace gui
} // namespace rn
