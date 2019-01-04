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
#include "loops.hpp"
#include "plane.hpp"
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

namespace rn {

namespace {

/****************************************************************
** The Window Plane
*****************************************************************/
struct WindowPlane : public Plane {
  WindowPlane() = default;
  bool enabled() const override { return wm.num_windows() > 0; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& tx ) const override {
    clear_texture_transparent( tx );
    wm.draw_layout( tx );
  }
  bool input( input::event_t const& event ) override {
    return wm.input( event );
  }
  ui::WindowManager wm;
};

WindowPlane g_window_plane;

} // namespace

Plane* window_plane() { return &g_window_plane; }

} // namespace rn

namespace rn::ui {

namespace {

/****************************************************************
** Misc. Helpers
*****************************************************************/
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

bool CompositeView::input( input::event_t const& event ) {
  for( auto p_view : *this )
    if( p_view.view->input( event ) ) return true;
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
    tx_ =
        render_text_line_shadow( fonts::standard, color, msg_ );
  else
    tx_ = render_text_line_standard( fonts::standard, color,
                                     msg_ );
}

void OneLineStringView::draw( Texture const& tx,
                              Coord          coord ) const {
  copy_texture( this->tx_, tx, coord );
}

/****************************************************************
** Derived Views
*****************************************************************/
OptionSelectItemView::OptionSelectItemView( string msg )
  : active_{e_option_active::inactive},
    background_active_{config_palette.yellow.sat1.lum11},
    background_inactive_{config_palette.orange.sat0.lum3},
    foreground_active_( msg, config_palette.orange.sat0.lum2,
                        /*shadow=*/true ),
    foreground_inactive_( msg, config_palette.orange.sat1.lum11,
                          /*shadow=*/true ) {
  auto delta_active   = foreground_active_.delta();
  auto delta_inactive = foreground_inactive_.delta();
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

void OptionSelectItemView::grow_to( W w ) {
  auto new_delta = foreground_active_.delta();
  if( new_delta.w > w )
    // we only grow here, not shrink.
    return;
  new_delta.w = w;
  background_active_.set_delta( new_delta );
  background_inactive_.set_delta( new_delta );
}

OptionSelectView::OptionSelectView( StrVec const& options,
                                    int initial_selection )
  : selected_{initial_selection}, has_confirmed{false} {
  CHECK( options.size() > 0 );
  CHECK( selected_ >= 0 && selected_ < int( options.size() ) );

  Coord so_far{};
  W     min_width{0};
  for( auto const& option : options ) {
    auto view   = make_unique<OptionSelectItemView>( option );
    auto width  = view->delta().w;
    auto height = view->delta().h;
    this->push_back(
        OwningPositionedView( move( view ), so_far ) );
    // `view` is no longer available here (moved from).
    so_far.y += height;
    min_width = std::max( min_width, width );
  }

  grow_to( min_width );

  // Now that we have the individual options populated we can
  // officially set a selected one.
  set_selected( selected_ );
}

ObserverPtr<OptionSelectItemView> OptionSelectView::get_view(
    int item ) {
  CHECK( item >= 0 && item < count(),
         "item '{}' is out of bounds", item );
  auto* view    = at( item ).view.get();
  auto* o_s_i_v = view->cast<OptionSelectItemView>();
  return ObserverPtr<OptionSelectItemView>{o_s_i_v};
}

// TODO: duplication
ObserverCPtr<OptionSelectItemView> OptionSelectView::get_view(
    int item ) const {
  CHECK( item >= 0 && item < count(),
         "item '{}' is out of bounds", item );
  auto* view    = at_const( item ).view.get();
  auto* o_s_i_v = view->cast<OptionSelectItemView>();
  return ObserverCPtr<OptionSelectItemView>{o_s_i_v};
}

void OptionSelectView::set_selected( int item ) {
  get_view( selected_ )->set_active( e_option_active::inactive );
  get_view( item )->set_active( e_option_active::active );
  selected_ = item;
}

void OptionSelectView::grow_to( W w ) {
  for( auto p_view : *this ) {
    auto* view    = p_view.view.get();
    auto* o_s_i_v = view->cast<OptionSelectItemView>();
    o_s_i_v->grow_to( w );
  }
}

bool OptionSelectView::input( input::event_t const& event ) {
  bool handled = false;
  switch_v( event ) {
    case_v( input::key_event_t ) {
      auto const& key_event = val;
      if( key_event.change != input::e_key_change::down )
        break_v;
      // It's a key down.
      switch( key_event.keycode ) {
        case ::SDLK_UP:
        case ::SDLK_KP_8:
          if( selected_ > 0 ) set_selected( selected_ - 1 );
          handled = true;
          break;
        case ::SDLK_DOWN:
        case ::SDLK_KP_2:
          if( selected_ < count() - 1 )
            set_selected( selected_ + 1 );
          handled = true;
          break;
        case ::SDLK_RETURN:
        case ::SDLK_KP_ENTER:
          has_confirmed = true;
          handled       = true;
          break;
        default: break;
      }
    }
    default_v_no_check;
  };
  return handled;
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
  auto inner_size    = win_size - Scale( 2 ) * window_border();
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
  res += Scale( 2 ) * window_border();
  return res;
}

Rect WindowManager::window::rect() const {
  return Rect::from( position, delta() );
}

Coord WindowManager::window::inside_border() const {
  return position + window_border();
}

Rect WindowManager::window::inside_border_rect() const {
  auto res = rect();
  res.x += window_border().w;
  res.y += window_border().h;
  res.w -= window_border().w * 2_sx;
  res.h -= window_border().h * 2_sy;
  return res;
}

Rect WindowManager::window::title_bar() const {
  auto title_bar_rect = title_view->rect( inside_border() );
  title_bar_rect.w =
      std::max( title_bar_rect.w, view->delta().w );
  return title_bar_rect;
}

void WindowManager::draw_layout( Texture const& tx ) const {
  for( auto const& window : windows_ ) window.draw( tx );
}

WindowManager::window* WindowManager::add_window(
    string title_, unique_ptr<View> view_ ) {
  windows_.emplace_back( move( title_ ), move( view_ ),
                         Coord{} );
  auto& new_window = windows_.back();
  new_window.position =
      centered( new_window.delta(), screen_logical_rect() );
  return &new_window;
}

WindowManager::window* WindowManager::add_window(
    string title_, unique_ptr<View> view_, Coord position ) {
  windows_.emplace_back( move( title_ ), move( view_ ),
                         position );
  return &windows_.back();
}

void WindowManager::clear_windows() { windows_.clear(); }

bool WindowManager::input( input::event_t const& event ) {
  if( num_windows() == 0 ) return false;
  // Rect title_bar = focused().title_bar();
  // if_v( event.event, input::mouse_move_event_t, move_event ) {
  //  logger->trace( "Mouse move event" );
  //  if( move_event->pos.is_inside( title_bar ) ||
  //        move_event->prev.is_inside( title_bar ) ) {
  //    if( event.mouse_state.left ) {
  //      // We're dragging on the title bar.
  //      focused().position += mouse_event->delta;
  //      return true;
  //    }
  //  }
  //}
  return focused().view->input( event );
}

WindowManager::window& WindowManager::focused() {
  CHECK( !windows_.empty() );
  return windows_[0];
}

/****************************************************************
** High-level Methods
*****************************************************************/
string select_box( string const& title, StrVec options ) {
  std::vector<OwningPositionedView> views;

  auto selector = make_unique<OptionSelectView>(
      options, /*initial_selection=*/0 );
  auto* selector_ptr = selector.get();
  auto  finished     = [selector_ptr] {
    return selector_ptr->confirmed();
  };
  views.emplace_back(
      OwningPositionedView{move( selector ), Coord{0_y, 0_x}} );
  auto view = make_unique<ViewVector>( move( views ) );

  auto* win =
      g_window_plane.wm.add_window( title, move( view ) );
  selector_ptr->grow_to( win->inside_border_rect().w );
  reset_fade_to_dark( chrono::milliseconds( 1500 ),
                      chrono::milliseconds( 3000 ), 65 );
  effects_plane_enable( true );
  frame_throttler( true, finished );
  effects_plane_enable( false );
  logger->info( "selected: {}", selector_ptr->get_selected() );
  auto result = selector_ptr->get_selected();
  g_window_plane.wm.clear_windows();
  return result;
}

e_confirm yes_no( string const& title ) {
  vector<pair<e_confirm, string>> dict{
      {e_confirm::yes, "Yes"},
      {e_confirm::no, "No"},
  };
  auto res = select_box_enum( title, dict );
  return res;
}

void message_box( string_view msg ) { (void)msg; }

} // namespace rn::ui
