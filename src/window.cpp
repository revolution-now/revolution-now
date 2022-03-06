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
#include "config-files.hpp"
#include "error.hpp"
#include "game-ui-views.hpp"
#include "gfx.hpp"
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

// Revolution Now (config)
#include "../config/rcl/palette.inl"
#include "../config/rcl/ui.inl"

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

namespace rn::ui {

namespace rl = ::base::rl;

namespace {

using ::base::function_ref;

/****************************************************************
** Window
*****************************************************************/
struct Window {
  Window( std::string title_, Coord position_ );
  // Removes this window from the window manager.
  ~Window() noexcept;

  Window( Window&& ) = default;
  Window& operator=( Window&& ) = default;

  void set_view( unique_ptr<View> view_ ) {
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

  void  draw( Texture& tx ) const;
  Delta delta() const;
  Rect  rect() const;
  Coord inside_border() const;
  Rect  inside_border_rect() const;
  Coord inside_padding() const;
  // Rect  inside_padding_rect() const;
  Rect title_bar() const;
  // abs coord of upper-left corner of view.
  Coord view_pos() const;

  std::string                        title;
  std::unique_ptr<View>              view;
  std::unique_ptr<OneLineStringView> title_view;
  Coord                              position;
};
NOTHROW_MOVE( Window );

/****************************************************************
** WindowManager
*****************************************************************/
class WindowManager {
 public:
  void draw_layout( Texture& tx ) const;

  ND Plane::e_input_handled input( input::event_t const& event );

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

} // namespace
} // namespace rn::ui

namespace rn {

namespace {

/****************************************************************
** The Window Plane
*****************************************************************/
struct WindowPlane : public Plane {
  WindowPlane() = default;
  bool covers_screen() const override { return false; }
  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    wm.draw_layout( tx );
  }
  e_input_handled input( input::event_t const& event ) override {
    // Windows are modal, so ignore the result of this function
    // because we need to swallow the event whether the window
    // manager handled it or not. I.e., we cannot ever let an
    // event go down to a lower plane.
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
Window::Window( string title_, Coord position_ )
  : title( move( title_ ) ),
    view{},
    title_view(),
    position( position_ ) {
  title_view = make_unique<OneLineStringView>(
      title, config_palette.orange.sat1.lum11, /*shadow=*/true );
  g_window_plane.wm.add_window( this );
}

Window::~Window() noexcept {
  g_window_plane.wm.remove_window( this );
}

void Window::draw( Texture& tx ) const {
  CHECK( view );
  auto win_size = delta();
  render_fill_rect(
      tx, gfx::pixel{ 0, 0, 0, 64 },
      Rect::from( position + Delta{ 4_w, 4_h }, win_size ) );
  render_fill_rect(
      tx, config_palette.orange.sat0.lum1,
      { position.x, position.y, win_size.w, win_size.h } );
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

// Rect Window::inside_padding_rect() const {
//  auto res = rect();
//  res.x += window_border().w;
//  res.y += window_border().h;
//  res.w -= window_border().w * 2_sx;
//  res.h -= window_border().h * 2_sy;
//  res.x += window_padding().w;
//  res.y += window_padding().h;
//  res.w -= window_padding().w * 2_sx;
//  res.h -= window_padding().h * 2_sy;
//  return res;
//}

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
  for( Window* window : active_windows() ) window->draw( tx );
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

Plane::e_input_handled WindowManager::input(
    input::event_t const& event ) {
  if( this->num_windows() == 0 )
    return Plane::e_input_handled::no;

  maybe<input::mouse_event_base_t const&> mouse_event =
      input::is_mouse_event( event );
  if( !mouse_event ) {
    // It's a non-mouse event, so just send it to the top-most
    // window and return if it was handled.
    return focused().view->input( event )
               ? Plane::e_input_handled::yes
               : Plane::e_input_handled::no;
  }

  // It's a mouse event.
  maybe<Window&> win = window_for_cursor_pos( mouse_event->pos );
  if( !win )
    // Only send mouse events when the cursor is over a window.
    return Plane::e_input_handled::no;
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
    // Always return that we handled the mouse event if we are
    // inside a view.
    return Plane::e_input_handled::yes;
  }
  return Plane::e_input_handled::no;
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

ValidatorFunc make_int_validator( maybe<int> min,
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
    std::string_view title, unique_ptr<View> view ) {
  auto win = make_unique<Window>( string( title ), Coord{} );
  autopad( view, /*use_fancy=*/false );
  win->set_view( std::move( view ) );
  win->center_window();
  return win;
}

using GetOkCancelSubjectViewFunc = unique_ptr<View>(
    function<void( bool )> /*enable_ok_button*/ //
);

template<typename ResultT>
[[nodiscard]] unique_ptr<Window> ok_cancel_window_builder(
    string_view title, function<ResultT()> get_result,
    function<bool( ResultT const& )> validator,
    // on_result must be copyable.
    function<void( maybe<ResultT> )>         on_result,
    function_ref<GetOkCancelSubjectViewFunc> get_view_fn ) {
  auto ok_cancel_view = make_unique<OkCancelView>(
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
  unique_ptr<View> subject_view = get_view_fn(
      /*enable_ok_button=*/std::move( enable_ok_button ) //
  );
  CHECK( subject_view != nullptr );
  vector<unique_ptr<View>> view_vec;
  view_vec.emplace_back( std::move( subject_view ) );
  view_vec.emplace_back( std::move( ok_cancel_view ) );
  auto view = make_unique<VerticalArrayView>(
      std::move( view_vec ), VerticalArrayView::align::center );
  return async_window_builder( title, std::move( view ) );
}

using GetOkBoxSubjectViewFunc = unique_ptr<View>(
    function<void( bool )> /*enable_ok_button*/ //
);

template<typename ResultT>
[[nodiscard]] unique_ptr<Window> ok_box_window_builder(
    string_view title, function<ResultT()> get_result,
    function<bool( ResultT const& )> validator,
    // on_result must be copyable.
    function<void( ResultT )>             on_result,
    function_ref<GetOkBoxSubjectViewFunc> get_view_fn ) {
  auto ok_button_view = make_unique<OkButtonView>(
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
  vector<unique_ptr<View>> view_vec;
  view_vec.emplace_back( std::move( subject_view ) );
  view_vec.emplace_back( std::move( ok_button_view ) );
  auto view = make_unique<VerticalArrayView>(
      std::move( view_vec ), VerticalArrayView::align::center );
  return async_window_builder( title, std::move( view ) );
}

[[nodiscard]] unique_ptr<Window> ok_cancel_impl(
    string_view msg, function<void( e_ok_cancel )> on_result ) {
  auto on_ok_cancel_result =
      [on_result{ std::move( on_result ) }]( maybe<int> o ) {
        if( o.has_value() ) return on_result( e_ok_cancel::ok );
        on_result( e_ok_cancel::cancel );
      };
  TextMarkupInfo m_info{
      /*normal=*/config_ui.dialog_text.normal,
      /*highlight=*/config_ui.dialog_text.highlighted };
  TextReflowInfo r_info{
      /*max_cols=*/config_ui.dialog_text.columns };
  auto view =
      make_unique<TextView>( string( msg ), m_info, r_info );

  // We can capture by reference here because the function will
  // be called before this scope exits.
  auto get_view_fn =
      [&]( function<void( bool )> /*enable_ok_button*/ ) {
        return std::move( view );
      };

  // Use <int> for lack of anything better.
  return ok_cancel_window_builder<int>(
      /*title=*/"Question",
      /*get_result=*/L0( 0 ),
      /*validator=*/L( _ == 0 ), // always true.
      /*on_result=*/std::move( on_ok_cancel_result ),
      /*get_view_fn=*/get_view_fn //
  );
}

wait<e_ok_cancel> ok_cancel( std::string_view msg ) {
  wait_promise<e_ok_cancel> p;
  unique_ptr<Window>        win = ok_cancel_impl(
             msg, [p]( e_ok_cancel oc ) { p.set_value( oc ); } );
  co_return co_await p.wait();
}

namespace {
[[nodiscard]] unique_ptr<Window> text_input_box(
    string_view title, string_view msg, string_view initial_text,
    ValidatorFunc                   validator,
    function<void( maybe<string> )> on_result ) {
  TextMarkupInfo m_info{
      /*normal=*/config_ui.dialog_text.normal,
      /*highlight=*/config_ui.dialog_text.highlighted };
  TextReflowInfo r_info{
      /*max_cols=*/config_ui.dialog_text.columns };
  auto text =
      make_unique<TextView>( string( msg ), m_info, r_info );
  auto le_view = make_unique<LineEditorView>( /*chars_wide=*/20,
                                              initial_text );
  LineEditorView* p_le_view = le_view.get();

  // We can capture by reference here because the function will
  // be called before this scope exits.
  auto get_view_fn =
      [&]( function<void( bool )> enable_ok_button ) {
        LineEditorView::OnChangeFunc on_change =
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
        vector<unique_ptr<View>> view_vec;
        view_vec.emplace_back( std::move( text ) );
        view_vec.emplace_back( std::move( le_view ) );
        return make_unique<VerticalArrayView>(
            std::move( view_vec ),
            VerticalArrayView::align::center );
      };

  return ok_cancel_window_builder<string>(
      /*title=*/title,
      /*get_result=*/
      [p_le_view]() -> string { return p_le_view->text(); },
      /*validator=*/validator, // must be copied.
      /*on_result=*/std::move( on_result ),
      /*get_view_fun=*/get_view_fn //
  );
}
} // namespace

wait<maybe<int>> int_input_box(
    IntInputBoxOptions const& options ) {
  wait_promise<maybe<int>> p;

  string initial_text =
      options.initial.has_value()
          ? fmt::format( "{}", *options.initial )
          : "";
  unique_ptr<Window> win = text_input_box(
      options.title, options.msg, initial_text,
      make_int_validator( options.min, options.max ),
      [p]( maybe<string> result ) {
        p.set_value( result.bind( L( base::stoi( _ ) ) ) );
      } );
  co_return co_await p.wait();
}

wait<maybe<string>> str_input_box( string_view title,
                                   string_view msg,
                                   string_view initial_text ) {
  wait_promise<maybe<string>> p;
  unique_ptr<Window>          win = text_input_box(
               title, msg, initial_text, L( _.size() > 0 ),
               [p]( maybe<string> result ) { p.set_value( result ); } );
  co_return co_await p.wait();
}

/****************************************************************
** High-level Methods
*****************************************************************/
wait<string> select_box( string_view    title,
                         vector<string> options ) {
  lg.info( "question: \"{}\"", title );
  auto view = make_unique<OptionSelectView>(
      options, /*initial_selection=*/0 );
  auto* p_selector = view.get();

  // p_selector->grow_to( win->inside_padding_rect().w );

  wait_promise<string> p;

  auto on_result_prime = [=]( string const& result ) {
    lg.info( "selected: {}", result );
    p.set_value( result );
  };

  // We can capture by reference here because the function will
  // be called before this scope exits.
  auto get_view_fn =
      [&]( function<void( bool )> /*enable_ok_button*/ ) {
        return std::move( view );
      };

  unique_ptr<Window> win = ok_box_window_builder<string>(
      /*title=*/title,
      /*get_result=*/
      [p_selector] { return p_selector->get_selected(); },
      /*validator=*/
      []( auto const& ) { return true; }, // always
                                          // true.
      /*on_result=*/std::move( on_result_prime ),
      /*get_view_fn=*/get_view_fn //
  );

  // Need to co_await instead of returning so that the window
  // stays alive while we wait.
  co_return co_await p.wait();
}

wait<e_confirm> yes_no( std::string_view title ) {
  return select_box_enum<e_confirm>( title );
}

wait<> message_box_basic( string_view msg ) {
  wait_promise<>     p;
  unique_ptr<Window> win = async_window_builder(
      /*title=*/"note",
      PlainMessageBoxView::create( string( msg ), p ) );
  co_await p.wait();
}

wait<vector<UnitSelection>> unit_selection_box(
    vector<UnitId> const& ids, bool allow_activation ) {
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
          lg.debug( "selection: {} --> {}",
                    debug_string( selection.id ),
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
** Testing Only
*****************************************************************/
void window_test() {}

} // namespace rn::ui
