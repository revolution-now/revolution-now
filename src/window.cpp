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
#include "frame.hpp"
#include "gfx.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "ownership.hpp"
#include "plane.hpp"
#include "ranges.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "tiles.hpp"
#include "typed-int.hpp"
#include "ui.hpp"
#include "unit.hpp"
#include "util.hpp"
#include "views.hpp"

// Revolution Now (config)
#include "../config/ucl/palette.inl"
#include "../config/ucl/ui.inl"

// base-util
#include "base-util/misc.hpp"
#include "base-util/optional.hpp"
#include "base-util/string.hpp"
#include "base-util/variant.hpp"

// function_ref
#include "tl/function_ref.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// range-v3
#include "range/v3/view/filter.hpp"

// c++ standard library
#include <algorithm>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>

using namespace std;

namespace rn::ui {

namespace {

/****************************************************************
** Window
*****************************************************************/
enum class e_window_state { running, closed };
struct Window {
  Window( std::string title_, Coord position_ );
  Window( std::string title_, std::unique_ptr<View> view_,
          Coord position_ );

  void set_view( UPtr<View> view_ ) {
    view = std::move( view_ );
  }

  void center_window() {
    // Here "main window" refers to the real window (recognized
    // by the OS) in which this game lives.
    position = centered( delta(), main_window_logical_rect() );
  }

  void close_window() { window_state = e_window_state::closed; }

  void  draw( Texture& tx ) const;
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

/****************************************************************
** WindowManager
*****************************************************************/
class WindowManager {
public:
  void draw_layout( Texture& tx ) const;

  ND bool input( input::event_t const& event );

  Plane::DragInfo can_drag( input::e_mouse_button button,
                            Coord                 origin );
  void            on_drag( input::mod_keys const& mod,
                           input::e_mouse_button button, Coord origin,
                           Coord prev, Coord current );

  auto active_windows() {
    return rv::all( windows_ ) //
           | rv::filter(
                 L( _.window_state != e_window_state::closed ) );
  }

  auto active_windows() const {
    return rv::all( windows_ ) //
           | rv::filter(
                 L( _.window_state != e_window_state::closed ) );
  }

  int num_windows() const {
    auto aw = active_windows();
    return std::distance( aw.begin(), aw.end() );
  }

  void remove_closed_windows();

public:
  Window* add_window( std::string title );

  Window* add_window( std::string           title,
                      std::unique_ptr<View> view );

  Window* add_window( std::string           title,
                      std::unique_ptr<View> view,
                      Coord                 position );

  void remove_window( Window* p_win ) {
    windows_.remove_if( LC( &_ == p_win ) );
  }

private:
  // Gets the window with focus, throws if no windows.
  Window& focused();

  // We need order and pointer stability here.
  list<Window> windows_;
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
  void on_frame_start() override { wm.remove_closed_windows(); }
  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    wm.draw_layout( tx );
  }
  bool input( input::event_t const& event ) override {
    CHECK( wm.num_windows() != 0 );
    // Windows are modal, so ignore the result of this function
    // because we need to swallow the event whether the window
    // manager handled it or not. I.e., we cannot ever let an
    // event go down to a lower plane.
    (void)wm.input( event );
    return true;
  }
  Plane::DragInfo can_drag( input::e_mouse_button button,
                            Coord origin ) override {
    CHECK( wm.num_windows() != 0 );
    return wm.can_drag( button, origin );
  }
  void on_drag( input::mod_keys const& mod,
                input::e_mouse_button button, Coord origin,
                Coord prev, Coord current ) override {
    CHECK( wm.num_windows() != 0 );
    wm.on_drag( mod, button, origin, prev, current );
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
Window::Window( string title_, unique_ptr<View> view_,
                Coord position_ )
  : window_state( e_window_state::running ),
    title( move( title_ ) ),
    view( move( view_ ) ),
    title_view(),
    position( position_ ) {
  CHECK( view );
  title_view = make_unique<OneLineStringView>(
      title, config_palette.orange.sat1.lum11, /*shadow=*/true );
}

Window::Window( string title_, Coord position_ )
  : window_state( e_window_state::running ),
    title( move( title_ ) ),
    view{},
    title_view(),
    position( position_ ) {
  title_view = make_unique<OneLineStringView>(
      title, config_palette.orange.sat1.lum11, /*shadow=*/true );
}

void Window::draw( Texture& tx ) const {
  CHECK( view );
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
Delta Window::delta() const {
  CHECK( view );
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

Rect Window::rect() const {
  return Rect::from( position, delta() );
}

Coord Window::inside_border() const {
  return position + window_border();
}

Coord Window::inside_padding() const {
  return position + window_border() + window_padding();
}

Rect Window::inside_padding_rect() const {
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

Rect Window::title_bar() const {
  CHECK( view );
  auto title_bar_rect = title_view->rect( inside_border() );
  title_bar_rect.w =
      std::max( title_bar_rect.w, view->delta().w );
  return title_bar_rect;
}

Coord Window::view_pos() const {
  return inside_padding() + title_view->delta().h;
}

void WindowManager::draw_layout( Texture& tx ) const {
  for( auto const& window : active_windows() ) window.draw( tx );
}

void WindowManager::remove_closed_windows() {
  // Don't use active_windows() here... would defeat the points.
  windows_.remove_if(
      L( _.window_state == e_window_state::closed ) );
}

Window* WindowManager::add_window( string           title_,
                                   unique_ptr<View> view_ ) {
  windows_.emplace_back( move( title_ ), move( view_ ),
                         Coord{} );
  auto& new_window = windows_.back();
  new_window.center_window();
  return &new_window;
}

Window* WindowManager::add_window( string title_ ) {
  windows_.emplace_back( move( title_ ), Coord{} );
  return &windows_.back();
}

bool WindowManager::input( input::event_t const& event ) {
  auto& win = focused();
  auto  view_rect =
      Rect::from( win.view_pos(), win.view->delta() );

  if_v( event, input::mouse_move_event_t, val ) {
    auto new_pos = val->pos;
    auto old_pos = val->prev;
    if( new_pos.is_inside( view_rect ) &&
        !old_pos.is_inside( view_rect ) )
      win.view->on_mouse_enter(
          new_pos.with_new_origin( view_rect.upper_left() ) );
    if( !new_pos.is_inside( view_rect ) &&
        old_pos.is_inside( view_rect ) )
      win.view->on_mouse_leave(
          old_pos.with_new_origin( view_rect.upper_left() ) );
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

Plane::DragInfo WindowManager::can_drag(
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
  // If it's not in our window then swallow it to prevent any
  // other plane from handling it (i.e., windows are modal).
  return Plane::e_accept_drag::swallow;
}

void WindowManager::on_drag( input::mod_keys const& /*unused*/,
                             input::e_mouse_button button,
                             Coord /*unused*/, Coord prev,
                             Coord current ) {
  if( button == input::e_mouse_button::l ) {
    auto& pos = focused().position;
    pos += ( current - prev );
    // Now prevent the window from being dragged off screen.
    pos.y =
        clamp( pos.y, 16_y,
               main_window_logical_rect().bottom_edge() - 16_h );
    pos.x =
        clamp( pos.x, 0_x - focused().delta().w + 16_w,
               main_window_logical_rect().right_edge() - 16_w );
  }
}

Window& WindowManager::focused() {
  CHECK( num_windows() > 0 );
  return *active_windows().begin();
}

/****************************************************************
** Validators
*****************************************************************/
// These should probably be moved elsewhere.

ValidatorFunc make_int_validator( Opt<int> min, Opt<int> max ) {
  return [min, max]( std::string const& proposed ) {
    auto maybe_int = util::stoi( proposed );
    if( !maybe_int.has_value() ) return false;
    if( min.has_value() && *maybe_int < *min ) return false;
    if( max.has_value() && *maybe_int > *max ) return false;
    return true;
  };
}

/****************************************************************
** Windows
*****************************************************************/
void async_window_builder(
    std::string_view                        title,
    tl::function_ref<UPtr<View>( Window* )> get_view_fn ) {
  auto* win  = g_window_plane.wm.add_window( string( title ) );
  auto  view = get_view_fn( win );
  autopad( view, /*use_fancy=*/false );
  win->set_view( std::move( view ) );
  win->center_window();
}

using GetOkCancelSubjectViewFunc = function<UPtr<View>(
    function<void( bool )> /*enable_ok_button*/ //
    )>;

template<typename ResultT>
void ok_cancel_window_builder(
    string_view title, function<ResultT()> get_result,
    function<bool( ResultT const& )> validator,
    // on_result must be copyable.
    function<void( Opt<ResultT> )> on_result,
    GetOkCancelSubjectViewFunc     get_view_fn ) {
  async_window_builder( title, [=]( auto* win ) {
    auto ok_cancel_view = make_unique<OkCancelView>(
        /*on_ok=*/
        [win, on_result, validator{std::move( validator )},
         get_result{std::move( get_result )}] {
          lg.trace( "selected ok." );
          decltype( auto ) proposed = get_result();
          if( validator( proposed ) ) {
            on_result( proposed );
            win->close_window();
          } else {
            lg.debug( "{} is invalid.", proposed );
          }
        }, /*on_cancel=*/
        [win, on_result] {
          lg.trace( "selected cancel." );
          on_result( nullopt );
          win->close_window();
        } );
    auto* p_ok_button      = ok_cancel_view->ok_button();
    auto  enable_ok_button = [p_ok_button]( bool enable ) {
      p_ok_button->enable( enable );
    };
    auto subject_view = get_view_fn(
        /*enable_ok_button=*/std::move( enable_ok_button ) //
    );
    Vec<UPtr<View>> view_vec;
    view_vec.emplace_back( std::move( subject_view ) );
    view_vec.emplace_back( std::move( ok_cancel_view ) );
    auto view = make_unique<VerticalArrayView>(
        std::move( view_vec ),
        VerticalArrayView::align::center );
    return view;
  } );
}

void ok_cancel( string_view                   msg,
                function<void( e_ok_cancel )> on_result ) {
  auto on_ok_cancel_result =
      [on_result{std::move( on_result )}]( Opt<int> o ) {
        if( o.has_value() ) return on_result( e_ok_cancel::ok );
        on_result( e_ok_cancel::cancel );
      };
  TextMarkupInfo m_info{
      /*normal=*/config_ui.dialog_text.normal,
      /*highlight=*/config_ui.dialog_text.highlighted};
  TextReflowInfo r_info{
      /*max_cols=*/config_ui.dialog_text.columns};
  auto view =
      make_unique<TextView>( string( msg ), m_info, r_info );

  // We can capture by reference here because the function will
  // be called before this scope exits.
  GetOkCancelSubjectViewFunc get_view_fn =
      [&]( function<void( bool )> /*enable_ok_button*/ ) {
        return std::move( view );
      };

  // Use <int> for lack of anything better.
  ok_cancel_window_builder<int>(
      /*title=*/"Question",
      /*get_result=*/L0( 0 ),
      /*validator=*/L( _ == 0 ), // always true.
      /*on_result=*/std::move( on_ok_cancel_result ),
      /*get_view_fn=*/std::move( get_view_fn ) //
  );
}

void text_input_box( string_view title, string_view msg,
                     ValidatorFunc                 validator,
                     function<void( Opt<string> )> on_result ) {
  TextMarkupInfo m_info{
      /*normal=*/config_ui.dialog_text.normal,
      /*highlight=*/config_ui.dialog_text.highlighted};
  TextReflowInfo r_info{
      /*max_cols=*/config_ui.dialog_text.columns};
  auto text =
      make_unique<TextView>( string( msg ), m_info, r_info );
  auto le_view =
      make_unique<LineEditorView>( /*chars_wide=*/20 );
  LineEditorView* p_le_view = le_view.get();

  // We can capture by reference here because the function will
  // be called before this scope exits.
  GetOkCancelSubjectViewFunc get_view_fn{
      [&]( function<void( bool )> enable_ok_button ) {
        LineEditorView::OnChangeFunc on_change =
            [validator,
             enable_ok_button{std::move( enable_ok_button )}](
                string const& new_str ) {
              if( validator( new_str ) )
                enable_ok_button( true );
              else
                enable_ok_button( false );
            };
        // Make sure initial state of button is consistent with
        // initial validation result.
        on_change( le_view->text() );
        le_view->set_on_change_fn( std::move( on_change ) );
        Vec<UPtr<View>> view_vec;
        view_vec.emplace_back( std::move( text ) );
        view_vec.emplace_back( std::move( le_view ) );
        return make_unique<VerticalArrayView>(
            std::move( view_vec ),
            VerticalArrayView::align::center );
      }};

  ok_cancel_window_builder<string>(
      /*title=*/title,
      /*get_result=*/
      [p_le_view]() -> string { return p_le_view->text(); },
      /*validator=*/validator, // must be copied.
      /*on_result=*/std::move( on_result ),
      /*get_view_fun=*/std::move( get_view_fn ) //
  );
}

void int_input_box( std::string_view title, std::string_view msg,
                    std::function<void( Opt<int> )> on_result,
                    Opt<int> min, Opt<int> max ) {
  using namespace util::infix;
  text_input_box(
      title, msg, make_int_validator( min, max ),
      [on_result{std::move( on_result )}]( Opt<string> result ) {
        on_result( result | fmap_join( L( util::stoi( _ ) ) ) );
      } );
}

/****************************************************************
** High-level Methods
*****************************************************************/
string select_box( string_view title, Vec<Str> options ) {
  lg.info( "question: \"{}\"", title );

  auto selector = make_unique<OptionSelectView>(
      options, /*initial_selection=*/0 );
  auto* selector_ptr = selector.get();
  auto  finished     = [selector_ptr] {
    return selector_ptr->confirmed();
  };

  UPtr<View> view( std::move( selector ) );
  autopad( view, /*use_fancy=*/false );

  auto* win = g_window_plane.wm.add_window( string( title ),
                                            move( view ) );
  selector_ptr->grow_to( win->inside_padding_rect().w );
  reset_fade_to_dark( chrono::milliseconds( 1500 ),
                      chrono::milliseconds( 3000 ), 65 );
  effects_plane_enable( true );
  frame_loop( true, finished );
  effects_plane_enable( false );
  lg.info( "selected: {}", selector_ptr->get_selected() );
  auto result = selector_ptr->get_selected();
  win->close_window();
  return result;
}

e_confirm yes_no( string_view title ) {
  return select_box_enum<e_confirm>( title );
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
  autopad( view, /*use_fancy=*/false );

  auto* win = g_window_plane.wm.add_window( string( "Alert!" ),
                                            move( view ) );
  frame_loop( true, [&] { return pressed_ok; } );

  g_window_plane.wm.remove_window( win );
}

Vec<UnitSelection> unit_selection_box( Vec<UnitId> const& ids_,
                                       bool allow_activation ) {
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
          lg.debug( "clicked on {}", debug_string( id ) );
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
  autopad( view, /*use_fancy=*/false, 4 );

  auto* win = g_window_plane.wm.add_window(
      string( "Activate Units" ), move( view ) );

  frame_loop( true, [&state] { return state != nullopt; } );
  lg.info( "pressed `{}`.", state );

  Vec<UnitSelection> res;

  if( state == e_ok_cancel::ok ) {
    for( auto const& [id, info] : infos ) {
      if( info.is_activated ) {
        CHECK( info.current_orders == e_unit_orders::none );
        res.push_back( {id, e_unit_selection::activate} );
      } else if( info.current_orders != info.original_orders ) {
        CHECK( info.current_orders == e_unit_orders::none );
        res.push_back( {id, e_unit_selection::clear_orders} );
      }
    }
  }

  win->close_window();

  for( auto r : res )
    lg.debug( "selection: {} --> {}", debug_string( r.id ),
              r.what );
  return res;
}

/****************************************************************
** Testing Only
*****************************************************************/
void window_test() {
  int magic = 0;
  while( magic != 33 ) {
    int_input_box(
        /*title=*/"Line Editor Test",
        /*msg=*/"Please enter a valid number between 1-100:",
        /*on_result=*/
        [&]( Opt<int> result ) {
          lg.info( "result: {}", result );
          if( result ) magic = result.value();
        },
        /*min=*/1,
        /*max=*/100 );

    // ------------------------------------------------------------
    frame_loop( true,
                L0( g_window_plane.wm.num_windows() == 0 ) );
  }
  lg.info( "exiting." );
}

} // namespace rn::ui
