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

#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "globals.hpp"
#include "tiles.hpp"
#include "typed-int.hpp"

#include "base-util/misc.hpp"

#include <algorithm>
#include <numeric>

using namespace std;

namespace rn::ui {

namespace {

// This returns the width in pixels of the window border ( same
// for left/right sides as for top/bottom sides). Don't forget
// that this Delta must be multiplied by two to get the total
// change in width/height of a window with such a border.
Delta const& window_border() {
  static Delta const delta{W( config_ui.window.border_width ),
                           H( config_ui.window.border_width )};
  return delta;
}

} // namespace

void SolidRectView::draw( Texture const& tx,
                          Coord          coord ) const {
  auto rect = Rect{coord.x, coord.y, size_.w, size_.h};
  render_fill_rect( tx, color_, rect );
}

Delta SolidRectView::size() const { return size_; }

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

OneLineStringView::OneLineStringView( string msg, W ) {
  tx = render_line_shadow( fonts::standard, msg );
}

void OneLineStringView::draw( Texture const& tx,
                              Coord          coord ) const {
  CHECK( copy_texture( this->tx, tx, coord ) );
}

Delta OneLineStringView::size() const { return tx.size(); }

void WindowManager::window::draw( Texture const& tx ) const {
  auto win_size = size();
  render_fill_rect(
      tx, Color::red(),
      {position.x, position.y, win_size.w, win_size.h} );
  auto inside_border = position + window_border();
  auto inner_size    = win_size - 2 * window_border();
  render_fill_rect( tx, Color::white(),
                    Rect::from( inside_border, inner_size ) );
  title_bar->draw( tx, inside_border );
  view->draw( tx, inside_border + title_bar->size().h );
}

// Includes border
Delta WindowManager::window::size() const {
  Delta res;
  res.w = std::max( title_bar->size().w, view->size().w );
  res.h += title_bar->size().h + view->size().h;
  // multiply by two since there is top/bottom or left/right.
  res += 2 * window_border();
  return res;
}

void WindowManager::draw_layout( Texture const& tx ) const {
  for( auto const& window : windows_ ) window.draw( tx );
}

void WindowManager::add_window( std::string           title_,
                                std::unique_ptr<View> view_,
                                Coord position ) {
  windows_.emplace_back( std::move( title_ ), std::move( view_ ),
                         position );
}

bool WindowManager::accept_input( SDL_Event event ) {
  switch( event.type ) {
    case ::SDL_MOUSEBUTTONDOWN:
      switch( event.button.button ) {
        case SDL_BUTTON_LEFT: break;
        case SDL_BUTTON_RIGHT: break;
      }
    default: break;
  }
  return false;
}

void test_window() {
  auto square = Delta{g_tile_width, g_tile_height};
  vector<PositionedView> squares;
  squares.emplace_back( PositionedView{
      make_unique<SolidRectView>( Color::red(), square ),
      Coord{0_y, 0_x}} );
  squares.emplace_back( PositionedView{
      make_unique<SolidRectView>( Color::green(), square ),
      Coord{0_y, 32_x}} );
  squares.emplace_back( PositionedView{
      make_unique<SolidRectView>( Color::black(), square ),
      Coord{20_y, 0_x}} );
  squares.emplace_back( PositionedView{
      make_unique<SolidRectView>( Color::blue(), square ),
      Coord{16_y, 16_x}} );
  auto view = make_unique<CompositeView>( move( squares ) );
  WindowManager wm;
  wm.add_window( "First Window", move( view ), {200_y, 200_x} );
  wm.run( [] {} );
}

void WindowManager::run( RenderFunc const& render ) {
  ::SDL_Event event;
  bool        running = true;
  draw_layout( Texture() );
  while( running ) {
    clear_texture_black( Texture() );
    render();
    draw_layout( Texture() );
    ::SDL_RenderPresent( g_renderer );
    while( ::SDL_PollEvent( &event ) != 0 ) {
      if( event.type == SDL_KEYDOWN &&
          event.key.keysym.sym == ::SDLK_q )
        running = false;
      (void)accept_input( event );
    }
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    ::SDL_Delay( 10 );
  }
}

void message_box( string_view       msg,
                  RenderFunc const& render_bg ) {
  (void)msg;
  (void)render_bg;
}

} // namespace rn::ui
