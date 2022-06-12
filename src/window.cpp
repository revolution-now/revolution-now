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
#include "auto-pad.hpp"
#include "co-wait.hpp"
#include "compositor.hpp"
#include "error.hpp"
#include "game-state.hpp"
#include "game-ui-views.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "tiles.hpp"
#include "typed-int.hpp"
#include "ui.hpp"
#include "unit.hpp"
#include "ustate.hpp"
#include "util.hpp"
#include "views.hpp"

// config
#include "config/ui.rds.hpp"

// render
#include "render/renderer.hpp"

// base
#include "base/conv.hpp"
#include "base/function-ref.hpp"
#include "base/lambda.hpp"
#include "base/range-lite.hpp"

// c++ standard library
#include <algorithm>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>

using namespace std;

namespace rn {

namespace rl = ::base::rl;

using ::base::function_ref;

/****************************************************************
** Window
*****************************************************************/
struct Window {
  Window( WindowPlane& window_plane, std::string title_,
          Coord position_ );
  // Removes this window from the window manager.
  ~Window() noexcept;

  void set_view( unique_ptr<ui::View> view_ ) {
    view = std::move( view_ );
  }

  void center_window() {
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    position = centered( delta(), normal_area );
  }

  bool operator==( Window const& rhs ) const {
    return this == &rhs;
  }

  void  draw( rr::Renderer& renderer ) const;
  Delta delta() const;
  Rect  rect() const;
  Coord inside_border() const;
  Rect  inside_border_rect() const;
  Coord inside_padding() const;
  Rect  inside_padding_rect() const;
  Rect  title_bar() const;
  // abs coord of upper-left corner of view.
  Coord view_pos() const;

  WindowPlane&                           window_plane_;
  std::string                            title;
  std::unique_ptr<ui::View>              view;
  std::unique_ptr<ui::OneLineStringView> title_view;
  Coord                                  position;
};

/****************************************************************
** WindowManager
*****************************************************************/
class WindowManager {
 public:
  void draw_layout( rr::Renderer& renderer ) const;

  ND e_input_handled input( input::event_t const& event );

  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord                 origin );
  void                 on_drag( input::mod_keys const& mod,
                                input::e_mouse_button button, Coord origin,
                                Coord prev, Coord current );

  vector<Window*>& active_windows() { return windows_; }

  vector<Window*> const& active_windows() const {
    return windows_;
  }

  int num_windows() const { return windows_.size(); }

 public:
  void add_window( Window* win ) { windows_.push_back( win ); }

  void remove_window( Window* p_win ) {
    erase_if( windows_, LC( _ == p_win ) );
    if( dragging_win_ == p_win ) dragging_win_ = nullptr;
  }

 private:
  // Gets the window with focus, throws if no windows.
  Window& focused();

  // Get the top-most window under the cursor position.
  maybe<Window&> window_for_cursor_pos( Coord const& pos );
  // Same as above but only when cursor is in a window's view.
  maybe<Window&> window_for_cursor_pos_in_view(
      Coord const& pos );

  vector<Window*> windows_;
  // The value of this is only relevant during a drag.
  Window* dragging_win_ = nullptr;
};
NOTHROW_MOVE( WindowManager );

/****************************************************************
** WindowPlane::Impl
*****************************************************************/
struct WindowPlane::Impl : public Plane {
  Impl() = default;
  bool covers_screen() const override { return false; }
  void draw( rr::Renderer& renderer ) const override {
    wm.draw_layout( renderer );
  }
  e_input_handled input( input::event_t const& event ) override {
    return wm.input( event );
  }
  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    return wm.can_drag( button, origin );
  }
  void on_drag( input::mod_keys const& mod,
                input::e_mouse_button button, Coord origin,
                Coord prev, Coord current ) override {
    CHECK( wm.num_windows() != 0 );
    wm.on_drag( mod, button, origin, prev, current );
  }
  WindowManager wm;
};

namespace {

/****************************************************************
** Misc. Helpers
*****************************************************************/
// This returns the width in pixels of the window border ( same
// for left/right sides as for top/bottom sides). Don't forget
// that this Delta must be multiplied by two to get the total
// change in width/height of a window with such a border.
Delta const& window_border() {
  static Delta const delta{ W( config_ui.window.border_width ),
                            H( config_ui.window.border_width ) };
  return delta;
}

// Number of pixels of padding between border and start of the
// window's view.
Delta const& window_padding() {
  static Delta const delta{
      W( config_ui.window.window_padding ),
      H( config_ui.window.window_padding ) };
  return delta;
}

} // namespace

/****************************************************************
** WindowManager
*****************************************************************/
Window::Window( WindowPlane& window_plane, string title_,
                Coord position_ )
  : window_plane_( window_plane ),
    title( move( title_ ) ),
    view{},
    title_view{},
    position( position_ ) {
  title_view = make_unique<ui::OneLineStringView>(
      title,
      gfx::pixel{ .r = 0xE4, .g = 0xC8, .b = 0x90, .a = 255 } );
  window_plane_.impl_->wm.add_window( this );
}

Window::~Window() noexcept {
  window_plane_.impl_->wm.remove_window( this );
}

void Window::draw( rr::Renderer& renderer ) const {
  CHECK( view );
  rr::Painter painter = renderer.painter();
  Rect        r       = rect();
  // Render shadow behind window.
  painter.draw_solid_rect( r + Delta{ 4_w, 4_h },
                           gfx::pixel{ 0, 0, 0, 64 } );
  painter.draw_solid_rect(
      inside_border_rect(),
      gfx::pixel{ .r = 0x58, .g = 0x3C, .b = 0x30, .a = 255 } );
  // Render window border, highlights on top and right.
  painter.draw_horizontal_line(
      r.lower_left(), r.w._,
      gfx::pixel{ .r = 0x42, .g = 0x2D, .b = 0x22, .a = 255 } );
  painter.draw_vertical_line(
      r.upper_left(), r.h._,
      gfx::pixel{ .r = 0x42, .g = 0x2D, .b = 0x22, .a = 255 } );
  painter.draw_horizontal_line(
      r.upper_left(), r.w._,
      gfx::pixel{ .r = 0x6D, .g = 0x49, .b = 0x3C, .a = 255 } );
  painter.draw_vertical_line(
      r.upper_right(), r.h._,
      gfx::pixel{ .r = 0x6D, .g = 0x49, .b = 0x3C, .a = 255 } );
  painter.draw_point(
      r.upper_left(),
      gfx::pixel{ .r = 0x58, .g = 0x3C, .b = 0x30, .a = 255 } );
  painter.draw_point(
      r.lower_right(),
      gfx::pixel{ .r = 0x58, .g = 0x3C, .b = 0x30, .a = 255 } );

  auto title_start =
      centered( title_view->delta(), title_bar() );
  title_view->draw( renderer, title_start );
  view->draw( renderer, view_pos() );
}

// Includes border
Delta Window::delta() const {
  CHECK( view );
  Delta res;
  res.w = std::max( title_view->delta().w, view->delta().w );
  res.h += title_view->delta().h + view->delta().h +
           window_padding().h * 2_sy;
  // Padding inside window border.
  res.w += config_ui.window.window_padding * 2;
  // Padding under title bar.
  res.h += config_ui.window.ui_padding;
  // multiply by two since there is top/bottom or left/right.
  res += Scale( 2 ) * window_border();
  return res;
}

Rect Window::rect() const {
  return Rect::from( position, delta() );
}

// Coord Window::inside_border() const {
//   return position + window_border();
// }

Rect Window::inside_border_rect() const {
  return Rect::from( position + window_border(),
                     delta() - window_border() );
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
  auto title_bar_rect = title_view->rect( inside_padding() );
  title_bar_rect.w =
      std::max( title_bar_rect.w, view->delta().w );
  return title_bar_rect;
}

Coord Window::view_pos() const {
  return inside_padding() + title_view->delta().h +
         H{ config_ui.window.ui_padding };
}

void WindowManager::draw_layout( rr::Renderer& renderer ) const {
  for( Window* window : active_windows() )
    window->draw( renderer );
}

maybe<Window&> WindowManager::window_for_cursor_pos(
    Coord const& pos ) {
  for( Window* win : rl::rall( active_windows() ) )
    if( pos.is_inside( win->rect() ) ) //
      return *win;
  return nothing;
}

maybe<Window&> WindowManager::window_for_cursor_pos_in_view(
    Coord const& pos ) {
  for( Window* win : rl::rall( active_windows() ) ) {
    auto view_rect =
        Rect::from( win->view_pos(), win->view->delta() );
    if( pos.is_inside( view_rect ) ) //
      return *win;
  }
  return nothing;
}

e_input_handled WindowManager::input(
    input::event_t const& event ) {
  // Since windows are model we will always declare that we've
  // handled the event, unless there are no windows open.
  if( this->num_windows() == 0 ) return e_input_handled::no;

  maybe<input::mouse_event_base_t const&> mouse_event =
      input::is_mouse_event( event );
  if( !mouse_event ) {
    // It's a non-mouse event, so just send it to the top-most
    // window and declare it to be handled.
    (void)focused().view->input( event );
    return e_input_handled::yes;
  }

  // It's a mouse event.
  maybe<Window&> win = window_for_cursor_pos( mouse_event->pos );
  if( !win )
    // Only send mouse events when the cursor is over a window.
    // But still declare that we've handled the event.
    return e_input_handled::yes;
  auto view_rect =
      Rect::from( win->view_pos(), win->view->delta() );

  // If it's a movement event then see if we need to send
  // on-leave and on-enter notifications to any views that are
  // being entered or left.
  if( auto* val =
          std::get_if<input::mouse_move_event_t>( &event ) ) {
    auto           new_pos = val->pos;
    auto           old_pos = val->prev;
    maybe<Window&> old_view =
        window_for_cursor_pos_in_view( old_pos );
    maybe<Window&> new_view =
        window_for_cursor_pos_in_view( new_pos );
    if( old_view != new_view ) {
      if( old_view )
        old_view->view->on_mouse_leave(
            old_pos.with_new_origin( view_rect.upper_left() ) );
      if( new_view )
        new_view->view->on_mouse_enter(
            new_pos.with_new_origin( view_rect.upper_left() ) );
    }
  }

  // Only send the event if the mouse position is within the
  // view. And, when we send it, we make the mouse position rela-
  // tive to the upper left corner of the view.
  if( mouse_event->pos.is_inside( view_rect ) ) {
    auto new_event = input::move_mouse_origin_by(
        event, win->view_pos() - Coord{} );
    (void)win->view->input( new_event );
  }
  // Always handle the event if there is a window open.
  return e_input_handled::yes;
}

Plane::e_accept_drag WindowManager::can_drag(
    input::e_mouse_button /*unused*/, Coord origin ) {
  if( num_windows() == 0 ) return Plane::e_accept_drag::no;
  maybe<Window&> win = window_for_cursor_pos( origin );
  // If it's not in a window then swallow it to prevent any other
  // plane from handling it.
  if( !win ) return Plane::e_accept_drag::swallow;
  dragging_win_ = &*win;
  // If we're in the title bar then we'll drag; if we not, but
  // still in the window somewhere, we will "swallow" which means
  // that no other planes should get this drag even (because the
  // cursor is rightly in the window) but we don't want to handle
  // it ourselves because we only drag from the title bar.
  if( origin.is_inside( win->title_bar() ) )
    return Plane::e_accept_drag::yes;
  // Receive drag events as normal mouse events.
  return Plane::e_accept_drag::motion;
}

void WindowManager::on_drag( input::mod_keys const& /*unused*/,
                             input::e_mouse_button button,
                             Coord /*unused*/, Coord prev,
                             Coord current ) {
  if( dragging_win_ == nullptr )
    // This can happen if the window is destroyed while dragging
    // is in progress, which can happen if a wait that owns
    // the window is cancelled.
    return;
  if( button == input::e_mouse_button::l ) {
    auto& pos = dragging_win_->position;
    pos += ( current - prev );
    // Now prevent the window from being dragged off screen.
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    pos.y =
        clamp( pos.y, 16_y, normal_area.bottom_edge() - 16_h );
    pos.x = clamp( pos.x, 0_x - focused().delta().w + 16_w,
                   normal_area.right_edge() - 16_w );
  }
}

Window& WindowManager::focused() {
  CHECK( num_windows() > 0 );
  return *active_windows().back();
}

/****************************************************************
** Validators
*****************************************************************/
// These should probably be moved elsewhere.

ui::ValidatorFunc make_int_validator( maybe<int> min,
                                      maybe<int> max ) {
  return [min, max]( std::string const& proposed ) {
    auto maybe_int = base::stoi( proposed );
    if( !maybe_int.has_value() ) return false;
    if( min.has_value() && *maybe_int < *min ) return false;
    if( max.has_value() && *maybe_int > *max ) return false;
    return true;
  };
}

/****************************************************************
** Windows
*****************************************************************/
// We need to have pointer stability on the returned window since
// its address needs to go into callbacks.
[[nodiscard]] unique_ptr<Window> async_window_builder(
    WindowPlane& window_plane, std::string_view title,
    unique_ptr<ui::View> view, bool auto_pad ) {
  auto win = make_unique<Window>( window_plane, string( title ),
                                  Coord{} );
  if( auto_pad ) autopad( *view, /*use_fancy=*/false );
  win->set_view( std::move( view ) );
  win->center_window();
  return win;
}

using GetOkCancelSubjectViewFunc = unique_ptr<ui::View>(
    function<void( bool )> /*enable_ok_button*/ //
);

template<typename ResultT>
[[nodiscard]] unique_ptr<Window> ok_cancel_window_builder(
    WindowPlane& window_plane, string_view title,
    function<ResultT()>              get_result,
    function<bool( ResultT const& )> validator,
    // on_result must be copyable.
    function<void( maybe<ResultT> )>         on_result,
    function_ref<GetOkCancelSubjectViewFunc> get_view_fn ) {
  auto ok_cancel_view = make_unique<ui::OkCancelView>(
      /*on_ok=*/
      [on_result, validator{ std::move( validator ) },
       get_result{ std::move( get_result ) }] {
        lg.trace( "selected ok." );
        decltype( auto ) proposed = get_result();
        if( validator( proposed ) ) {
          on_result( proposed );
        } else {
          if constexpr( base::Show<decltype( proposed )> ) {
            lg.debug( "{} is invalid.", proposed );
          } else {
            lg.debug( "result is invalid." );
          }
        }
      }, /*on_cancel=*/
      [on_result] {
        lg.trace( "selected cancel." );
        on_result( nothing );
      } );
  auto* p_ok_button      = ok_cancel_view->ok_button();
  auto  enable_ok_button = [p_ok_button]( bool enable ) {
    p_ok_button->enable( enable );
  };
  unique_ptr<ui::View> subject_view = get_view_fn(
      /*enable_ok_button=*/std::move( enable_ok_button ) //
  );
  CHECK( subject_view != nullptr );
  vector<unique_ptr<ui::View>> view_vec;
  view_vec.emplace_back( std::move( subject_view ) );
  view_vec.emplace_back( std::move( ok_cancel_view ) );
  auto view = make_unique<ui::VerticalArrayView>(
      std::move( view_vec ),
      ui::VerticalArrayView::align::center );
  return async_window_builder( window_plane, title,
                               std::move( view ),
                               /*auto_pad=*/true );
}

using GetOkBoxSubjectViewFunc = unique_ptr<ui::View>(
    function<void( bool )> /*enable_ok_button*/ //
);

template<typename ResultT>
[[nodiscard]] unique_ptr<Window> ok_box_window_builder(
    WindowPlane& window_plane, string_view title,
    function<ResultT()>              get_result,
    function<bool( ResultT const& )> validator,
    // on_result must be copyable.
    function<void( ResultT )>             on_result,
    function_ref<GetOkBoxSubjectViewFunc> get_view_fn ) {
  auto ok_button_view = make_unique<ui::OkButtonView>(
      /*on_ok=*/
      [on_result, validator{ std::move( validator ) },
       get_result{ std::move( get_result ) }] {
        lg.trace( "selected ok." );
        auto const& proposed = get_result();
        if( validator( proposed ) ) {
          on_result( proposed );
        } else {
          lg.debug( "{} is invalid.", proposed );
        }
      } );
  auto* p_ok_button      = ok_button_view->ok_button();
  auto  enable_ok_button = [p_ok_button]( bool enable ) {
    p_ok_button->enable( enable );
  };
  auto subject_view = get_view_fn(
      /*enable_ok_button=*/std::move( enable_ok_button ) //
  );
  vector<unique_ptr<ui::View>> view_vec;
  view_vec.emplace_back( std::move( subject_view ) );
  view_vec.emplace_back( std::move( ok_button_view ) );
  auto view = make_unique<ui::VerticalArrayView>(
      std::move( view_vec ),
      ui::VerticalArrayView::align::center );
  return async_window_builder( window_plane, title,
                               std::move( view ),
                               /*auto_pad=*/true );
}

[[nodiscard]] unique_ptr<Window> ok_cancel_impl(
    WindowPlane& window_plane, string_view msg,
    function<void( ui::e_ok_cancel )> on_result ) {
  auto on_ok_cancel_result =
      [on_result{ std::move( on_result ) }]( maybe<int> o ) {
        if( o.has_value() )
          return on_result( ui::e_ok_cancel::ok );
        on_result( ui::e_ok_cancel::cancel );
      };
  TextMarkupInfo m_info{
      /*normal=*/config_ui.dialog_text.normal,
      /*highlight=*/config_ui.dialog_text.highlighted };
  TextReflowInfo r_info{
      /*max_cols=*/config_ui.dialog_text.columns };
  auto view =
      make_unique<ui::TextView>( string( msg ), m_info, r_info );

  // We can capture by reference here because the function will
  // be called before this scope exits.
  auto get_view_fn =
      [&]( function<void( bool )> /*enable_ok_button*/ ) {
        return std::move( view );
      };

  // Use <int> for lack of anything better.
  return ok_cancel_window_builder<int>(
      window_plane,
      /*title=*/"Question",
      /*get_result=*/L0( 0 ),
      /*validator=*/L( _ == 0 ), // always true.
      /*on_result=*/std::move( on_ok_cancel_result ),
      /*get_view_fn=*/get_view_fn //
  );
}

wait<ui::e_ok_cancel> ok_cancel( WindowPlane&     window_plane,
                                 std::string_view msg ) {
  wait_promise<ui::e_ok_cancel> p;
  unique_ptr<Window>            win = ok_cancel_impl(
                 window_plane, msg,
                 [p]( ui::e_ok_cancel oc ) { p.set_value( oc ); } );
  co_return co_await p.wait();
}

namespace {
[[nodiscard]] unique_ptr<Window> text_input_box(
    WindowPlane& window_plane, string_view title,
    string_view msg, string_view initial_text,
    ui::ValidatorFunc               validator,
    function<void( maybe<string> )> on_result ) {
  TextMarkupInfo m_info{
      /*normal=*/config_ui.dialog_text.normal,
      /*highlight=*/config_ui.dialog_text.highlighted };
  TextReflowInfo r_info{
      /*max_cols=*/config_ui.dialog_text.columns };
  auto text =
      make_unique<ui::TextView>( string( msg ), m_info, r_info );
  auto le_view = make_unique<ui::LineEditorView>(
      /*chars_wide=*/20, initial_text );
  ui::LineEditorView* p_le_view = le_view.get();

  // We can capture by reference here because the function will
  // be called before this scope exits.
  auto get_view_fn =
      [&]( function<void( bool )> enable_ok_button ) {
        ui::LineEditorView::OnChangeFunc on_change =
            [validator,
             enable_ok_button{ std::move( enable_ok_button ) }](
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
        vector<unique_ptr<ui::View>> view_vec;
        view_vec.emplace_back( std::move( text ) );
        view_vec.emplace_back( std::move( le_view ) );
        return make_unique<ui::VerticalArrayView>(
            std::move( view_vec ),
            ui::VerticalArrayView::align::center );
      };

  return ok_cancel_window_builder<string>(
      window_plane,
      /*title=*/title,
      /*get_result=*/
      [p_le_view]() -> string { return p_le_view->text(); },
      /*validator=*/validator, // must be copied.
      /*on_result=*/std::move( on_result ),
      /*get_view_fun=*/get_view_fn //
  );
}
} // namespace

/****************************************************************
** High-level Methods
*****************************************************************/
wait<vector<UnitSelection>> unit_selection_box(
    WindowPlane& window_plane, vector<UnitId> const& ids,
    bool allow_activation ) {
  wait_promise<vector<UnitSelection>> s_promise;

  function<void( maybe<UnitActivationView::map_t> )> on_result =
      [s_promise]( maybe<UnitActivationView::map_t> result ) {
        vector<UnitSelection> selections;
        if( result.has_value() ) {
          for( auto const& [id, info] : *result ) {
            if( info.is_activated ) {
              CHECK( info.current_orders ==
                     e_unit_orders::none );
              selections.push_back(
                  { id, e_unit_selection::activate } );
            } else if( info.current_orders !=
                       info.original_orders ) {
              CHECK( info.current_orders ==
                     e_unit_orders::none );
              selections.push_back(
                  { id, e_unit_selection::clear_orders } );
            }
          }
        }
        for( auto selection : selections )
          lg.debug(
              "selection: {} --> {}",
              debug_string( GameState::units(), selection.id ),
              // FIXME: until we can format this enum.
              static_cast<int>( selection.what ) );
        s_promise.set_value( std::move( selections ) );
      };

  auto unit_activation_view =
      UnitActivationView::Create( ids, allow_activation );
  auto* p_unit_activation_view = unit_activation_view.get();

  // We can capture by reference here because the function will
  // be called before this scope exits.
  auto get_view_fn =
      [&]( function<void( bool )> /*enable_ok_button*/ ) {
        return std::move( unit_activation_view );
      };

  unique_ptr<Window> win = ok_cancel_window_builder<
      unordered_map<UnitId, UnitActivationInfo>>(
      window_plane,
      /*title=*/"Activate Units",
      /*get_result=*/
      [p_unit_activation_view]() {
        return p_unit_activation_view->info_map();
      },
      /*validator=*/L( ( (void)_, true ) ), // always true.
      /*on_result=*/std::move( on_result ),
      /*get_view_fun=*/get_view_fn //
  );

  co_return co_await s_promise.wait();
}

/****************************************************************
** WindowPlane
*****************************************************************/
Plane& WindowPlane::impl() { return *impl_; }

WindowPlane::~WindowPlane() = default;

WindowPlane::WindowPlane() : impl_( new Impl() ) {}

wait<> WindowPlane::message_box( string_view msg ) {
  wait_promise<>     p;
  unique_ptr<Window> win = async_window_builder(
      *this, /*title=*/"note",
      ui::PlainMessageBoxView::create( string( msg ), p ),
      /*auto_pad=*/true );
  co_await p.wait();
}

wait<int> WindowPlane::select_box(
    string_view msg, vector<string> const& options ) {
  lg.info( "question: \"{}\"", msg );
  auto selector_view = make_unique<ui::OptionSelectView>(
      options, /*initial_selection=*/0 );
  auto* p_selector_view = selector_view.get();

  wait_promise<int> p;

  auto on_input = [&]( input::event_t const& event ) {
    bool selected = false;
    switch( event.to_enum() ) {
      case input::e_input_event::key_event: {
        auto const& key_event = event.as<input::key_event_t>();
        if( key_event.keycode != ::SDLK_RETURN &&
            key_event.keycode != ::SDLK_KP_ENTER &&
            key_event.keycode != ::SDLK_SPACE &&
            key_event.keycode != ::SDLK_KP_5 )
          return false; // not handled.
        if( key_event.change != input::e_key_change::down )
          return true;
        // An enter-like key is being released, so take action.
        selected = true;
        break;
      }
      case input::e_input_event::mouse_button_event: {
        auto const& button_event =
            event.as<input::mouse_button_event_t>();
        if( button_event.buttons !=
            input::e_mouse_button_event::left_up )
          break;
        selected = true;
        break;
      }
      default: break;
    }
    if( selected ) {
      int result = p_selector_view->get_selected();
      CHECK( result >= 0 && result < int( options.size() ) );
      lg.info( "selected: {}", options[result] );
      p.set_value( result );
    }
    bool handled = selected;
    return handled;
  };

  auto on_input_view = make_unique<ui::OnInputView>(
      std::move( selector_view ), std::move( on_input ) );

  unique_ptr<ui::View> view = std::move( on_input_view );
  unique_ptr<Window>   win  = async_window_builder(
         *this, msg, std::move( view ), /*auto_pad=*/false );

  p_selector_view->grow_to( win->inside_padding_rect().w );

  // Need to co_await instead of returning so that the window
  // stays alive while we wait.
  co_return co_await p.wait();
}

wait<maybe<string>> WindowPlane::str_input_box(
    string_view title, string_view msg,
    string_view initial_text ) {
  wait_promise<maybe<string>> p;
  unique_ptr<Window>          win = text_input_box(
               *this, title, msg, initial_text, L( _.size() > 0 ),
               [p]( maybe<string> result ) { p.set_value( result ); } );
  co_return co_await p.wait();
}

wait<maybe<int>> WindowPlane::int_input_box(
    IntInputBoxOptions const& options ) {
  wait_promise<maybe<int>> p;

  string initial_text =
      options.initial.has_value()
          ? fmt::format( "{}", *options.initial )
          : "";
  unique_ptr<Window> win = text_input_box(
      *this, options.title, options.msg, initial_text,
      make_int_validator( options.min, options.max ),
      [p]( maybe<string> result ) {
        p.set_value( result.bind( L( base::stoi( _ ) ) ) );
      } );
  co_return co_await p.wait();
}

} // namespace rn
