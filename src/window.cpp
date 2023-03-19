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
#include "co-combinator.hpp"
#include "co-time.hpp"
#include "co-wait.hpp"
#include "compositor.hpp"
#include "error.hpp"
#include "game-ui-views.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ui.hpp"
#include "unit-mgr.hpp"
#include "unit.hpp"
#include "util.hpp"
#include "views.hpp"

// config
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// ss
#include "ss/ref.hpp"

// render
#include "render/renderer.hpp"

// base
#include "base/conv.hpp"
#include "base/function-ref.hpp"
#include "base/lambda.hpp"
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"

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
struct WindowManager {
 public:
  struct PositionedWindow {
    Window* win = nullptr;
    Coord   pos = {};
  };

  void draw_layout( rr::Renderer& renderer ) const;

  void advance_state() {
    for( PositionedWindow const& pw : windows_ )
      pw.win->advance_state();
  }

  ND e_input_handled input( input::event_t const& event );

  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord                 origin );
  void                 on_drag( input::mod_keys const& mod,
                                input::e_mouse_button button, Coord origin,
                                Coord prev, Coord current );

  vector<PositionedWindow> const& active_windows() const {
    return windows_;
  }

  int num_windows() const { return windows_.size(); }

  void center_window( Window const& win ) {
    UNWRAP_CHECK( area, compositor::section(
                            compositor::e_section::normal ) );
    find_window( win ).pos = centered( win.delta(), area );
  }

  void add_window( Window& window ) {
    ++num_windows_created_;
    windows_.push_back( { .win = &window, .pos = Coord{} } );
  }

  void set_position( Window const& window, Coord pos ) {
    find_window( window ).pos = pos;
  }

  Coord position( Window const& window ) {
    return find_window( window ).pos;
  }

  Coord position( Window const& window ) const {
    return find_window( window ).pos;
  }

  PositionedWindow& find_window( Window const& window ) {
    for( PositionedWindow& pw : windows_ )
      if( pw.win == &window ) return pw;
    FATAL( "window not found." );
  }

  PositionedWindow const& find_window(
      Window const& window ) const {
    for( PositionedWindow const& pw : windows_ )
      if( pw.win == &window ) return pw;
    FATAL( "window not found." );
  }

  void remove_window( Window const& win ) {
    erase_if( windows_, LC( _.win == &win ) );
    if( dragging_win_ == &win ) dragging_win_ = nullptr;
  }

  int num_windows_created() const {
    return num_windows_created_;
  }

  int num_windows_currently_open() const {
    return windows_.size();
  }

  auto& transient_messages() { return transient_messages_; }

 private:
  wait<> process_transient_messages();

  // Gets the window with focus, throws if no windows.
  Window& focused();

  // Get the top-most window under the cursor position.
  maybe<Window&> window_for_cursor_pos( Coord const& pos );
  // Same as above but only when cursor is in a window's view.
  maybe<Window&> window_for_cursor_pos_in_view(
      Coord const& pos );

  vector<PositionedWindow> windows_;
  // The value of this is only relevant during a drag.
  Window* dragging_win_ = nullptr;

  // Each time a new window is created this is incremented, and
  // it is never decremented.
  int num_windows_created_ = 0;

  struct TransientMessage {
    string         msg;
    double         alpha       = 1.0;
    TextReflowInfo reflow_info = {};
    // This is stored here because it is expensive to compute; we
    // don't want to do it every frame.
    Delta rendered_size = {};
  };

  co::stream<string>      transient_messages_ = {};
  maybe<TransientMessage> active_transient_message_;

  // This is a background coroutine that runs as long as this ob-
  // ject is alive and displays the transient windows.
  wait<> transient_message_processor_ =
      process_transient_messages();
};

/****************************************************************
** WindowPlane::Impl
*****************************************************************/
struct WindowPlane::Impl : public Plane {
  Impl() = default;

  bool covers_screen() const override { return false; }

  void advance_state() override { wm.advance_state(); }

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

  int num_windows_created() const {
    return wm.num_windows_created();
  }

  int num_windows_currently_open() const {
    return wm.num_windows_currently_open();
  }

  WindowManager wm;
};

/****************************************************************
** Window
*****************************************************************/
void Window::set_view( unique_ptr<ui::View> view ) {
  view_ = std::move( view );
}

void Window::advance_state() {
  CHECK( view_ != nullptr );
  view_->advance_state();
}

void Window::center_me() const {
  window_manager_.center_window( *this );
}

void Window::autopad_me() {
  autopad( *view_, /*use_fancy=*/false );
}

bool Window::operator==( Window const& rhs ) const {
  return this == &rhs;
}

Window::Window( WindowManager& window_manager )
  : window_manager_( window_manager ), view_{} {
  window_manager_.add_window( *this );
}

Window::~Window() noexcept {
  window_manager_.remove_window( *this );
}

void Window::draw( rr::Renderer& renderer, Coord where ) const {
  CHECK( view_ );
  {
    // FIXME: remove this once the translation is done automati-
    // cally.
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.repos.translation,
        gfx::size( where.distance_from_origin() ).to_double() );
    rr::Painter painter = renderer.painter();
    Rect        r       = rect( Coord{} );
    // Render shadow behind window.
    painter.draw_solid_rect( r + Delta{ .w = 4, .h = 4 },
                             gfx::pixel{ 0, 0, 0, 64 } );
    tile_sprite( painter, e_tile::wood_middle, rect( Coord{} ) );
    // Render window border, highlights on top and right.
    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .75 );
      render_shadow_hightlight_border(
          renderer, r.edges_removed( 2 ),
          config_ui.window.border_dark,
          config_ui.window.border_lighter );
      render_shadow_hightlight_border(
          renderer, r.edges_removed( 1 ),
          config_ui.window.border_darker,
          config_ui.window.border_light );
    }
  }

  view_->draw( renderer, view_pos( where ) );
}

// Includes border
Delta Window::delta() const {
  CHECK( view_ );
  Delta res;
  res.w += view_->delta().w;
  res.h += view_->delta().h;
  // Padding inside window border.
  res.w += config_ui.window.window_padding * 2;
  res.h += config_ui.window.window_padding * 2;
  // multiply by two since there is top/bottom or left/right.
  res += window_border() * 2;
  return res;
}

Rect Window::rect( Coord origin ) const {
  return Rect::from( origin, delta() );
}

Rect Window::inside_border_rect( Coord origin ) const {
  return Rect::from( origin + window_border(),
                     delta() - window_border() * 2 );
}

Coord Window::inside_padding( Coord origin ) const {
  return origin + window_border() + window_padding();
}

Coord Window::view_pos( Coord origin ) const {
  return inside_padding( origin );
}

/****************************************************************
** WindowManager
*****************************************************************/
void WindowManager::draw_layout( rr::Renderer& renderer ) const {
  for( PositionedWindow const& pw : active_windows() )
    pw.win->draw( renderer, pw.pos );

  if( active_transient_message_.has_value() ) {
    string_view const msg = active_transient_message_->msg;
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha,
                             active_transient_message_->alpha );
    UNWRAP_CHECK(
        area, compositor::section(
                  compositor::e_section::viewport_and_panel ) );
    Delta const rendered_size =
        active_transient_message_->rendered_size;
    Coord const start = centered( rendered_size, area )
                            .with_y( area.top_edge() ) +
                        Delta{ .h = 5 };
    Rect const background = Rect::from( start, rendered_size )
                                .with_border_added( 2 );
    rr::Painter painter = renderer.painter();
    painter.draw_solid_rect( background, gfx::pixel::wood() );
    painter.draw_empty_rect( background,
                             rr::Painter::e_border_mode::outside,
                             gfx::pixel::black() );
    TextMarkupInfo const markup_info{
        .shadow = gfx::pixel::black(),
    };
    TextReflowInfo const& reflow_info =
        active_transient_message_->reflow_info;
    render_text_markup_reflow( renderer, start, e_font::habbo_15,
                               markup_info, reflow_info, msg );
  }
}

maybe<Window&> WindowManager::window_for_cursor_pos(
    Coord const& cursor ) {
  for( PositionedWindow const& pw :
       rl::rall( active_windows() ) )
    if( cursor.is_inside( pw.win->rect( pw.pos ) ) ) //
      return *pw.win;
  return nothing;
}

maybe<Window&> WindowManager::window_for_cursor_pos_in_view(
    Coord const& cursor ) {
  for( PositionedWindow const& pw :
       rl::rall( active_windows() ) ) {
    auto view_rect = Rect::from( pw.win->view_pos( pw.pos ),
                                 pw.win->view()->delta() );
    if( cursor.is_inside( view_rect ) ) //
      return *pw.win;
  }
  return nothing;
}

e_input_handled WindowManager::input(
    input::event_t const& event ) {
  // Since windows are modal we will always declare that we've
  // handled the event, unless there are no windows open.
  if( this->num_windows() == 0 ) return e_input_handled::no;

  CHECK(
      !focused().view()->disabled(),
      "a top-level view should not be in the disabled state." );

  maybe<input::mouse_event_base_t const&> mouse_event =
      input::is_mouse_event( event );
  if( !mouse_event ) {
    // It's a non-mouse event, so just send it to the top-most
    // window and declare it to be handled.
    (void)focused().view()->input( event );
    return e_input_handled::yes;
  }

  // It's a mouse event.
  maybe<Window&> win = window_for_cursor_pos( mouse_event->pos );
  if( !win ) {
    auto button_event =
        event.get_if<input::mouse_button_event_t>();
    if( !button_event.has_value() ) return e_input_handled::yes;
    if( button_event->buttons !=
        input::e_mouse_button_event::left_up )
      return e_input_handled::yes;
    // We have a window open and the user has clicked the mouse,
    // but the user has clicked outside of the window. In order
    // to reproduce the behavior of the original game (which al-
    // lows closing/cancelling popup boxes by clicking outside of
    // the window in many cases) we will forward this to the
    // window as an escape key, and the window can decide what to
    // do with it; many windows will cancel/close the window in
    // response.
    input::key_event_t escape_event;
    escape_event.change    = input::e_key_change::down;
    escape_event.keycode   = ::SDLK_ESCAPE;
    escape_event.scancode  = ::SDL_SCANCODE_ESCAPE;
    escape_event.direction = nothing;
    (void)focused().view()->input( escape_event );

    return e_input_handled::yes;
  }
  auto view_rect = Rect::from( win->view_pos( position( *win ) ),
                               win->view()->delta() );

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
        old_view->view()->on_mouse_leave(
            old_pos.with_new_origin( view_rect.upper_left() ) );
      if( new_view )
        new_view->view()->on_mouse_enter(
            new_pos.with_new_origin( view_rect.upper_left() ) );
    }
  }

  // Only send the event if the mouse position is within the
  // view. And, when we send it, we make the mouse position rela-
  // tive to the upper left corner of the view.
  if( mouse_event->pos.is_inside( view_rect ) ) {
    auto new_event = input::move_mouse_origin_by(
        event, win->view_pos( position( *win ) ) - Coord{} );
    (void)win->view()->input( new_event );
  }
  // Always handle the event if there is a window open.
  return e_input_handled::yes;
}

Plane::e_accept_drag WindowManager::can_drag(
    input::e_mouse_button /*button*/, Coord origin ) {
  if( num_windows() == 0 ) return Plane::e_accept_drag::no;
  maybe<Window&> win = window_for_cursor_pos( origin );
  // If it's not in a window then swallow it to prevent any other
  // plane from handling it.
  if( !win ) return Plane::e_accept_drag::swallow;
  dragging_win_ = &*win;
  // Allow dragging from anywhere in the window.
  return Plane::e_accept_drag::yes;
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
    Coord pos = position( *dragging_win_ );
    pos += ( current - prev );
    // Now prevent the window from being dragged off screen.
    Rect const total_area = main_window_logical_rect();
    pos.y = clamp( pos.y, 0, total_area.bottom_edge() - 16 );
    pos.x = clamp( pos.x, 16 - focused().delta().w,
                   total_area.right_edge() - 16 );
    set_position( *dragging_win_, pos );
  }
}

Window& WindowManager::focused() {
  CHECK( num_windows() > 0 );
  return *active_windows().back().win;
}

wait<> WindowManager::process_transient_messages() {
  while( true ) {
    string msg = co_await transient_messages_.next();
    TextReflowInfo const reflow_info{ .max_cols = 50 };
    active_transient_message_ = {
        .msg         = msg,
        .alpha       = 1.0,
        .reflow_info = reflow_info,
        .rendered_size =
            rendered_text_size( reflow_info, msg ) };
    SCOPE_EXIT( active_transient_message_.reset() );
    while( active_transient_message_->alpha > 0 ) {
      co_await 100ms;
      active_transient_message_->alpha -= .05;
    }
  }
}

/****************************************************************
** Validators
*****************************************************************/
// These should probably be moved elsewhere.
namespace {

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

}

/****************************************************************
** Windows
*****************************************************************/
// We need to have pointer stability on the returned window since
// its address needs to go into callbacks.
namespace {
[[nodiscard]] unique_ptr<Window> async_window_builder(
    WindowManager& window_manager, unique_ptr<ui::View> view,
    bool auto_pad ) {
  auto win = make_unique<Window>( window_manager );
  if( auto_pad ) autopad( *view, /*use_fancy=*/false );
  win->set_view( std::move( view ) );
  window_manager.center_window( *win );
  return win;
}
} // namespace

using GetOkCancelSubjectViewFunc = unique_ptr<ui::View>(
    function<void( bool )> /*enable_ok_button*/ //
);

template<typename ResultT>
[[nodiscard]] unique_ptr<Window> ok_cancel_window_builder(
    WindowManager&                   window_manager,
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
  return async_window_builder( window_manager, std::move( view ),
                               /*auto_pad=*/true );
}

using GetOkBoxSubjectViewFunc = unique_ptr<ui::View>(
    function<void( bool )> /*enable_ok_button*/ //
);

template<typename ResultT>
[[nodiscard]] unique_ptr<Window> ok_box_window_builder(
    WindowManager&                   window_manager,
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
  return async_window_builder( window_manager, std::move( view ),
                               /*auto_pad=*/true );
}

namespace {

[[nodiscard]] unique_ptr<Window> text_input_box(
    WindowManager& window_manager, string_view msg,
    string_view initial_text, e_input_required required,
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

  vector<unique_ptr<ui::View>> view_vec;
  view_vec.emplace_back( std::move( text ) );
  view_vec.emplace_back( std::move( le_view ) );
  auto va_view = make_unique<ui::VerticalArrayView>(
      std::move( view_vec ),
      ui::VerticalArrayView::align::center );

  auto on_input = [=]( input::event_t const& event ) {
    switch( event.to_enum() ) {
      case input::e_input_event::key_event: {
        auto const& key_event = event.as<input::key_event_t>();
        if( key_event.change != input::e_key_change::down )
          return true; // handled.
        if( required == e_input_required::no &&
            key_event.keycode == ::SDLK_ESCAPE ) {
          lg.info( "cancelled." );
          on_result( nothing );
          return true; // handled.
        }
        if( key_event.keycode == ::SDLK_RETURN ||
            key_event.keycode == ::SDLK_KP_ENTER ) {
          if( !validator( p_le_view->text() ) ) {
            // Invalid input.
            return true; // handled.
          }
          lg.info( "received: {}", p_le_view->text() );
          on_result( p_le_view->text() );
          return true; // handled.
        }
        return false;  // not handled.
      }
      default:
        break;
    }
    return false; // not handled.
  };

  auto view = make_unique<ui::OnInputView>(
      std::move( va_view ), std::move( on_input ) );

  return async_window_builder( window_manager, std::move( view ),
                               /*auto_pad=*/true );
}

} // namespace

/****************************************************************
** High-level Methods
*****************************************************************/
wait<vector<UnitSelection>> unit_selection_box(
    SSConst const& ss, WindowPlane& window_plane,
    vector<UnitId> const& ids ) {
  wait_promise<vector<UnitSelection>> s_promise;

  function<void( maybe<UnitActivationView::map_t> )> on_result =
      [&s_promise]( maybe<UnitActivationView::map_t> result ) {
        vector<UnitSelection> selections;
        if( result.has_value() ) {
          for( auto const& [id, info] : *result ) {
            if( info.is_activated ) {
              CHECK( info.current_orders.to_enum() ==
                     unit_orders::e::none );
              selections.push_back(
                  { id, e_unit_selection::activate } );
            } else if( info.current_orders !=
                       info.original_orders ) {
              CHECK( info.current_orders.to_enum() ==
                     unit_orders::e::none );
              selections.push_back(
                  { id, e_unit_selection::clear_orders } );
            }
          }
        }
        s_promise.set_value( std::move( selections ) );
      };

  auto unit_activation_view =
      UnitActivationView::Create( ss, ids );
  auto* p_unit_activation_view = unit_activation_view.get();

  // We can capture by reference here because the function will
  // be called before this scope exits.
  auto get_view_fn =
      [&]( function<void( bool )> /*enable_ok_button*/ ) {
        return std::move( unit_activation_view );
      };

  unique_ptr<Window> win = ok_cancel_window_builder<
      unordered_map<UnitId, UnitActivationInfo>>(
      window_plane.manager(),
      /*get_result=*/
      [p_unit_activation_view]() {
        return p_unit_activation_view->info_map();
      },
      /*validator=*/L( ( (void)_, true ) ), // always true.
      /*on_result=*/std::move( on_result ),
      /*get_view_fun=*/get_view_fn          //
  );

  co_return co_await s_promise.wait();
}

/****************************************************************
** WindowPlane
*****************************************************************/
Plane& WindowPlane::impl() { return *impl_; }

WindowPlane::~WindowPlane() = default;

WindowPlane::WindowPlane() : impl_( new Impl() ) {}

WindowManager& WindowPlane::manager() { return impl_->wm; }

wait<> WindowPlane::message_box( string_view msg ) {
  wait_promise<>     p;
  unique_ptr<Window> win = async_window_builder(
      impl_->wm,
      ui::PlainMessageBoxView::create( string( msg ), p ),
      /*auto_pad=*/true );
  // Need to keep p alive since it is held by refererence by the
  // message box view.
  co_await p.wait();
}

void WindowPlane::transient_message_box( string_view msg ) {
  impl_->wm.transient_messages().send( string( msg ) );
}

wait<maybe<int>> WindowPlane::select_box(
    string_view msg, vector<SelectBoxOption> const& options,
    e_input_required required, maybe<int> initial_selection ) {
  lg.info( "question: \"{}\"", msg );
  vector<ui::OptionSelectItemView::Option> view_options;
  view_options.reserve( options.size() );
  for( auto& option : options )
    view_options.push_back(
        { .name = option.name, .enabled = option.enabled } );
  auto selector_view = make_unique<ui::OptionSelectView>(
      view_options, initial_selection );
  auto* p_selector_view = selector_view.get();

  wait_promise<maybe<int>> p;

  auto on_input = [&]( input::event_t const& event ) {
    bool selected = false;
    switch( event.to_enum() ) {
      case input::e_input_event::key_event: {
        auto const& key_event = event.as<input::key_event_t>();
        if( key_event.change != input::e_key_change::down )
          return true;
        if( required == e_input_required::no &&
            key_event.keycode == ::SDLK_ESCAPE ) {
          lg.info( "cancelled." );
          p.set_value( nothing );
          return true; // handled.
        }
        if( key_event.keycode != ::SDLK_RETURN &&
            key_event.keycode != ::SDLK_KP_ENTER &&
            key_event.keycode != ::SDLK_SPACE &&
            key_event.keycode != ::SDLK_KP_5 )
          return false; // not handled.
        // An enter-like key is being released, so take action,
        // but only if there is an option highlighted. There may
        // not be if there are no enabled items.
        selected = p_selector_view->get_selected().has_value();
        break;
      }
      case input::e_input_event::mouse_button_event: {
        auto const& button_event =
            event.as<input::mouse_button_event_t>();
        // The way this works is that the bottom-down event will
        // not be handled here, so it will pass through to the
        // OptionSelectView's mouse button handler, causing an
        // option to potentially be selected, then on the
        // mouse-up event we will intercept it here and set se-
        // lected=true, then get the selected item.
        if( button_event.buttons !=
            input::e_mouse_button_event::left_up )
          break;
        if( p_selector_view->enabled_option_at_point(
                button_event.pos ) )
          selected = true;
        break;
      }
      default:
        break;
    }
    if( selected ) {
      UNWRAP_CHECK( result, p_selector_view->get_selected() );
      CHECK( result >= 0 && result < int( options.size() ) );
      lg.info( "selected: {}", options[result].name );
      p.set_value( result );
    }
    bool handled = selected;
    return handled;
  };

  auto on_input_view = make_unique<ui::OnInputView>(
      std::move( selector_view ), std::move( on_input ) );
  TextMarkupInfo const& m_info = ui::default_text_markup_info();
  TextReflowInfo const& r_info = ui::default_text_reflow_info();
  auto                  text_view =
      make_unique<ui::TextView>( string( msg ), m_info, r_info );
  vector<unique_ptr<ui::View>> vert_views;
  vert_views.push_back( std::move( text_view ) );
  vert_views.push_back( std::move( on_input_view ) );
  auto va_view = make_unique<ui::VerticalArrayView>(
      std::move( vert_views ),
      ui::VerticalArrayView::align::left );

  unique_ptr<ui::View> view   = std::move( va_view );
  ui::CompositeView*   p_view = view->cast<ui::CompositeView>();
  unique_ptr<Window>   win =
      async_window_builder( impl_->wm, std::move( view ),
                            /*auto_pad=*/true );

  p_selector_view->grow_to( p_view->delta().w );
  p_view->children_updated();

  // Need to co_await instead of returning so that the window
  // stays alive while we wait.
  co_return co_await p.wait();
}

wait<maybe<string>> WindowPlane::str_input_box(
    string_view msg, string_view initial_text,
    e_input_required required ) {
  wait_promise<maybe<string>> p;
  unique_ptr<Window>          win = text_input_box(
      impl_->wm, msg, initial_text, required, L( _.size() > 0 ),
      [&p]( maybe<string> result ) { p.set_value( result ); } );
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
      impl_->wm, options.msg, initial_text, options.required,
      make_int_validator( options.min, options.max ),
      [&p]( maybe<string> result ) {
        p.set_value( result.bind( L( base::stoi( _ ) ) ) );
      } );
  co_return co_await p.wait();
}

int WindowPlane::num_windows_created() const {
  return impl_->num_windows_created();
}

int WindowPlane::num_windows_currently_open() const {
  return impl_->num_windows_currently_open();
}

} // namespace rn
