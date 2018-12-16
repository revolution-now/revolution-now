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

bool CompositeView::accept_input( input::event_t const& event ) {
  for( auto p_view : *this )
    if( p_view.view->accept_input( event ) ) return true;
  return false;
}

PositionedView ViewVector::at( int idx ) {
  CHECK( idx >= 0 && idx < int( views_.size() ) );
  auto& view = views_[idx];
  return {view.view(), view.coord()};
}

PositionedViewConst ViewVector::at( int idx ) const {
  CHECK( idx >= 0 && idx < int( views_.size() ) );
  auto& view = views_[idx];
  return {view.view(), view.coord()};
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
** Derived Views
*****************************************************************/
PositionedViewConst OptionSelectItemView::at( int idx ) const {
  CHECK( idx == 0 || idx == 1 );
  Coord       coord{};
  View const* view{};
  switch( idx ) {
    case 0:
      switch( active_ ) {
        case e_option_active::active:
          view = &background_active_;
          break;
        case e_option_active::inactive:
          view = &background_inactive_;
          break;
      }
      break;
    case 1: view = &line_; break;
  }
  return {ObserverCPtr<View>{view}, coord};
}
// TODO: need to find a better way to implement both a const and
// non-const version of the same function.
PositionedView OptionSelectItemView::at( int idx ) {
  auto p_view =
      static_cast<OptionSelectItemView const*>( this )->at(
          idx );
  ObserverPtr<View> view{const_cast<View*>( p_view.view.get() )};
  return {view, p_view.coord};
}

OptionSelectView::OptionSelectView( StrVec const& options,
                                    int initial_selection )
  : selected_{initial_selection} {
  CHECK( options.size() > 0 );
  CHECK( selected_ >= 0 && selected_ < int( options.size() ) );

  Coord so_far{};
  for( auto const& option : options ) {
    auto view   = make_unique<OptionSelectItemView>( option );
    auto height = view->delta().h;
    push_back( OwningPositionedView( move( view ), so_far ) );
    // `view` is no longer available here (moved from).
    so_far.y += height;
  }

  // Now that we have the individual options populated we can
  // officially set a selected one.
  set_selected( selected_ );
}

ObserverPtr<OptionSelectItemView> OptionSelectView::get_view(
    int item ) {
  CHECK_( item >= 0 && item < count(),
          "item '" << item << "' is out of bounds" );
  auto* view    = at( item ).view.get();
  auto* o_s_i_v = dynamic_cast<OptionSelectItemView*>( view );
  return ObserverPtr<OptionSelectItemView>{o_s_i_v};
}

void OptionSelectView::set_selected( int item ) {
  get_view( selected_ )->set_active( e_option_active::inactive );
  get_view( item )->set_active( e_option_active::active );
  selected_ = item;
}

bool OptionSelectView::accept_input(
    input::event_t const& event ) {
  if( !holds_alternative<input::key_event_t>( event.event ) )
    return false;
  // It's a keyboard event.
  auto key_event = get<input::key_event_t>( event.event );
  if( key_event.change != input::e_key_change::down )
    return false;
  // It's a key down.
  switch( key_event.scancode ) {
    case ::SDL_SCANCODE_UP:
      if( selected_ > 0 ) set_selected( selected_ - 1 );
      return true;
    case ::SDL_SCANCODE_DOWN:
      if( selected_ < count() - 1 )
        set_selected( selected_ + 1 );
      return true;
    default: return false;
  };
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

void WindowManager::add_window( string           title_,
                                unique_ptr<View> view_,
                                Coord            position ) {
  windows_.emplace_back( move( title_ ), move( view_ ),
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
  if( holds_alternative<input::mouse_event_t>( event.event ) ) {
    auto const& mouse_event =
        get<input::mouse_event_t>( event.event );
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
  std::vector<OwningPositionedView> views;
  StrVec options{"This is the first option.",
                 "This is the second option.",
                 "This is the third option.", "Short.",
                 "A very long long long long long long option"};
  views.emplace_back( OwningPositionedView{
      make_unique<OptionSelectView>( options, 2 ),
      Coord{0_y, 0_x}} );
  auto view = make_unique<ViewVector>( move( views ) );

  WindowManager wm;
  wm.add_window( "Would you like to make landfall?",
                 move( view ), {200_y, 200_x} );
  wm.run( [] {} );
}

void message_box( string_view       msg,
                  RenderFunc const& render_bg ) {
  (void)msg;
  (void)render_bg;
}

} // namespace rn::ui
