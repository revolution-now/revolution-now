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
#include "auto-pad.hpp"
#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "frame.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "ownership.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"
#include "typed-int.hpp"
#include "ui.hpp"
#include "unit.hpp"
#include "util.hpp"
#include "views.hpp"

// base-util
#include "base-util/misc.hpp"
#include "base-util/variant.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

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

  Plane::e_accept_drag can_drag( input::e_mouse_button button,
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
    Coord inside_padding() const;
    Rect  inside_padding_rect() const;
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

  void remove_window( window* ) {
    // FIXME
    clear_windows();
  }

  void clear_windows();

private:
  // Gets the window with focus, throws if no windows.
  window& focused();

  // FIXME: should use a container with pointer stability here.
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
    return wm.can_drag( button, origin );
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

// Number of pixels of padding between border and start of the
// window's view.
Delta const& window_padding() {
  static Delta const delta{W( config_ui.window.window_padding ),
                           H( config_ui.window.window_padding )};
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
      Rect::from( position + Delta{4_w, 4_h}, win_size ) );
  render_fill_rect(
      tx, config_palette.orange.sat0.lum1,
      {position.x, position.y, win_size.w, win_size.h} );
  auto inside_border = position + window_border();
  auto inner_size    = win_size - Scale( 2 ) * window_border();
  render_fill_rect( tx, config_palette.orange.sat1.lum4,
                    Rect::from( inside_border, inner_size ) );
  auto title_start =
      centered( title_view->delta(), title_bar() );
  title_view->draw( tx, title_start );
  view->draw( tx, inside_padding() + title_view->delta().h );
}

// Includes border
Delta WindowManager::window::delta() const {
  Delta res;
  res.w = std::max(
      title_view->delta().w,
      W( view->delta().w + window_padding().w * 2_sx ) );
  res.h += title_view->delta().h + view->delta().h +
           window_padding().h * 2_sy;
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

Coord WindowManager::window::inside_padding() const {
  return position + window_border() + window_padding();
}

Rect WindowManager::window::inside_padding_rect() const {
  auto res = rect();
  res.x += window_border().w;
  res.y += window_border().h;
  res.w -= window_border().w * 2_sx;
  res.h -= window_border().h * 2_sy;
  res.x += window_padding().w;
  res.y += window_padding().h;
  res.w -= window_padding().w * 2_sx;
  res.h -= window_padding().h * 2_sy;
  return res;
}

Rect WindowManager::window::title_bar() const {
  auto title_bar_rect = title_view->rect( inside_border() );
  title_bar_rect.w =
      std::max( title_bar_rect.w, view->delta().w );
  return title_bar_rect;
}

Coord WindowManager::window::view_pos() const {
  return inside_padding() + title_view->delta().h;
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

void WindowManager::clear_windows() { windows_.clear(); }

bool WindowManager::input( input::event_t const& event ) {
  auto& win = focused();
  auto  view_rect =
      Rect::from( win.view_pos(), win.view->delta() );

  if_v( event, input::mouse_move_event_t, val ) {
    auto new_pos = val->pos;
    auto old_pos = val->prev;
    // We need to find which views (if any) have been entered or
    // exited by the mouse as a result of this move and send them
    // any relevant on_mouse_leave and on_mouse_enter messages.
    Object::ObjectSet views_under_new_cursor;
    Object::ObjectSet views_under_old_cursor;
    if( new_pos.is_inside( view_rect ) ) {
      views_under_new_cursor.insert( win.view.get() );
      win.view->children_under_coord(
          new_pos.with_new_origin( win.view_pos() ),
          views_under_new_cursor );
    }
    if( old_pos.is_inside( view_rect ) ) {
      views_under_old_cursor.insert( win.view.get() );
      win.view->children_under_coord(
          old_pos.with_new_origin( win.view_pos() ),
          views_under_old_cursor );
    }
    // These loops are taking the disjunction of the two sets,
    // i.e., finding the elements in one that are not in the
    // other. Somehow there does not seem to be an API method for
    // that.
    for( auto* obj : views_under_new_cursor ) {
      if( views_under_old_cursor.contains( obj ) ) continue;
      obj->on_mouse_enter();
    }
    for( auto* obj : views_under_old_cursor ) {
      if( views_under_new_cursor.contains( obj ) ) continue;
      obj->on_mouse_leave();
    }
  }

  if( input::is_mouse_event( event ) ) {
    auto maybe_pos = input::mouse_position( event );
    CHECK( maybe_pos.has_value() );
    // Only send the event if the mouse position is within the
    // view. And, when we send it, we make the mouse position
    // relative to the upper left corner of the view.
    if( maybe_pos.value().get().is_inside( view_rect ) ) {
      auto new_event = input::move_mouse_origin_by(
          event, win.view_pos() - Coord{} );
      // Always return true, meaning that we handle the mouse
      // event if we are inside a view.
      return (void)win.view->input( new_event ), true;
    }
  } else {
    // It's a non-mouse event, so just send it and return if it
    // was handled.
    return win.view->input( event );
  }
  return false;
}

Plane::e_accept_drag WindowManager::can_drag(
    input::e_mouse_button /*unused*/, Coord origin ) {
  // If we're in the title bar then we'll drag; if we not, but
  // still in the window somewhere, we will "swallow" which means
  // that no other planes should get this drag even (because the
  // cursor is rightly in the window) but we don't want to handle
  // it ourselves because we only drag from the title bar.
  if( origin.is_inside( focused().title_bar() ) )
    return Plane::e_accept_drag::yes;
  if( origin.is_inside( focused().rect() ) )
    // Receive drag events as normal mouse events.
    return Plane::e_accept_drag::motion;
  return Plane::e_accept_drag::no;
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

  auto selector = make_unique<OptionSelectView>(
      options, /*initial_selection=*/0 );
  auto* selector_ptr = selector.get();
  auto  finished     = [selector_ptr] {
    return selector_ptr->confirmed();
  };

  auto* win = g_window_plane.wm.add_window( string( title ),
                                            move( selector ) );
  selector_ptr->grow_to( win->inside_padding_rect().w );
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

void message_box( std::string_view msg ) {
  bool pressed_ok = false;

  auto button = make_unique<ButtonView>(
      "OK", Delta{2_h, 8_w}, [&] { pressed_ok = true; } );

  vector<unique_ptr<View>> view_vec;
  view_vec.emplace_back( make_unique<OneLineStringView>(
      string( msg ), Color::banana(), /*shadow=*/false ) );
  view_vec.emplace_back( std::move( button ) );

  auto msg_view = make_unique<VerticalArrayView>(
      std::move( view_vec ), VerticalArrayView::align::center );

  UPtr<View> view( std::move( msg_view ) );
  autopad( view );

  auto* win = g_window_plane.wm.add_window( string( "Alert!" ),
                                            move( view ) );
  frame_loop( true, [&] { return pressed_ok; } );

  g_window_plane.wm.remove_window( win );
}

Vec<UnitSelectionResult> unit_selection_box(
    Vec<UnitId> const& ids_, bool allow_activation ) {
  /* First we assemble this structure:
   *
   * OkCancelAdapter
   * +-VerticalScrollView // TODO
   *   +-VerticalArrayView
   *     |-...
   *     |-HorizontalArrayView
   *     | |-OneLineTextView
   *     | +-AddSelectBorderView
   *     |   +-ClickableView
   *     |     +-FakeUnitView
   *     |       +-SpriteView
   *     +-...
   */

  g_window_plane.wm.clear_windows();

  auto cmp = []( UnitId l, UnitId r ) {
    auto const& unit1 = unit_from_id( l ).desc();
    auto const& unit2 = unit_from_id( r ).desc();
    if( unit1.boat && !unit2.boat ) return true;
    if( unit1.cargo_slots > unit2.cargo_slots ) return true;
    if( unit1.attack_points > unit2.attack_points ) return true;
    return false;
  };

  auto ids = ids_;
  sort( ids.begin(), ids.end(), cmp );

  Vec<UPtr<View>> units_vec;

  // Holds the state of each unit in the window as the player is
  // selecting them and cycling them through the states.
  struct UnitInfo {
    e_unit_orders original_orders;
    e_unit_orders current_orders;
    bool          is_activated;
  };

  absl::flat_hash_map<UnitId, UnitInfo> infos;
  for( auto id : ids ) {
    auto const& unit = unit_from_id( id );

    UnitInfo info{/*original_orders=*/unit.orders(),
                  /*current_orders=*/unit.orders(),
                  /*is_activated=*/false};
    infos[id] = info;
  }
  CHECK( ids.size() == infos.size() );

  /* Each click on a unit cycles it through one the following cy-
   * cles depending on whether it initially had orders and
   * whether or not it is the end of turn:
   *
   * Orders=Fortified:
   *   Not End-of-turn:
   *     Fortified --> No Orders --> No Orders+Prio --> ...
   *   End-of-turn:
   *     Fortified --> No Orders --> ...
   * Orders=None:
   *   Not End-of-turn:
   *     No Orders --> No Orders+Prioritized --> ...
   *   End-of-turn:
   *     No Orders --> ...
   */
  auto on_click_unit = [&infos, allow_activation]( UnitId id ) {
    CHECK( infos.contains( id ) );
    UnitInfo& info = infos[id];
    if( info.original_orders != e_unit_orders::none ) {
      if( allow_activation ) {
        // Orders --> No Orders --> No Orders+Prio --> ...
        if( info.current_orders != e_unit_orders::none ) {
          CHECK( !info.is_activated );
          info.current_orders = e_unit_orders::none;
        } else if( info.is_activated ) {
          CHECK( info.current_orders == e_unit_orders::none );
          info.current_orders = info.original_orders;
          info.is_activated   = false;
        } else {
          CHECK( !info.is_activated );
          info.is_activated = true;
        }
      } else {
        // Orders --> No Orders --> ...
        CHECK( !info.is_activated );
        if( info.current_orders != e_unit_orders::none ) {
          info.current_orders = e_unit_orders::none;
        } else {
          info.current_orders = info.original_orders;
        }
      }
    } else {
      if( allow_activation ) {
        // No Orders --> No Orders+Prioritized --> ...
        CHECK( info.original_orders == e_unit_orders::none );
        CHECK( info.current_orders == e_unit_orders::none );
        info.is_activated = !info.is_activated;
      } else {
        // No Orders --> ...
        CHECK( info.original_orders == e_unit_orders::none );
        CHECK( info.current_orders == e_unit_orders::none );
        CHECK( !info.is_activated );
      }
    }
  };

  for( auto id : ids ) {
    auto const& unit = unit_from_id( id );

    auto fake_unit_view = make_unique<FakeUnitView>(
        unit.desc().type, unit.nation(), unit.orders() );

    auto* fake_unit_view_ptr = fake_unit_view.get();

    auto border_view = make_unique<BorderView>(
        std::move( fake_unit_view ), Color::white(),
        /*padding=*/1,
        /*on_initially=*/false );

    auto* border_view_ptr = border_view.get();

    auto clickable = make_unique<ClickableView>(
        // Capture local variables by value.
        std::move( border_view ),
        [&infos, id, fake_unit_view_ptr, border_view_ptr,
         &on_click_unit] {
          logger->debug( "clicked on {}", debug_string( id ) );
          on_click_unit( id );
          CHECK( infos.contains( id ) );
          auto& info = infos[id];
          fake_unit_view_ptr->set_orders( info.current_orders );
          border_view_ptr->on( info.is_activated );
        } );

    auto unit_label = make_unique<OneLineStringView>(
        unit.desc().name, Color::banana(), /*shadow=*/true );

    Vec<UPtr<View>> horizontal_vec;
    horizontal_vec.emplace_back( std::move( clickable ) );
    horizontal_vec.emplace_back( std::move( unit_label ) );
    auto horizontal_view = make_unique<HorizontalArrayView>(
        std::move( horizontal_vec ),
        HorizontalArrayView::align::middle );

    units_vec.emplace_back( std::move( horizontal_view ) );
  }
  auto arr_view = make_unique<VerticalArrayView>(
      std::move( units_vec ), VerticalArrayView::align::left );

  Opt<e_ok_cancel> state;

  auto adapter = make_unique<OkCancelAdapterView>(
      std::move( arr_view ),
      [&state]( auto button ) { state = button; } );

  UPtr<View> view( std::move( adapter ) );
  autopad( view, 4 );

  g_window_plane.wm.add_window( string( "Test Window" ),
                                move( view ) );

  frame_loop( true, [&state] { return state != nullopt; } );
  logger->info( "pressed `{}`.", state );

  Vec<UnitSelectionResult> res;

  if( state == e_ok_cancel::ok ) {
    for( auto const& [id, info] : infos ) {
      if( info.is_activated ) {
        CHECK( info.current_orders == e_unit_orders::none );
        res.push_back( {id, e_unit_selection_result::activate} );
      } else if( info.current_orders != info.original_orders ) {
        CHECK( info.current_orders == e_unit_orders::none );
        res.push_back(
            {id, e_unit_selection_result::clear_orders} );
      }
    }
  }

  g_window_plane.wm.clear_windows();

  for( auto r : res )
    logger->debug( "selection: {} --> {}", debug_string( r.id ),
                   r.result );
  return res;
}

/****************************************************************
** Testing Only
*****************************************************************/
void window_test() {
  unit_selection_box( {0_id, 1_id, 2_id},
                      /*allow_activation=*/true );
}

} // namespace rn::ui
