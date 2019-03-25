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
#include "aliases.hpp"
#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "frame.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"
#include "typed-int.hpp"
#include "ui.hpp"
#include "util.hpp"
#include "views.hpp"

// base-util
#include "base-util/misc.hpp"
#include "base-util/variant.hpp"

// c++ standard library
#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>

using namespace std;

namespace rn::ui {

namespace {

/****************************************************************
** WindowManager
*****************************************************************/
using FinishedFunc = std::function<bool( void )>;

enum class e_window_state { running, closed };

class WindowManager {
public:
  void draw_layout( Texture const& tx ) const;

  ND bool input( input::event_t const& event );

  // We don't use the corresponding enum from plane.hpp so that
  // we don't have to include that header here.
  enum class e_window_drag { yes, no, swallow };

  e_window_drag can_drag( input::e_mouse_button button,
                          Coord                 origin );
  void on_drag( input::e_mouse_button button, Coord origin,
                Coord prev, Coord current );

  auto num_windows() const { return windows_.size(); }

private:
  struct window {
    window( std::string title_, std::unique_ptr<View> view_,
            Coord position_ );

    void  draw( Texture const& tx ) const;
    Delta delta() const;
    Rect  rect() const;
    Coord inside_border() const;
    Rect  inside_border_rect() const;
    Rect  title_bar() const;
    // abs coord of upper-left corner of view.
    Coord view_pos() const;

    e_window_state                     window_state;
    std::string                        title;
    std::unique_ptr<View>              view;
    std::unique_ptr<OneLineStringView> title_view;
    Coord                              position;
  };

public:
  window* add_window( std::string           title,
                      std::unique_ptr<View> view );

  window* add_window( std::string           title,
                      std::unique_ptr<View> view,
                      Coord                 position );

  void clear_windows();

private:
  // Gets the window with focus, throws if no windows.
  window& focused();

  std::vector<window> windows_;
};

} // namespace
} // namespace rn::ui

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
  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    CHECK( wm.num_windows() != 0 );
    switch( wm.can_drag( button, origin ) ) {
      case ui::WindowManager::e_window_drag::yes:
        return Plane::e_accept_drag::yes;
      case ui::WindowManager::e_window_drag::no:
        return Plane::e_accept_drag::no;
      case ui::WindowManager::e_window_drag::swallow:
        return Plane::e_accept_drag::swallow;
    };
    return {}; // for gcc (?!)
  }
  void on_drag( input::e_mouse_button button, Coord origin,
                Coord prev, Coord current ) override {
    CHECK( wm.num_windows() != 0 );
    wm.on_drag( button, origin, prev, current );
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
** Window/View Classes
*****************************************************************/

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

Coord WindowManager::window::view_pos() const {
  return inside_border() + title_view->delta().h;
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
  auto& win = focused();
  auto  view_rect =
      Rect::from( win.view_pos(), win.view->delta() );

  if( input::is_mouse_event( event ) ) {
    auto maybe_pos = input::mouse_position( event );
    CHECK( maybe_pos.has_value() );
    // Only send the event if the mouse position is within the
    // view. And, when we send it, we make the mouse position
    // relative to the upper left corner of the view.
    if( maybe_pos.value().get().is_inside( view_rect ) ) {
      auto new_event = input::move_mouse_origin_by(
          event, win.view_pos() - Coord{} );
      return win.view->input( new_event );
    }
  } else {
    // It's a non-mouse event, so just send it and return if it
    // was handled.
    return win.view->input( event );
  }
  return false;
}

WindowManager::e_window_drag WindowManager::can_drag(
    input::e_mouse_button /*unused*/, Coord origin ) {
  // If we're in the title bar then we'll drag; if we not, but
  // still in the window somewhere, we will "swallow" which means
  // that no other planes should get this drag even (because the
  // cursor is rightly in the window) but we don't want to handle
  // it ourselves because we only drag from the title bar.
  if( origin.is_inside( focused().title_bar() ) )
    return e_window_drag::yes;
  if( origin.is_inside( focused().rect() ) )
    return e_window_drag::swallow;
  return e_window_drag::no;
}

void WindowManager::on_drag( input::e_mouse_button button,
                             Coord /*unused*/, Coord prev,
                             Coord current ) {
  if( button == input::e_mouse_button::l )
    focused().position += ( current - prev );
}

WindowManager::window& WindowManager::focused() {
  CHECK( !windows_.empty() );
  return windows_[0];
}

/****************************************************************
** High-level Methods
*****************************************************************/
string select_box( string_view title, Vec<Str> options ) {
  logger->info( "question: \"{}\"", title );
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

  auto* win = g_window_plane.wm.add_window( string( title ),
                                            move( view ) );
  selector_ptr->grow_to( win->inside_border_rect().w );
  reset_fade_to_dark( chrono::milliseconds( 1500 ),
                      chrono::milliseconds( 3000 ), 65 );
  effects_plane_enable( true );
  frame_loop( true, finished );
  effects_plane_enable( false );
  logger->info( "selected: {}", selector_ptr->get_selected() );
  auto result = selector_ptr->get_selected();
  g_window_plane.wm.clear_windows();
  return result;
}

template<typename Enum>
Enum select_box_enum(
    std::string_view                          title,
    std::vector<std::pair<Enum, std::string>> options ) {
  // map over member function?
  std::vector<std::string> words;
  for( auto const& option : options )
    words.push_back( option.second );
  auto result = select_box( title, words );
  for( auto const& option : options )
    if( result == option.second ) return option.first;
  SHOULD_NOT_BE_HERE;
}

e_confirm yes_no( string_view title ) {
  vector<pair<e_confirm, string>> dict{
      {e_confirm::yes, "Yes"},
      {e_confirm::no, "No"},
  };
  auto res = select_box_enum( title, dict );
  return res;
}

/****************************************************************
** Testing Only
*****************************************************************/
void window_test() {
  auto finished = [] { return input::is_any_key_down(); };

  auto view =
      make_unique<ButtonView>( "Cancel" ); //, Delta{2_h, 9_w} );
  // view->set_enabled( false );
  // view->set_pressed( true );

  g_window_plane.wm.add_window( string( "Test Window" ),
                                move( view ) );
  frame_loop( true, finished );
  g_window_plane.wm.clear_windows();

  yes_no( "this is a test" );
}

} // namespace rn::ui
