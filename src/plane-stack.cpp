/****************************************************************
**plane-stack.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: A (quasi) stack for storing the list of active
*              planes.
*
*****************************************************************/
#include "plane-stack.hpp"

// Revolution Now
#include "colony-view.hpp"
#include "console.hpp"
#include "harbor-view.hpp"
#include "land-view.hpp"
#include "main-menu.hpp"
#include "map-edit.hpp"
#include "menu.hpp"
#include "omni.hpp"
#include "panel.hpp"
#include "plane.hpp"
#include "window.hpp"

// render
#include "render/renderer.hpp"

// base
#include "base/range-lite.hpp"

using namespace std;

#define PLANE_ACCESSOR_IMPL( type, name ) \
  type& Planes::name() {                  \
    PlaneGroup& pg = back();              \
    auto*       p  = pg.name;             \
    CHECK( p != nullptr );                \
    return *p;                            \
  }                                       \
  type const& Planes::name() const {      \
    PlaneGroup const& pg = back();        \
    auto*             p  = pg.name;       \
    CHECK( p != nullptr );                \
    return *p;                            \
  }

namespace rn {

/****************************************************************
** PlaneGroup
*****************************************************************/
maybe<Plane&> plane_pointer( PlaneGroup const& group,
                             e_plane           plane ) {
  switch( plane ) {
    case e_plane::omni: {
      auto* p = group.omni;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::console: {
      auto* p = group.console;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::window: {
      auto* p = group.window;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::main_menu: {
      auto* p = group.main_menu;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::menu: {
      auto* p = group.menu;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::panel: {
      auto* p = group.panel;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::land_view: {
      auto* p = group.land_view;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::map_edit: {
      auto* p = group.map_edit;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::colony: {
      auto* p = group.colony;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
    case e_plane::harbor: {
      auto* p = group.harbor;
      if( p == nullptr ) return nothing;
      return p->impl();
    }
  }
}

/****************************************************************
** Planes
*****************************************************************/
Planes::Planes() { groups_.emplace_back(); }

PlaneGroup& Planes::back() {
  CHECK( !groups_.empty() );
  return groups_.back();
}

PlaneGroup const& Planes::back() const {
  CHECK( !groups_.empty() );
  return groups_.back();
}

PLANE_ACCESSOR_IMPL( OmniPlane, omni );
PLANE_ACCESSOR_IMPL( ConsolePlane, console );
PLANE_ACCESSOR_IMPL( WindowPlane, window );
PLANE_ACCESSOR_IMPL( MainMenuPlane, main_menu );
PLANE_ACCESSOR_IMPL( MenuPlane, menu );
PLANE_ACCESSOR_IMPL( PanelPlane, panel );
PLANE_ACCESSOR_IMPL( LandViewPlane, land_view );
PLANE_ACCESSOR_IMPL( MapEditPlane, map_edit );
PLANE_ACCESSOR_IMPL( ColonyPlane, colony );
PLANE_ACCESSOR_IMPL( HarborPlane, harbor );

Planes::popper Planes::new_group() {
  groups_.emplace_back();
  return popper( *this );
}

Planes::popper Planes::new_copied_group() {
  PlaneGroup pg;
  if( !groups_.empty() ) pg = back();
  groups_.push_back( pg );
  return popper( *this );
}

Planes::popper::~popper() {
  CHECK( !planes_.groups_.empty() );
  planes_.groups_.pop_back();
}

vector<Plane*> Planes::active_planes() const {
  vector<Plane*> res;
  if( groups_.empty() ) return res;
  res.reserve( kNumPlanes );
  PlaneGroup const& pg = back();
  for( e_plane plane : refl::enum_values<e_plane> ) {
    maybe<Plane&> p = plane_pointer( pg, plane );
    if( !p.has_value() ) continue;
    // This will ensure that planes that are completely obscured
    // by a another plane on top of them will not be considered
    // for either drawing or input.
    if( p->covers_screen() ) res.clear();
    res.push_back( addressof( *p ) );
  }
  return res;
}

void Planes::draw( rr::Renderer& renderer ) const {
  renderer.clear_screen( gfx::pixel::black() );
  if( groups_.empty() ) return;
  vector<Plane*> active = active_planes();
  for( Plane* plane : active ) plane->draw( renderer );
}

void Planes::advance_state() {
  if( groups_.empty() ) return;
  vector<Plane*> active = active_planes();
  for( Plane* plane : active ) plane->advance_state();
}

e_input_handled Planes::send_input(
    input::event_t const& event ) {
  if( groups_.empty() ) return e_input_handled::no;
  using namespace input;
  auto* drag_event =
      std::get_if<input::mouse_drag_event_t>( &event );
  if( drag_event == nullptr ) {
    vector<Plane*> active   = active_planes();
    auto           reversed = base::rl::rall( active );
    // Normal event, so send it out using the usual protocol.
    for( Plane* plane : reversed ) {
      switch( plane->input( event ) ) {
        case e_input_handled::yes: return e_input_handled::yes;
        case e_input_handled::no: break;
      }
    }
    return e_input_handled::no;
  }

  // We have a drag event. Test if there is a plane registered
  // to receive it already.
  if( drag_state_ ) {
    CHECK( drag_state_->plane != nullptr );
    Plane& plane = *drag_state_->plane;
    // Drag should already be in progress.
    CHECK( drag_event->state.phase != e_drag_phase::begin );
    switch( drag_state_->mode ) {
      case e_drag_send_mode::motion: {
        // The plane wants to be sent this drag event as
        // regular mouse motion / click events, so we need to
        // convert the drag events to those events and send
        // them.
        auto motion = input::drag_event_to_mouse_motion_event(
            *drag_event );
        // There is already a drag in progress, so send this
        // one to the same plane that accepted the initial drag
        // event. Motion first, then button, since if the
        // button event exists then it's a mouse-up, should
        // should be sent after the motion. The mouse motion
        // event might not exist if this is a drag end event
        // consisting just of a released button.
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
      drag_state_.reset();
    // Here it is assumed/required that the plane handle it be-
    // cause the plane has already accepted this drag.
    return e_input_handled::yes;
  }

  // No drag plane registered to accept the event, so lets send
  // out the event but only if it's a `begin` event.
  if( drag_event->state.phase == e_drag_phase::begin ) {
    vector<Plane*> active   = active_planes();
    auto           reversed = base::rl::rall( active );
    for( Plane* plane : reversed ) {
      // Note here we use the origin position of the mouse drag
      // as opposed to the current mouse position because that
      // is what is relevant for determining whether the plane
      // can handle the drag event or not (at this point, even
      // though we are in a `begin` event, the current mouse
      // position may already have moved a bit from the
      // origin).
      Plane::e_accept_drag accept = plane->can_drag(
          drag_event->button, drag_event->state.origin );
      switch( accept ) {
        // If the plane doesn't want to handle it then move on
        // to ask the next one.
        case Plane::e_accept_drag::no: continue;
        case Plane::e_accept_drag::motion: {
          // In this case the plane says that it wants to
          // receive the events, but just as normal mouse
          // move/click events.
          drag_state_.reset();
          drag_state_.emplace();
          drag_state_->plane = plane;
          drag_state_->mode  = e_drag_send_mode::motion;
          auto motion = input::drag_event_to_mouse_motion_event(
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
          return e_input_handled::yes;
        }
        case Plane::e_accept_drag::swallow:
          drag_state_.reset();
          // The plane says that it doesn't want to handle it
          // AND it doesn't want anyone else to handle it.
          return e_input_handled::yes;
        case Plane::e_accept_drag::yes:
          // Wants to handle it.
          drag_state_.reset();
          drag_state_.emplace();
          drag_state_->plane = plane;
          drag_state_->mode  = e_drag_send_mode::normal;
          // Now we must send it an on_drag because this mouse
          // event that we're dealing with serves both to tell
          // us about a new drag even but also may have a mouse
          // delta in it that needs to be processed.
          plane->on_drag( drag_event->mod, drag_event->button,
                          drag_event->state.origin,
                          drag_event->prev, drag_event->pos );
          return e_input_handled::yes;
        case Plane::e_accept_drag::yes_but_raw:
          drag_state_.reset();
          drag_state_.emplace();
          drag_state_->plane = plane;
          drag_state_->mode  = e_drag_send_mode::raw;
          (void)plane->input( event );
          return e_input_handled::yes;
      }
    }
  }
  // If no one handled it then that's it.
  return e_input_handled::no;
}

} // namespace rn
