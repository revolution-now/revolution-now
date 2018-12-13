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

namespace {} // namespace

bool SolidRectView::needs_redraw() const { return true; }

void SolidRectView::draw( Texture const& tx,
                          Coord          coord ) const {
  auto rect = Rect{coord.x, coord.y, size_.w, size_.h};
  render_fill_rect( tx, color_, rect );
}

Delta SolidRectView::size() const { return size_; }

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
  background_    = std::make_unique<SolidRectView>(
      Color::white(),
      Delta{std::max( size, W( text_size.w + 4 ) ),
            text_size.h + 4} );
  tx = render_line_shadow( fonts::standard, msg_ );
}

bool OneLineStringView::needs_redraw() const { return true; }

void OneLineStringView::draw( Texture const& tx,
                              Coord          coord ) const {
  background_->draw( tx, coord );
  CHECK( copy_texture( this->tx, tx, coord + Delta{2_w, 2_h} ) );
}

Delta OneLineStringView::size() const {
  return background_->size();
}

bool Window::needs_redraw() const {
  return view_->needs_redraw() || title_bar_->needs_redraw();
}

void Window::draw( Texture const& tx, Coord coord ) const {
  auto inside_border = coord + Delta{1_w, 1_h};
  render_fill_rect( tx, Color::black(),
                    {coord.x, coord.y, size().w, size().h} );
  title_bar_->draw( tx, inside_border );
  view_->draw( tx, inside_border + title_bar_->size().h );
  render_rect( tx, Color::black(),
               {coord.x, coord.y, size().w, size().h} );
}

Delta Window::size() const {
  Delta res;
  res.w = std::max( title_bar_->size().w, view_->size().w );
  res.h += title_bar_->size().h + view_->size().h;
  res.w += 2;
  res.h += 2;
  return res;
}

Delta WindowManager::window_size() const {
  Delta res{};
  for( auto const& positioned_window : windows_ )
    res = rn::max( res, positioned_window.window->size() );
  return res;
}

void WindowManager::draw_layout( Texture const& tx ) const {
  for( auto const& [window, position] : windows_ )
    window->draw( tx, position );
}

void WindowManager::add_window( WinPtr window, Coord position ) {
  windows_.push_back( {std::move( window ), position} );
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
  auto               square = Delta{g_tile_width, g_tile_height};
  vector<FramedView> squares;
  squares.emplace_back( FramedView{
      Coord{0_y, 0_x},
      make_unique<SolidRectView>( Color::red(), square )} );
  squares.emplace_back( FramedView{
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
      Coord{16_y, 16_x},
      make_unique<SolidRectView>( Color::blue(), square )} );
  squares.emplace_back( FramedView{
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
      Coord{0_y, 32_x},
      make_unique<SolidRectView>( Color::green(), square )} );
  squares.emplace_back( FramedView{
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
      Coord{20_y, 0_x},
      make_unique<SolidRectView>( Color::white(), square )} );
  auto window = make_unique<Window>(
      "First Window",
      make_unique<CompositeView>( move( squares ) ) );
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  WindowManager wm;
  wm.add_window( move( window ), {200_y, 200_x} );
  wm.run( [] {} );
}

void WindowManager::run( RenderFunc const& /*unused*/ ) {
  ::SDL_Event event;
  bool        running = true;
  draw_layout( Texture() );
  while( running ) {
    clear_texture_black( Texture() );
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
