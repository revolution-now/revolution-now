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
#include "plane.hpp"

// render
#include "render/renderer.hpp"

// base
#include "base/range-lite.hpp"

using namespace std;

namespace rn {

namespace {

PlaneStack g_plane_stack;

} // namespace

/****************************************************************
** Planes
*****************************************************************/
void Planes::push( Plane& plane, e_plane_stack where ) {
  switch( where ) {
    case e_plane_stack::back:
      planes_.push_back( &plane );
      return;
    case e_plane_stack::front:
      planes_.insert( planes_.begin(), &plane );
      return;
  }
}

void Planes::pop( e_plane_stack where ) {
  DCHECK( planes_.size() > 0 );
  switch( where ) {
    case e_plane_stack::back: //
      planes_.pop_back();
      return;
    case e_plane_stack::front:
      planes_.erase( planes_.begin() );
      return;
  }
}

/****************************************************************
** PlaneStack
*****************************************************************/
PlaneStack::PlaneStack()
  : groups_( refl::enum_count<e_plane_stack_level> ) {}

PlaneStack& PlaneStack::global() { return g_plane_stack; }

Planes& PlaneStack::operator[]( e_plane_stack_level level ) {
  int n = static_cast<int>( level );
  CHECK_GE( n, 0 );
  CHECK_LT( n, int( groups_.size() ) );
  return groups_[n];
}

// This will find the last plane that will render (opaquely) over
// every pixel. If one is found then we will not render any
// planes before it. This is technically not necessary, but saves
// rendering work by avoiding to render things that would go un-
// seen anyway.
vector<Plane*> PlaneStack::relevant() {
  vector<Plane*> flat;
  flat.reserve( 10 );
  int i = 0, first_relevant = 0;
  for( Planes& planes : groups_ ) {
    for( Plane* plane : planes.all() ) {
      flat.push_back( plane );
      if( plane->covers_screen() ) first_relevant = i;
      ++i;
    }
  }
  if( !flat.empty() ) {
    DCHECK( first_relevant >= 0 );
    CHECK_LT( first_relevant, int( flat.size() ) );
    flat.erase( flat.begin(), flat.begin() + first_relevant );
  }
  return flat;
}

void PlaneStack::draw( rr::Renderer& renderer ) {
  renderer.clear_screen( gfx::pixel::black() );
  for( Plane* plane : relevant() ) plane->draw( renderer );
}

void PlaneStack::advance_state() {
  for( Plane* plane : relevant() ) plane->advance_state();
}

e_input_handled PlaneStack::send_input(
    input::event_t const& event ) {
  using namespace input;
  auto* drag_event =
      std::get_if<input::mouse_drag_event_t>( &event );
  if( drag_event == nullptr ) {
    vector<Plane*> planes = relevant();
    // Normal event, so send it out using the usual protocol.
    for( Plane* plane : base::rl::rall( planes ) ) {
      switch( plane->input( event ) ) {
        case e_input_handled::yes: return e_input_handled::yes;
        case e_input_handled::no: break;
      }
    }
    return e_input_handled::no;
  }

  // We have a drag event. Test if there is a plane registered to
  // receive it already.
  if( drag_state_ ) {
    CHECK( drag_state_->plane != nullptr );
    Plane& plane = *drag_state_->plane;
    // Drag should already be in progress.
    CHECK( drag_event->state.phase != e_drag_phase::begin );
    switch( drag_state_->mode ) {
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
      drag_state_.reset();
    // Here it is assumed/required that the plane handle it be-
    // cause the plane has already accepted this drag.
    return e_input_handled::yes;
  }

  // No drag plane registered to accept the event, so lets send
  // out the event but only if it's a `begin` event.
  if( drag_event->state.phase == e_drag_phase::begin ) {
    vector<Plane*> planes = relevant();
    for( Plane* plane : base::rl::rall( planes ) ) {
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
          drag_state_.reset();
          drag_state_.emplace();
          drag_state_->plane = plane;
          drag_state_->mode  = e_drag_send_mode::motion;
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
          return e_input_handled::yes;
        }
        case Plane::e_accept_drag::swallow:
          drag_state_.reset();
          // The plane says that it doesn't want to handle it AND
          // it doesn't want anyone else to handle it.
          return e_input_handled::yes;
        case Plane::e_accept_drag::yes:
          // Wants to handle it.
          drag_state_.reset();
          drag_state_.emplace();
          drag_state_->plane = plane;
          drag_state_->mode  = e_drag_send_mode::normal;
          // Now we must send it an on_drag because this mouse
          // event that we're dealing with serves both to tell us
          // about a new drag even but also may have a mouse
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
