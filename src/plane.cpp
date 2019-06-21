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
#include "aliases.hpp"
#include "console.hpp"
#include "europe.hpp"
#include "frame.hpp"
#include "image.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "menu.hpp"
#include "ranges.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "window.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/misc.hpp"
#include "base-util/variant.hpp"

// C++ standard library
#include <algorithm>
#include <array>

using namespace std;

namespace rn {

namespace {

constexpr auto num_planes =
    static_cast<size_t>( e_plane::_size() );

/****************************************************************
** Plane Textures
*****************************************************************/
// Planes are rendered from 0 --> count.
array<ObserverPtr<Plane>, num_planes> planes;
array<Texture, num_planes>            textures;

ObserverPtr<Plane>& plane( e_plane plane ) {
  auto idx = static_cast<size_t>( plane._value );
  CHECK( idx < planes.size() );
  return planes[idx];
}

Texture& plane_tx( e_plane plane ) {
  auto idx = static_cast<size_t>( plane._value );
  CHECK( idx < planes.size() );
  return textures[idx];
}

/****************************************************************
** The InactivePlane Plane
*****************************************************************/
struct InactivePlane : public Plane {
  InactivePlane() {}
  bool enabled() const override { return false; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& /*unused*/ ) const override {}
};

InactivePlane dummy;

/****************************************************************
** The Omni Plane
*****************************************************************/
// This plane is intended to be:
//
//   1) Always present / enabled
//   2) Always invisible
//   3) Always on top
//   4) Catching any global events (such as special key presses)
//
struct OmniPlane : public Plane {
  OmniPlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& /*unused*/ ) const override {}
  bool input( input::event_t const& event ) override {
    bool handled = false;
    switch_v( event ) {
      case_v( input::quit_event_t ) { throw exception_exit{}; }
      case_v( input::key_event_t ) {
        auto& key_event = val;
        if( key_event.change != input::e_key_change::up )
          break_v;
        handled = true;
        switch( key_event.keycode ) {
          case ::SDLK_F11:
            if( is_window_fullscreen() ) {
              toggle_fullscreen();
              restore_window();
            } else {
              toggle_fullscreen();
            }
            break;
          case ::SDLK_EQUALS:
          case ::SDLK_KP_PLUS:      //
            inc_resolution_scale(); //
            break;
          case ::SDLK_MINUS:
          case ::SDLK_KP_MINUS:     //
            dec_resolution_scale(); //
            break;
          default: handled = false; break;
        }
      }
      default_v_no_check;
    }
    return handled;
  }
};

OmniPlane g_omni_plane;

Plane* omni_plane() { return &g_omni_plane; }

/****************************************************************
** Dragging Metadata
*****************************************************************/
struct DragState {
  explicit DragState() { reset(); }

  // This is the plan that is currently receiving mouse dragging
  // events (nullopt if there is not dragging event happening).
  Opt<e_plane> plane{};
  bool         send_as_motion{false};
  Opt<Delta>   projection{};

  void reset() {
    plane          = nullopt;
    send_as_motion = false;
    projection     = nullopt;
  }
};

DragState g_drag_state;

/****************************************************************
** Plane Range Combinators
*****************************************************************/
auto relevant_planes() {
  auto not_covers_screen = L( !_.second->covers_screen() );
  auto enabled           = L( _.second->enabled() );
  return rv::zip( values<e_plane>, planes ) //
         | rv::filter( enabled )            //
         | rv::reverse                      //
         | take_while_inclusive( not_covers_screen );
}

auto planes_to_draw() { return relevant_planes() | rv::reverse; }

/****************************************************************
** Initialization
*****************************************************************/
void init_planes() {
  // By default, all planes are dummies, unless we provide an
  // object below.
  planes.fill( ObserverPtr<Plane>( &dummy ) );

  plane( e_plane::viewport ).reset( viewport_plane() );
  plane( e_plane::panel ).reset( panel_plane() );
  plane( e_plane::image ).reset( image_plane() );
  // plane( e_plane::colony ).reset( colony_plane() );
  plane( e_plane::europe ).reset( europe_plane() );
  plane( e_plane::effects ).reset( effects_plane() );
  plane( e_plane::window ).reset( window_plane() );
  plane( e_plane::menu ).reset( menu_plane() );
  plane( e_plane::console ).reset( console_plane() );
  plane( e_plane::omni ).reset( omni_plane() );

  // No plane must be null, they must all point to a valid Plane
  // object even if it is the dummy above.
  for( auto p : planes ) { CHECK( p ); }

  // Call any custom initialization routines.
  for( auto p : planes ) { p->initialize(); }

  // Initialize the textures. These are intended to be large
  // enough to cover the entire screen at a scale factor of unity
  // if necessary (i.e., when in fullscreen mode with smallest
  // scale) but will also work when the main window is restored
  // (only part of them will be used). Their coordinates are mea-
  // sured in logical coordinates (which means, typically, that
  // they will be smaller than the full screen resolution).
  for( auto& tx : textures ) {
    tx = create_screen_physical_sized_texture();
    clear_texture_transparent( tx );
  }
}

void cleanup_planes() {
  // This actually just destroys the textures, since the planes
  // will be held by value as global variables elsewhere.
  for( auto& tx : textures ) tx = {};
}

REGISTER_INIT_ROUTINE( planes );

input::mouse_drag_event_t project_drag_event(
    input::mouse_drag_event_t const& drag_event,
    Delta const&                     along ) {
  // Takes the vector defined by (end-start) and projects it
  // along the `along` vector, returning a new `end`.
  auto project = []( Coord const& start, Coord const& end,
                     Delta const& along ) {
    return start + ( end - start ).projected_along( along );
  };

  auto res = drag_event;
  res.prev = project( /*start=*/drag_event.state.origin,
                      /*end=*/drag_event.prev,
                      /*along=*/along );
  res.pos  = project( /*start=*/drag_event.state.origin,
                     /*end=*/drag_event.pos,
                     /*along=*/along );
  return res;
}

} // namespace

/****************************************************************
** Plane Default Implementations
*****************************************************************/
void Plane::initialize() {}

Plane& Plane::get( e_plane p ) { return *plane( p ); }

bool Plane::input( input::event_t const& /*unused*/ ) {
  return false;
}

void Plane::on_frame_tick() {}

Plane::DragInfo Plane::can_drag(
    input::e_mouse_button /*unused*/, Coord /*unused*/ ) {
  return e_accept_drag::no;
}

void Plane::on_drag( input::e_mouse_button /*unused*/,
                     Coord /*unused*/, Coord /*unused*/,
                     Coord /*unused*/ ) {}

void Plane::on_drag_finished( input::e_mouse_button /*unused*/,
                              Coord /*unused*/,
                              Coord /*unused*/ ) {}

OptRef<Plane::MenuClickHandler> Plane::menu_click_handler(
    e_menu_item /*unused*/ ) const {
  return nullopt;
}

/****************************************************************
** External API
*****************************************************************/
void draw_all_planes( Texture const& tx ) {
  clear_texture_black( tx );

  // This will find the last plane that will render (opaquely)
  // over every pixel. If one is found then we will not render
  // any planes before it. This is technically not necessary, but
  // saves rendering work by avoiding to render things that would
  // go unseen anyway.
  for( auto [e, ptr] : planes_to_draw() ) {
    set_render_target( plane_tx( e ) );
    ptr->draw( plane_tx( e ) );
    copy_texture( plane_tx( e ), tx );
  }
}

void update_all_planes() {
  for( auto [e, ptr] : relevant_planes() ) ptr->on_frame_tick();
}

bool send_input_to_planes( input::event_t const& event ) {
  using namespace input;
  if_v( event, input::mouse_drag_event_t, drag_event ) {
    if( g_drag_state.plane ) {
      auto& plane = Plane::get( *g_drag_state.plane );
      CHECK( plane.enabled() );
      // Drag should already be in progress.
      //
      // FIXME: It seems that this can fire in very rare circum-
      // stances. If it happens again, try adding an error log-
      // ging statement into the input module when it tries to
      // send to e_drag_phase::begin events in succession. That
      // will tell us if the problem is this module or the input
      // module.
      CHECK( drag_event->state.phase != +e_drag_phase::begin );
      if( g_drag_state.send_as_motion ) {
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
        if( drag_event->state.phase == +e_drag_phase::end ) {
          lg.debug( "finished `{}` drag motion event",
                         *g_drag_state.plane );
          auto maybe_button =
              input::drag_event_to_mouse_button_event(
                  *drag_event );
          CHECK( maybe_button.has_value() );
          (void)plane.input( *maybe_button );
          g_drag_state.reset();
        }
      } else {
        // The plane wishes to be sent these drag events as drag
        // events. There is already a drag in progress, so send
        // this one to the same plane that accepted the initial
        // drag event.
        //
        // First project the coordinates if it was requested.
        input::mouse_drag_event_t prj_drag_event =
            ( g_drag_state.projection.has_value() )
                ? project_drag_event( *drag_event,
                                      *g_drag_state.projection )
                : *drag_event;
        plane.on_drag( prj_drag_event.button,
                       prj_drag_event.state.origin,
                       prj_drag_event.prev, //
                       prj_drag_event.pos );
        // If the drag is finished then send out that event.
        if( prj_drag_event.state.phase == +e_drag_phase::end ) {
          lg.debug( "finished `{}` drag event",
                         *g_drag_state.plane );
          plane.on_drag_finished( prj_drag_event.button,
                                  prj_drag_event.state.origin,
                                  prj_drag_event.pos );
          g_drag_state.reset();
        }
      }
      // Here it is assumed/required that the plane handle it
      // because the plane has already accepted this drag.
      return true;
    }
    // No drag plane registered to accept the event, so lets
    // send out the event but only if it's a `begin` event.
    if( drag_event->state.phase == +e_drag_phase::begin ) {
      for( auto [e, plane] : relevant_planes() ) {
        // Note here we use the origin position of the mouse drag
        // as opposed to the current mouse position because that
        // is what is relevant for determining whether the plane
        // can handle the drag event or not (at this point, even
        // though we are in a `begin` event, the current mouse
        // position may already have moved a bit from the orig-
        // in).
        auto drag_desc = plane->can_drag(
            drag_event->button, drag_event->state.origin );
        switch( drag_desc.accept ) {
          // If the plane doesn't want to handle it then move
          // on to ask the next one.
          case Plane::e_accept_drag::no: continue;
          case Plane::e_accept_drag::motion: {
            // In this case the plane says that it wants to re-
            // ceive the events, but just as normal mouse
            // move/click events.
            g_drag_state.reset();
            g_drag_state.plane          = e;
            g_drag_state.send_as_motion = true;
            lg.debug( "plane `{}` can drag as motion.", e );
            auto motion =
                input::drag_event_to_mouse_motion_event(
                    *drag_event );
            auto maybe_button =
                input::drag_event_to_mouse_button_event(
                    *drag_event );
            // Drag events in the e_drag_phase::begin phase
            // should not contain any button events because the
            // initial mouse-down will always be sent as a normal
            // click event.
            CHECK( !maybe_button.has_value() );
            (void)plane->input( motion );
            // All events within a drag are assumed handled.
            return true;
          }
          case Plane::e_accept_drag::swallow:
            // In this case the plane says that it doesn't want
            // to handle it AND it doesn't want anyone else to
            // handle it.
            lg.debug( "plane `{}` swallowed drag.", e );
            return true;
          case Plane::e_accept_drag::yes:
            // Wants to handle it.
            g_drag_state.reset();
            g_drag_state.plane          = e;
            g_drag_state.send_as_motion = false;
            g_drag_state.projection     = drag_desc.projection;
            lg.debug( "plane `{}` can drag", e );
            // Now we must send it an on_drag because this mouse
            // event that we're dealing with serves both to tell
            // us about a new drag even but also may have a mouse
            // delta in it that needs to be processed.
            //
            // First project the coordinates if it was requested.
            input::mouse_drag_event_t prj_drag_event =
                ( g_drag_state.projection.has_value() )
                    ? project_drag_event(
                          *drag_event, *g_drag_state.projection )
                    : *drag_event;
            plane->on_drag( prj_drag_event.button,
                            prj_drag_event.state.origin,
                            prj_drag_event.prev, //
                            prj_drag_event.pos );
            return true;
        }
      }
    }
    // If no one handled it then that's it.
    return false;
  }

  // Just a normal event, so send it out using the usual proto-
  // col.
  for( auto p : relevant_planes() )
    if( p.second->input( event ) ) return true;

  return false;
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
      maybe_handler.value().get()();
      return;
    }
  }
  SHOULD_NOT_BE_HERE;
}

#define MENU_ITEM_HANDLER_PLANE( item )                   \
  MENU_ITEM_HANDLER(                                      \
      e_menu_item::item,                                  \
      [] { on_menu_item_clicked( e_menu_item::item ); },  \
      [] {                                                \
        return is_menu_item_enabled( e_menu_item::item ); \
      } )

MENU_ITEM_HANDLER_PLANE( about );
MENU_ITEM_HANDLER_PLANE( economics_advisor );
MENU_ITEM_HANDLER_PLANE( european_advisor );
MENU_ITEM_HANDLER_PLANE( fortify );
MENU_ITEM_HANDLER_PLANE( founding_father_help );
MENU_ITEM_HANDLER_PLANE( military_advisor );
MENU_ITEM_HANDLER_PLANE( restore_zoom );
MENU_ITEM_HANDLER_PLANE( retire );
MENU_ITEM_HANDLER_PLANE( revolution );
MENU_ITEM_HANDLER_PLANE( sentry );
MENU_ITEM_HANDLER_PLANE( terrain_help );
MENU_ITEM_HANDLER_PLANE( units_help );
MENU_ITEM_HANDLER_PLANE( zoom_in );
MENU_ITEM_HANDLER_PLANE( zoom_out );

} // namespace

} // namespace rn
