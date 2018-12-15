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

// Revolution Now
#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "tiles.hpp"
#include "typed-int.hpp"

// base-util
#include "base-util/misc.hpp"

// c++ standard library
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

/****************************************************************
** Views
*****************************************************************/
void CompositeView::draw( Texture const& tx,
                          Coord          coord ) const {
  // Draw each of the sub views, by augmenting its origin (which
  // is relative to the origin of the parent by the origin that
  // we have been given.
  for( auto [view, view_coord] : *this )
    view->draw( tx, coord + ( view_coord - Coord() ) );
}

Delta CompositeView::delta() const {
  auto uni0n = L2( _1.uni0n( _2.view->rect( _2.coord ) ) );
  auto rect  = accumulate( begin(), end(), Rect{}, uni0n );
  return {rect.w, rect.h};
}

PositionedView const& ViewVector::at( int idx ) const {
  CHECK( idx >= 0 && idx < int( views_.size() ) );
  return views_[idx];
}

/****************************************************************
** Primitive Views
*****************************************************************/
void SolidRectView::draw( Texture const& tx,
                          Coord          coord ) const {
  render_fill_rect( tx, color_, rect( coord ) );
}

OneLineStringView::OneLineStringView( string msg ) {
  tx = render_line_shadow( fonts::standard, msg );
}

void OneLineStringView::draw( Texture const& tx,
                              Coord          coord ) const {
  CHECK( copy_texture( this->tx, tx, coord ) );
}

/****************************************************************
** WindowManager
*****************************************************************/
void WindowManager::window::draw( Texture const& tx ) const {
  auto win_size = delta();
  render_fill_rect(
      tx, Color::red(),
      {position.x, position.y, win_size.w, win_size.h} );
  auto inside_border = position + window_border();
  auto inner_size    = win_size - 2 * window_border();
  render_fill_rect( tx, Color::white(),
                    Rect::from( inside_border, inner_size ) );
  title_bar->draw( tx, inside_border );
  view->draw( tx, inside_border + title_bar->delta().h );
}

// Includes border
Delta WindowManager::window::delta() const {
  Delta res;
  res.w = std::max( title_bar->delta().w, view->delta().w );
  res.h += title_bar->delta().h + view->delta().h;
  // multiply by two since there is top/bottom or left/right.
  res += 2 * window_border();
  return res;
}

Rect WindowManager::window::rect() const {
  return Rect::from( position, delta() );
}

Coord WindowManager::window::inside_border() const {
  return position + window_border();
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

e_wm_input_result WindowManager::accept_input(
    SDL_Event const& sdl_event ) {
  bool handled = false;

  auto event = input::from_SDL( sdl_event );

  Rect title_bar =
      focused().title_bar->rect( focused().inside_border() );
  // auto mouse_pos = event.mouse_state.pos;
  // logger->trace( "title_bar: ({},{},{},{}), pos: ({},{})",
  //               title_bar.x, title_bar.y, title_bar.w,
  //               title_bar.h, mouse_pos.x, mouse_pos.y );
  if( std::holds_alternative<input::mouse_event_t>(
          event.event ) ) {
    auto const& mouse_event =
        std::get<input::mouse_event_t>( event.event );
    logger->trace( "Mouse event" );
    if( event.mouse_state.pos.is_inside( title_bar ) ||
        mouse_event.prev.is_inside( title_bar ) ) {
      logger->trace( "  and inside title bar" );
      if( event.mouse_state.left ) {
        logger->trace( "  and left mouse down" );
        logger->trace( "  and delta x: {}",
                       mouse_event.delta.w );
        logger->trace( "  and delta y: {}",
                       mouse_event.delta.h );
        // We're dragging on the title bar.
        focused().position += mouse_event.delta;
      }
    }
  }

  handled = focused().view->accept_input( event );

  return handled ? e_wm_input_result::handled
                 : e_wm_input_result::unhandled;
}

WindowManager::window& WindowManager::focused() {
  CHECK( !windows_.empty() );
  return windows_[0];
}

void WindowManager::run( RenderFunc const& render ) {
  logger->debug( "Running window manager" );
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
          event.key.keysym.sym == ::SDLK_q ) {
        running = false;
        logger->debug( "received quit signal SDLK_q" );
      } else {
        running &=
            ( accept_input( event ) != e_wm_input_result::quit );
      }
    }
    ::SDL_Delay( 10 );
  }
}

/****************************************************************
** High-level Methods
*****************************************************************/
void test_window() {
  auto square = Delta{g_tile_width, g_tile_height};
  vector<OwningPositionedView> squares;
  squares.emplace_back( OwningPositionedView{
      make_unique<SolidRectView>( Color::red(), square ),
      Coord{0_y, 0_x}} );
  squares.emplace_back( OwningPositionedView{
      make_unique<SolidRectView>( Color::green(), square ),
      Coord{0_y, 32_x}} );
  squares.emplace_back( OwningPositionedView{
      make_unique<SolidRectView>( Color::black(), square ),
      Coord{20_y, 0_x}} );
  squares.emplace_back( OwningPositionedView{
      make_unique<SolidRectView>( Color::blue(), square ),
      Coord{16_y, 16_x}} );
  auto view = make_unique<ViewVector>( move( squares ) );
  WindowManager wm;
  wm.add_window( "First Window", move( view ), {200_y, 200_x} );
  wm.run( [] {} );
}

void message_box( string_view       msg,
                  RenderFunc const& render_bg ) {
  (void)msg;
  (void)render_bg;
}

} // namespace rn::ui
