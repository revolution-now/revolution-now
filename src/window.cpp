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
#include "render.hpp"
#include "tiles.hpp"
#include "typed-int.hpp"
#include "util.hpp"

// base-util
#include "base-util/misc.hpp"
#include "base-util/variant.hpp"

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

PositionedView CompositeView::at( int idx ) {
  auto p_view =
      static_cast<CompositeView const*>( this )->at_const( idx );
  ObserverPtr<View> view{const_cast<View*>( p_view.view.get() )};
  return {view, p_view.coord};
}

PositionedViewConst ViewVector::at_const( int idx ) const {
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

OneLineStringView::OneLineStringView( string msg, Color color,
                                      bool shadow )
  : msg_( move( msg ) ) {
  if( shadow )
    tx_ = render_line_shadow( fonts::standard, color, msg_ );
  else
    tx_ = render_line_standard( fonts::standard, color, msg_ );
}

void OneLineStringView::draw( Texture const& tx,
                              Coord          coord ) const {
  CHECK( copy_texture( this->tx_, tx, coord ) );
}

/****************************************************************
** Derived Views
*****************************************************************/
OptionSelectItemView::OptionSelectItemView( string msg, W width )
  : active_{e_option_active::inactive},
    background_active_{config_palette.yellow.sat1.lum11},
    background_inactive_{config_palette.orange.sat0.lum3},
    foreground_active_( msg, config_palette.orange.sat0.lum2,
                        /*shadow=*/true ),
    foreground_inactive_( msg, config_palette.orange.sat1.lum11,
                          /*shadow=*/true ) {
  auto delta_active   = foreground_active_.delta();
  auto delta_inactive = foreground_inactive_.delta();
  delta_active.w      = width;
  delta_inactive.w    = width;
  background_active_.set_delta( delta_active );
  background_inactive_.set_delta( delta_inactive );
}

PositionedViewConst OptionSelectItemView::at_const(
    int idx ) const {
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
    case 1:
      switch( active_ ) {
        case e_option_active::active:
          view = &foreground_active_;
          break;
        case e_option_active::inactive:
          view = &foreground_inactive_;
          break;
      }
  }
  return {ObserverCPtr<View>{view}, coord};
}

OptionSelectView::OptionSelectView( StrVec const& options,
                                    W             width,
                                    int initial_selection )
  : selected_{initial_selection}, has_confirmed{false} {
  CHECK( options.size() > 0 );
  CHECK( selected_ >= 0 && selected_ < int( options.size() ) );

  Coord so_far{};
  for( auto const& option : options ) {
    auto view =
        make_unique<OptionSelectItemView>( option, width );
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

// TODO: duplication
ObserverCPtr<OptionSelectItemView> OptionSelectView::get_view(
    int item ) const {
  CHECK_( item >= 0 && item < count(),
          "item '" << item << "' is out of bounds" );
  auto* view = at_const( item ).view.get();
  auto* o_s_i_v =
      dynamic_cast<OptionSelectItemView const*>( view );
  return ObserverCPtr<OptionSelectItemView>{o_s_i_v};
}

void OptionSelectView::set_selected( int item ) {
  get_view( selected_ )->set_active( e_option_active::inactive );
  get_view( item )->set_active( e_option_active::active );
  selected_ = item;
}

bool OptionSelectView::accept_input(
    input::event_t const& event ) {
  if( !util::holds<input::key_event_t>( event.event ) )
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
    case ::SDL_SCANCODE_RETURN:
      has_confirmed = true;
      return true;
    default: return false;
  };
}

string const& OptionSelectView::get_selected() const {
  return get_view( selected_ )->line();
}

/****************************************************************
** WindowManager
*****************************************************************/
WindowManager::window::window( string           title_,
                               unique_ptr<View> view_,
                               Coord            position_ )
  : window_state( e_window_state::running ),
    title( move( title_ ) ),
    view( move( view_ ) ),
    title_view(),
    position( position_ ) {
  title_view = make_unique<OneLineStringView>(
      title, config_palette.orange.sat1.lum11, /*shadow=*/true );
}

void WindowManager::window::draw( Texture const& tx ) const {
  auto win_size = delta();
  render_fill_rect(
      tx, Color( 0, 0, 0, 64 ),
      Rect::from( position + Delta{5_w, 5_h}, win_size ) );
  render_fill_rect(
      tx, config_palette.orange.sat0.lum1,
      {position.x, position.y, win_size.w, win_size.h} );
  auto inside_border = position + window_border();
  auto inner_size    = win_size - 2 * window_border();
  render_fill_rect( tx, config_palette.orange.sat1.lum4,
                    Rect::from( inside_border, inner_size ) );
  title_view->draw( tx, inside_border );
  view->draw( tx, inside_border + title_view->delta().h );
}

// Includes border
Delta WindowManager::window::delta() const {
  Delta res;
  res.w = std::max( title_view->delta().w, view->delta().w );
  res.h += title_view->delta().h + view->delta().h;
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

Rect WindowManager::window::title_bar() const {
  auto title_bar_rect = title_view->rect( inside_border() );
  title_bar_rect.w    = view->delta().w;
  return title_bar_rect;
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

  Rect title_bar = focused().title_bar();
  // auto mouse_pos = event.mouse_state.pos;
  // logger->trace( "title_bar: ({},{},{},{}), pos: ({},{})",
  //               title_bar.x, title_bar.y, title_bar.w,
  //               title_bar.h, mouse_pos.x, mouse_pos.y );
  GET_IF( event.event, input::mouse_event_t, mouse_event ) {
    logger->trace( "Mouse event" );
    if( event.mouse_state.pos.is_inside( title_bar ) ||
        mouse_event->prev.is_inside( title_bar ) ) {
      logger->trace( "  and inside title bar" );
      if( event.mouse_state.left ) {
        logger->trace( "  and left mouse down" );
        logger->trace( "  and delta x: {}",
                       mouse_event->delta.w );
        logger->trace( "  and delta y: {}",
                       mouse_event->delta.h );
        // We're dragging on the title bar.
        focused().position += mouse_event->delta;
        return e_wm_input_result::handled;
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

void WindowManager::run( FinishedFunc const& finished ) {
  logger->debug( "Running window manager" );
  ::SDL_Event   event;
  bool          running = true;
  RenderStacker push_renderer(
      [this] { this->draw_layout( Texture() ); } );
  render_all();
  while( running && !finished() ) {
    render_all();
    while( ::SDL_PollEvent( &event ) != 0 ) {
      running &=
          ( accept_input( event ) != e_wm_input_result::quit );
    }
    ::SDL_Delay( 10 );
  }
}

/****************************************************************
** High-level Methods
*****************************************************************/
string select_box( string const& title, StrVec options ) {
  std::vector<OwningPositionedView> views;

  auto selector =
      make_unique<OptionSelectView>( options, 450_w, 0 );
  auto* selector_ptr = selector.get();
  auto  finished     = [selector_ptr] {
    return selector_ptr->confirmed();
  };
  views.emplace_back(
      OwningPositionedView{move( selector ), Coord{0_y, 0_x}} );
  auto view = make_unique<ViewVector>( move( views ) );

  WindowManager wm;
  wm.add_window( title, move( view ), {200_y, 200_x} );
  wm.run( finished );
  logger->info( "Selected: {}", selector_ptr->get_selected() );
  return selector_ptr->get_selected();
}

e_confirm yes_no( string const& title ) {
  vector<pair<e_confirm, string>> dict{
      {e_confirm::yes, "Yes"},
      {e_confirm::no, "No"},
  };
  auto res = select_box_enum( title, dict );
  logger->info( "Selected: {}", int( res ) );
  return res;
}

void message_box( string_view msg ) { (void)msg; }

} // namespace rn::ui
