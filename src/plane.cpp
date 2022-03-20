/****************************************************************
**plane.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-30.
*
* Description: Rendering planes.
*
*****************************************************************/
#include "plane.hpp"

// Revolution Now
#include "colony-view.hpp"
#include "console.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "main-menu.hpp"
#include "menu.hpp"
#include "old-world-view.hpp"
#include "panel.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "tiles.hpp"
#include "window.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"
#include "base/to-str-tags.hpp"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <algorithm>
#include <array>

using namespace std;

namespace rn {

namespace rl = ::base::rl;

namespace {

constexpr auto num_planes =
    static_cast<size_t>( refl::enum_count<e_plane> );

// The `values` array should be a constexpr.
vector<e_plane> g_plane_list;

// Planes are rendered from 0 --> count.
array<Plane*, num_planes> planes;

Plane*& plane( e_plane plane ) {
  auto idx = static_cast<int>( plane );
  CHECK( idx < int( planes.size() ) );
  return planes[idx];
}

/****************************************************************
** The InactivePlane Plane
*****************************************************************/
struct InactivePlane : public Plane {
  InactivePlane() {}
  bool covers_screen() const override { return false; }
  void draw( rr::Renderer& ) const override {}
};

InactivePlane dummy;

/****************************************************************
** The Omni Plane
*****************************************************************/
// This plane is intended to be:
//
//   1) Always present
//   2) Always invisible
//   3) Always on top
//   4) Catching any global events (such as special key presses)
//
struct OmniPlane : public Plane {
  OmniPlane() = default;
  bool covers_screen() const override { return false; }
  void draw( rr::Renderer& renderer ) const override {
    rr::Painter painter = renderer.painter();
    render_sprite( painter, e_tile::mouse_arrow1,
                   input::current_mouse_position() - 16_w );
  }
  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    switch( event.to_enum() ) {
      case input::e_input_event::quit_event:
        throw exception_exit{};
      case input::e_input_event::win_event: //
        break;
      case input::e_input_event::key_event: {
        auto& key_event = event.get<input::key_event_t>();
        if( key_event.change != input::e_key_change::down )
          break;
        handled = e_input_handled::yes;
        switch( key_event.keycode ) {
          case ::SDLK_F12:
            // if( !screenshot() )
            //   lg.warn( "failed to take screenshot." );
            break;
          case ::SDLK_F11:
            if( is_window_fullscreen() ) {
              toggle_fullscreen();
              restore_window();
            } else {
              toggle_fullscreen();
            }
            break;
          case ::SDLK_q:
            if( key_event.mod.ctrl_down ) throw exception_exit{};
            handled = e_input_handled::no;
            break;
          default: //
            handled = e_input_handled::no;
            break;
        }
        break;
      }
      default: //
        break;
    }
    return handled;
  }
};

OmniPlane g_omni_plane;

Plane* omni_plane() { return &g_omni_plane; }

/****************************************************************
** Dragging Metadata
*****************************************************************/
enum class e_drag_send_mode { normal, raw, motion };

struct DragState {
  e_plane          plane;
  e_drag_send_mode mode = e_drag_send_mode::normal;
};
NOTHROW_MOVE( DragState );

maybe<DragState> g_drag_state;

/****************************************************************
** Plane Range Combinators
*****************************************************************/
auto relevant_planes() {
  static auto not_covers_screen =
      L( !_.second->covers_screen() );
  // Use as_const because the view mechanism naturally will prop-
  // agate references to the contents of that vector and so we
  // want them to be const to avoid any unintended mutation,
  // which can easily happen because the references will be in-
  // side std::pairs, which will happily assign through to the
  // references on assignment.
  auto r_plane_list = rl::rall( as_const( g_plane_list ) );
  auto r_plane_ptrs = r_plane_list.copy_me().map_L( plane( _ ) );
  return rl::zip( r_plane_list, r_plane_ptrs )
      .take_while_incl( not_covers_screen );
}

/****************************************************************
** Initialization
*****************************************************************/
void init_planes() {
  // By default, all planes are dummies, unless we provide an
  // object below.
  planes.fill( &dummy );

  plane( e_plane::main_menu ) = main_menu_plane();
  plane( e_plane::land_view ) = land_view_plane();
  plane( e_plane::panel )     = panel_plane();
  plane( e_plane::colony )    = colony_plane();
  plane( e_plane::old_world ) = old_world_plane();
  plane( e_plane::window )    = window_plane();
  plane( e_plane::menu )      = menu_plane();
  plane( e_plane::console )   = console_plane();
  plane( e_plane::omni )      = omni_plane();

  // No plane must be null, they must all point to a valid Plane
  // object even if it is the dummy above.
  for( auto p : planes ) { CHECK( p ); }

  // Call any custom initialization routines.
  for( auto p : planes ) { p->initialize(); }
}

void cleanup_planes() {}

REGISTER_INIT_ROUTINE( planes );

} // namespace

/****************************************************************
** Plane Default Implementations
*****************************************************************/
void Plane::initialize() {}

Plane& Plane::get( e_plane p ) { return *plane( p ); }

Plane::e_input_handled Plane::input(
    input::event_t const& /*unused*/ ) {
  return e_input_handled::no;
}

void Plane::advance_state() {}

Plane::e_accept_drag Plane::can_drag(
    input::e_mouse_button /*unused*/, Coord /*unused*/ ) {
  return e_accept_drag::no;
}

void Plane::on_drag( input::mod_keys const& /*unused*/,
                     input::e_mouse_button /*unused*/,
                     Coord /*unused*/, Coord /*unused*/,
                     Coord /*unused*/ ) {}

void Plane::on_drag_finished( input::mod_keys const& /*unused*/,
                              input::e_mouse_button /*unused*/,
                              Coord /*unused*/,
                              Coord /*unused*/ ) {}

maybe<Plane::MenuClickHandler> Plane::menu_click_handler(
    e_menu_item /*unused*/ ) const {
  return nothing;
}

/****************************************************************
** External API
*****************************************************************/
void set_plane_list( vector<e_plane> const& arr ) {
  g_plane_list = arr;
  g_plane_list.push_back( e_plane::console );
  g_plane_list.push_back( e_plane::omni );
  lg.debug( "setting next plane list: {}",
            base::FmtJsonStyleList{ g_plane_list } );
}

bool is_plane_enabled( e_plane plane ) {
  return std::find( g_plane_list.begin(), g_plane_list.end(),
                    plane ) != g_plane_list.end();
}

void draw_all_planes( rr::Renderer& renderer ) {
  renderer.clear_screen( gfx::pixel::black() );

  auto reverse_planes_to_draw = relevant_planes().to_vector();
  auto planes_to_draw = rl::rall( reverse_planes_to_draw );

  // This will find the last plane that will render (opaquely)
  // over every pixel. If one is found then we will not render
  // any planes before it. This is technically not necessary, but
  // saves rendering work by avoiding to render things that would
  // go unseen anyway.
  for( auto [e, ptr] : planes_to_draw ) ptr->draw( renderer );
}

void advance_plane_state() {
  for( auto [e, ptr] : relevant_planes() ) ptr->advance_state();
}

void reinitialize_planes() {
  for( auto p : planes ) { p->initialize(); }
}

Plane::e_input_handled send_input_to_planes(
    input::event_t const& event ) {
  using namespace input;
  auto* drag_event =
      std::get_if<input::mouse_drag_event_t>( &event );
  if( drag_event == nullptr ) {
    // Normal event, so send it out using the usual protocol.
    for( auto p : relevant_planes() ) {
      switch( p.second->input( event ) ) {
        case Plane::e_input_handled::yes:
          return Plane::e_input_handled::yes;
        case Plane::e_input_handled::no: //
          break;
      }
    }
    return Plane::e_input_handled::no;
  }

  // We have a drag event. Test if there is a plane registered to
  // receive it already.
  if( g_drag_state ) {
    auto& plane = Plane::get( g_drag_state->plane );
    // Drag should already be in progress.
    CHECK( drag_event->state.phase != e_drag_phase::begin );
    switch( g_drag_state->mode ) {
      case e_drag_send_mode::motion: {
        // The plane wants to be sent this drag event as regular
        // mouse motion / click events, so we need to convert the
        // drag events to those events and send them.
        auto motion = input::drag_event_to_mouse_motion_event(
            *drag_event );
        // There is already a drag in progress, so send this one
        // to the same plane that accepted the initial drag
        // event. Motion first, then button, since if the button
        // event exists then it's a mouse-up, should should be
        // sent after the motion. The mouse motion event might
        // not exist if this is a drag end event consisting just
        // of a released button.
        (void)plane.input( motion );
        // If the drag is finished then send out that event.
        if( drag_event->state.phase == e_drag_phase::end ) {
          auto maybe_button =
              input::drag_event_to_mouse_button_event(
                  *drag_event );
          CHECK( maybe_button.has_value() );
          (void)plane.input( *maybe_button );
        }
        break;
      }
      case e_drag_send_mode::normal: {
        // The plane wishes to be sent these drag events using
        // the dedicated plane drag methods. There is already a
        // drag in progress, so send this one to the same plane
        // that accepted the initial drag event.
        plane.on_drag( drag_event->mod, drag_event->button,
                       drag_event->state.origin,
                       drag_event->prev, drag_event->pos );
        // If the drag is finished then send out that event.
        if( drag_event->state.phase == e_drag_phase::end )
          plane.on_drag_finished(
              drag_event->mod, drag_event->button,
              drag_event->state.origin, drag_event->pos );
        break;
      }
      case e_drag_send_mode::raw: {
        (void)plane.input( event );
        break;
      }
    }
    if( drag_event->state.phase == e_drag_phase::end )
      g_drag_state.reset();
    // Here it is assumed/required that the plane handle it be-
    // cause the plane has already accepted this drag.
    return Plane::e_input_handled::yes;
  }

  // No drag plane registered to accept the event, so lets send
  // out the event but only if it's a `begin` event.
  if( drag_event->state.phase == e_drag_phase::begin ) {
    for( auto [e, plane] : relevant_planes() ) {
      // Note here we use the origin position of the mouse drag
      // as opposed to the current mouse position because that is
      // what is relevant for determining whether the plane can
      // handle the drag event or not (at this point, even though
      // we are in a `begin` event, the current mouse position
      // may already have moved a bit from the origin).
      Plane::e_accept_drag accept = plane->can_drag(
          drag_event->button, drag_event->state.origin );
      switch( accept ) {
        // If the plane doesn't want to handle it then move on to
        // ask the next one.
        case Plane::e_accept_drag::no: continue;
        case Plane::e_accept_drag::motion: {
          // In this case the plane says that it wants to receive
          // the events, but just as normal mouse move/click
          // events.
          g_drag_state.reset();
          g_drag_state.emplace();
          g_drag_state->plane = e;
          g_drag_state->mode  = e_drag_send_mode::motion;
          auto motion = input::drag_event_to_mouse_motion_event(
              *drag_event );
          auto maybe_button =
              input::drag_event_to_mouse_button_event(
                  *drag_event );
          // Drag events in the e_drag_phase::begin phase should
          // not contain any button events because the initial
          // mouse-down will always be sent as a normal click
          // event.
          CHECK( !maybe_button.has_value() );
          (void)plane->input( motion );
          // All events within a drag are assumed handled.
          return Plane::e_input_handled::yes;
        }
        case Plane::e_accept_drag::swallow:
          g_drag_state.reset();
          // The plane says that it doesn't want to handle it AND
          // it doesn't want anyone else to handle it.
          return Plane::e_input_handled::yes;
        case Plane::e_accept_drag::yes:
          // Wants to handle it.
          g_drag_state.reset();
          g_drag_state.emplace();
          g_drag_state->plane = e;
          g_drag_state->mode  = e_drag_send_mode::normal;
          // Now we must send it an on_drag because this mouse
          // event that we're dealing with serves both to tell us
          // about a new drag even but also may have a mouse
          // delta in it that needs to be processed.
          plane->on_drag( drag_event->mod, drag_event->button,
                          drag_event->state.origin,
                          drag_event->prev, drag_event->pos );
          return Plane::e_input_handled::yes;
        case Plane::e_accept_drag::yes_but_raw:
          g_drag_state.reset();
          g_drag_state.emplace();
          g_drag_state->plane = e;
          g_drag_state->mode  = e_drag_send_mode::raw;
          (void)plane->input( event );
          return Plane::e_input_handled::yes;
      }
    }
  }
  // If no one handled it then that's it.
  return Plane::e_input_handled::no;
}

/****************************************************************
** Plane Menu Handling
*****************************************************************/
namespace {

bool is_menu_item_enabled_( e_menu_item item ) {
  for( auto p : relevant_planes() ) {
    if( p.second->menu_click_handler( item ).has_value() )
      return true;
  }
  return false;
}

auto is_menu_item_enabled =
    per_frame_memoize( is_menu_item_enabled_ );

void on_menu_item_clicked( e_menu_item item ) {
  for( auto p : relevant_planes() ) {
    if( auto maybe_handler =
            p.second->menu_click_handler( item );
        maybe_handler.has_value() ) {
      maybe_handler.value()();
      return;
    }
  }
  SHOULD_NOT_BE_HERE;
}

#define MENU_ITEM_HANDLER_PLANE( item )                        \
  MENU_ITEM_HANDLER(                                           \
      item, [] { on_menu_item_clicked( e_menu_item::item ); }, \
      [] {                                                     \
        return is_menu_item_enabled( e_menu_item::item );      \
      } )

MENU_ITEM_HANDLER_PLANE( about );
MENU_ITEM_HANDLER_PLANE( economics_advisor );
MENU_ITEM_HANDLER_PLANE( european_advisor );
MENU_ITEM_HANDLER_PLANE( find_blinking_unit );
MENU_ITEM_HANDLER_PLANE( fortify );
MENU_ITEM_HANDLER_PLANE( founding_father_help );
MENU_ITEM_HANDLER_PLANE( military_advisor );
MENU_ITEM_HANDLER_PLANE( restore_zoom );
MENU_ITEM_HANDLER_PLANE( retire );
MENU_ITEM_HANDLER_PLANE( sentry );
MENU_ITEM_HANDLER_PLANE( terrain_help );
MENU_ITEM_HANDLER_PLANE( units_help );
MENU_ITEM_HANDLER_PLANE( zoom_in );
MENU_ITEM_HANDLER_PLANE( zoom_out );

} // namespace

} // namespace rn
