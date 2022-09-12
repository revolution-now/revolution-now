/****************************************************************
**dragdrop.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-20.
*
* Description: A framework for drag & drop of entities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "anim.hpp"
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "igui.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "variant.hpp"
#include "wait.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/attributes.hpp"
#include "base/error.hpp"
#include "base/function-ref.hpp"
#include "base/maybe-util.hpp"
#include "base/scope-exit.hpp"

namespace rn {

struct IGui;
struct SS;

/****************************************************************
** Dragging State
*****************************************************************/
enum class e_drag_status_indicator { none, bad, good };

// FIXME: needed?
struct DragStep {
  input::mod_keys mod;
  Coord           current;
};

template<typename Draggable>
struct DragState {
  Draggable               object;
  e_drag_status_indicator indicator;
  bool                    user_requests_input;
  Coord                   where;
  Delta                   click_offset;
};

struct DragRejection {
  maybe<std::string> reason;
};

/****************************************************************
** Drag/Drop Interfaces
*****************************************************************/
// Interface for views that support prompting a user for informa-
// tion on the parameters of a drag.
template<typename Draggable>
struct IDragSourceUserInput {
  virtual ~IDragSourceUserInput() = default;

  // This will only be called if the user requests it, typically
  // by holding down a modifier key such as shift when releasing
  // the drag. Using this method, the view has the opportunity to
  // edit the dragged object before it is proposed to the drag
  // target view. If it edits it, then it will update its in-
  // ternal record. Either way, it will return the current object
  // being dragged after editing (which could be unchanged). If
  // it returns nothing then the drag is considered to be can-
  // celled.
  virtual wait<maybe<Draggable>> user_edit_object() const = 0;
};

// Interface for drag sources that can/might need to do some fur-
// ther checks before the drag is greenlighted that require some
// user input or message boxes. E.g., if the player is trying to
// buy a commodity from europe but they don't have enough money
// and we want to pop up a message box telling them that before
// rejecting it.
template<typename Draggable>
struct IDragSourceCheck {
  virtual ~IDragSourceCheck() = default;

  virtual wait<base::valid_or<DragRejection>> source_check(
      Draggable const& a, Coord const ) const = 0;
};

// Interface for views that can be the source for dragging. The
// idea here is that the view must keep track of what is being
// dragged.
template<typename Draggable>
struct IDragSource {
  virtual ~IDragSource() = default;
  // Decides whether a drag can happen on the given object. Note
  // that in some cases the coordinate parameter will not be
  // needed. If this returns true, this means that the view has
  // recorded the object and assumes that the drag has begun.
  // Note: this function may be called when a drag is already in
  // progress in order to adjust the object being dragged.
  virtual bool try_drag( Draggable const& o,
                         Coord const&     where ) = 0;

  // This function must be called if the drag is cancelled for
  // any reason before it is affected. It is recommended to call
  // this function in a SCOPE_EXIT just after calling try_drag.
  // Note that an implementation of this MUST be idempotent,
  // since this may be called more than once.
  virtual void cancel_drag() = 0;

  // This is used to indicate whether the user can hold down a
  // modifier key while releasing the drag to signal that they
  // wish to be prompted and asked for information to customize
  // the drag, such as e.g. specifying the amount of a commodity
  // to be dragged.
  maybe<IDragSourceUserInput<Draggable> const&> drag_user_input()
      const {
    return base::maybe_dynamic_cast<
        IDragSourceUserInput<Draggable> const&>( *this );
  }

  maybe<IDragSourceCheck<Draggable> const&> drag_check() const {
    return base::maybe_dynamic_cast<
        IDragSourceCheck<Draggable> const&>( *this );
  }

  // This will permanently remove the object from the source
  // view's ownership, so should only be called just before the
  // drop is to take effect.
  virtual wait<> disown_dragged_object() = 0;

  // Optional. After a successful drag this will be called to do
  // anything that needs to be done on the source side post-drag.
  virtual wait<> post_successful_source( Draggable const&,
                                         Coord const& ) {
    co_return;
  }
};

// Interface for drag targets that can/might need to do some fur-
// ther checks before the drag is greenlighted that require some
// user input or message boxes. Two possibilities for this would
// be that it needs to ask the user for a final confirmation, or
// it might check some further game logic that could cause the
// drag to be cancelled in a way that would require showing a
// message to the user. An example of the latter case would be
// dragging a unit over a water tile in a colony that does not
// contain docks; we want to all the drag UI-wise, but we want to
// then show a message to the user explaining why we are can-
// celling it. Returning `true` means "proceed".
template<typename Draggable>
struct IDragSinkCheck {
  virtual ~IDragSinkCheck() = default;

  virtual wait<base::valid_or<DragRejection>> sink_check(
      Draggable const&, int from_entity, Coord const ) const = 0;
};

// Interface for views that can accept dragged items.
template<typename Draggable>
struct IDragSink {
  virtual ~IDragSink() = default;
  // Coordinates are relative to view's upper left corner. If the
  // object can be received, then a new object will be returned
  // that is either the same or possibly altered from the origi-
  // nal, for example if the user is moving commodities into a
  // ship's cargo and the cargo only has room for half of the
  // quantity that the user is dragging, the returned object will
  // be updated to reflect the capacity of the cargo so that the
  // controller algorithm knows how much to remove from source.
  virtual maybe<Draggable> can_receive(
      Draggable const& o, int from_entity,
      Coord const& where ) const = 0;

  maybe<IDragSinkCheck<Draggable> const&> drag_check() const {
    return base::maybe_dynamic_cast<
        IDragSinkCheck<Draggable> const&>( *this );
  }

  // Coordinates are relative to view's upper left corner. The
  // sink MUST accept the object as-is.
  virtual wait<> drop( Draggable const& o,
                       Coord const&     where ) = 0;
};

template<typename Draggable>
struct IDraggableObjectsView;

template<typename Draggable>
struct DraggableObjectWithBounds {
  Draggable obj;
  Rect      bounds;
};

template<typename Draggable>
struct PositionedDraggableSubView {
  IDraggableObjectsView<Draggable>* drag_view;
  Coord                             upper_left;
};

// Interface for a view that has objects within it that can po-
// tentially be dragged. This interface acts as a kind of entry
// point into the view to extract references to the above more
// specific interfaces.
template<typename Draggable>
struct IDraggableObjectsView {
  virtual ~IDraggableObjectsView() = default;

  virtual maybe<int> entity() const = 0;

  // Coordinate will be relative to the upper-left of the view.
  // Should only be called if the coord is within the bounds of
  // the view.
  virtual maybe<PositionedDraggableSubView<Draggable>> view_here(
      Coord ) = 0;

  virtual maybe<DraggableObjectWithBounds<Draggable>>
  object_here( Coord const& where ) const = 0;

  // For convenience.
  maybe<IDragSource<Draggable>&> drag_source() {
    return base::maybe_dynamic_cast<IDragSource<Draggable>&>(
        *this );
  }

  maybe<IDragSink<Draggable>&> drag_sink() {
    return base::maybe_dynamic_cast<IDragSink<Draggable>&>(
        *this );
  }
};

/****************************************************************
** Macros
*****************************************************************/
// Must use this to exit the drag_drop_routine function prior to
// the point where the while look starts taking in new events.
// This will happen when the source is not draggable for whatever
// reason; in this case, we need to wait for and discard the re-
// mainder of the drag events before returning, which is what
// this macro does.
//
// FIXME: make this into a template function after clang learns
// to handle function templates that are coroutines.
#define NO_DRAG( ... )                                   \
  {                                                      \
    lg.debug( "cannot drag: " __VA_ARGS__ );             \
    co_await detail::eat_remaining_drag_events( input ); \
    co_return;                                           \
  }

namespace detail {

wait<> eat_remaining_drag_events(
    co::stream<input::event_t>& input );

}

/****************************************************************
** Drag/Drop Algorithm.
*****************************************************************/
template<typename Draggable>
wait<> drag_drop_routine(
    co::stream<input::event_t>&       input,
    IDraggableObjectsView<Draggable>& top_view,
    maybe<DragState<Draggable>>& drag_state, IGui& gui,
    input::mouse_drag_event_t const& event ) {
  CHECK( event.state.phase == input::e_drag_phase::begin );
  CHECK( !drag_state.has_value() );
  if( event.button != input::e_mouse_button::l )
    NO_DRAG( "wrong mouse button." );
  Coord const& origin = event.state.origin;

  // First get the view under the mouse at the drag origin.
  maybe<PositionedDraggableSubView<Draggable>>
      maybe_source_p_view = top_view.view_here( origin );
  if( !maybe_source_p_view )
    NO_DRAG( "there is no view to serve an object." );
  IDraggableObjectsView<Draggable>& source_view =
      *maybe_source_p_view->drag_view;
  Coord const& source_upper_left =
      maybe_source_p_view->upper_left;

  // Get entity ID from source view.
  maybe<int> source_entity = source_view.entity();
  if( !source_entity )
    NO_DRAG( "source view has no entity ID." );

  // Next check if there is an object under the cursor.
  maybe<DraggableObjectWithBounds<Draggable>> const
      source_bounded_object = source_view.object_here(
          origin.with_new_origin( source_upper_left ) );
  if( !source_bounded_object )
    NO_DRAG( "there is no object under the cursor." );
  Draggable source_object = source_bounded_object->obj;
  Rect      source_object_bounds =
      source_bounded_object->bounds.as_if_origin_were(
          source_upper_left );

  // Next check if source view allows dragging from that spot.
  maybe<IDragSource<Draggable>&> maybe_drag_source =
      source_view.drag_source();
  if( !maybe_drag_source )
    NO_DRAG(
        "the source view does not allow dragging in general." );
  IDragSource<Draggable>& drag_source = *maybe_drag_source;

  bool can_drag = drag_source.try_drag(
      source_object,
      origin.with_new_origin( source_upper_left ) );
  // This ensures that if the coroutine is interrupted somewhere
  // during the drag (e.g. early return, or cancellation) then
  // the source object will be told about it so that it can go
  // back to normal rendering of the dragged object.
  SCOPE_EXIT( drag_source.cancel_drag() );
  if( !can_drag )
    NO_DRAG(
        "the source view does not allow dragging object {}.",
        source_object );

  // The drag has now started.
  lg.debug( "dragging {} with bounds {}.", source_object,
            source_object_bounds );

  drag_state = DragState<Draggable>{
      .object              = source_object,
      .indicator           = e_drag_status_indicator::none,
      .user_requests_input = event.mod.shf_down,
      .where               = origin,
      .click_offset = origin - source_object_bounds.upper_left(),
  };
  SCOPE_EXIT( drag_state = nothing );

  // This can optionally be populated to be displayed as a mes-
  // sage to the user after the drag is rejected and the item is
  // rubber banded. This is because it tends to look better if
  // the item bounces back first, then the message is displayed.
  maybe<std::string> post_reject_message;

  // The first drag event also contains some motion that we want
  // to process.
  input::event_t latest    = event;
  Coord          mouse_pos = origin;
  goto have_event;

  while( true ) {
    if( auto drag = latest.get_if<input::mouse_drag_event_t>();
        drag.has_value() &&
        drag->state.phase == input::e_drag_phase::end ) {
      // The drag has ended but with no dice, so rubber-band the
      // dragged object back to its source.
      lg.debug( "drag rejected." );
      break;
    }
    latest = co_await input.next();
    // Optional sanity check.
    if( auto drag =
            latest.get_if<input::mouse_drag_event_t>() ) {
      CHECK( drag->state.phase != input::e_drag_phase::begin );
    }

    if( auto win_event = latest.get_if<input::win_event_t>();
        win_event &&
        win_event->type == input::e_win_event_type::resized ) {
      // If the window is getting resized then our coordinates
      // might get messed up. It seems safest just ot cancel the
      // drag.
      lg.warn(
          "cancelling drag operation due to window resize." );
      //
      // WARNING: do not force a re-composite here, since there
      // are SCOPE_EXIT's above that are hanging onto pointers
      // into the views, that we don't want to invalidate. FIXME:
      // to fix this, we need to have a way to re-composite
      // without invalidating the pointers to the views.
      //
      // We know that this event is not the last drag event,
      // since it is a window resize event.  So we need to eat
      // the remainder of the drag events.
      co_await detail::eat_remaining_drag_events( input );
      co_return; // no rubber banding.
    }

  have_event:
    auto drag_event = latest.get_if<input::mouse_drag_event_t>();
    if( drag_event.has_value() ) mouse_pos = drag_event->pos;
    // We allow non-drag events to pass through here so that we
    // can update the rendering of the mouse cursor in response
    // to key events, in particular when the user presses the
    // shift key to ask for user input.

    drag_state->where     = mouse_pos;
    drag_state->indicator = e_drag_status_indicator::none;
    drag_state->user_requests_input = false;

    // Check if there is a view under the current mouse pos.
    maybe<PositionedDraggableSubView<Draggable>>
        maybe_target_p_view = top_view.view_here( mouse_pos );
    if( !maybe_target_p_view ) continue;
    IDraggableObjectsView<Draggable>& target_view =
        *maybe_target_p_view->drag_view;
    Coord const& target_upper_left =
        maybe_target_p_view->upper_left;

    // Check if the target view can accept drags.
    maybe<IDragSink<Draggable>&> maybe_drag_sink =
        target_view.drag_sink();
    if( !maybe_drag_sink ) continue;
    IDragSink<Draggable>& drag_sink = *maybe_drag_sink;
    Coord                 sink_coord =
        mouse_pos.with_new_origin( target_upper_left );
    // Assume the drag won't work unless we find out otherwise.
    drag_state->indicator = e_drag_status_indicator::bad;

    // Check if the target view can receive the object that is
    // being dragged and can do so at the current mouse position.
    if( !drag_sink.can_receive( source_object, *source_entity,
                                sink_coord ) ) {
      lg.trace( "drag sink cannot accept object {}.",
                source_object );
      continue;
    }
    drag_state->indicator = e_drag_status_indicator::good;

    // Check if the user is allowed to request user input. If so,
    // then check if they are holding down shift, which is how
    // they do so.
    maybe<IDragSourceUserInput<Draggable> const&>
        drag_user_input = drag_source.drag_user_input();
    if( drag_user_input ) {
      auto const& base =
          variant_base<input::event_base_t>( latest );
      drag_state->user_requests_input = base.mod.shf_down;
    }

    // E.g. if this is a keyboard event, then we're done with it.
    if( !drag_event ) continue;

    // At this point, we have a drag event, so we need to check
    // if this is a drag end point, and if so, check to see if
    // the item can be dropped.
    if( drag_event->state.phase != input::e_drag_phase::end )
      continue;

    // *** After this point, we should only `break` as opposed
    // to `continue`, since we have already received the end drag
    // event.

    // Check if the user wants to input anything.
    if( drag_user_input.has_value() &&
        drag_state->user_requests_input ) {
      maybe<Draggable> new_obj =
          co_await drag_user_input->user_edit_object();
      if( new_obj == nothing ) {
        lg.debug( "drag of object {} cancelled by user.",
                  source_object );
        break;
      }
      source_object = *new_obj;
      lg.debug( "user requests {}.", source_object );
    }

    // Check that the target view can receive this object as it
    // currently is, and/or allow it to adjust it.
    maybe<Draggable> sink_edited = drag_sink.can_receive(
        source_object, *source_entity, sink_coord );
    if( !sink_edited ) {
      // The sink can't find a way to make it work, drag is can-
      // celled.
      lg.debug( "drag sink cannot receive object {}.",
                source_object );
      break;
    }
    lg.debug( "drag sink responded with object {}.",
              *sink_edited );
    // Ideally here we would check that the new object does not
    // have a larger quantity than the dragged object, if/where
    // that makes sense. In other words, if the user is dragging
    // 50 of a commodity, the drag sink cannot request more,
    // though it can request less. Not sure how to do that in a
    // generic way though.
    source_object = *sink_edited;

    // Since the sink may have edited the object, lets make sure
    // that the source can handle it.
    bool can_drag = drag_source.try_drag(
        source_object,
        origin.with_new_origin( source_upper_left ) );
    if( !can_drag ) {
      // The source and sink can't negotiate a way to make this
      // drag work, so cancel it.
      lg.debug(
          "drag source and sink cannot negotiate a draggable "
          "object, last attempt was {}.",
          source_object );
      break;
    }

    // The source and sink have agreed on an object that can be
    // transferred, so let's let give the source and sink each
    // one final opportunity to involve some user input to e.g.
    // either confirm the drag or to cancel it with a message box
    // explaining why, etc.

    maybe<IDragSourceCheck<Draggable> const&> drag_src_check =
        drag_source.drag_check();
    if( drag_src_check ) {
      base::valid_or<DragRejection> proceed =
          co_await drag_src_check->source_check( source_object,
                                                 sink_coord );
      if( !proceed.valid() ) {
        post_reject_message = proceed.error().reason;
        lg.debug( "drag of object {} cancelled by source.",
                  source_object );
        break;
      }
    }

    maybe<IDragSinkCheck<Draggable> const&> drag_sink_check =
        drag_sink.drag_check();
    if( drag_sink_check ) {
      base::valid_or<DragRejection> proceed =
          co_await drag_sink_check->sink_check(
              source_object, *source_entity, sink_coord );
      if( !proceed.valid() ) {
        post_reject_message = proceed.error().reason;
        lg.debug( "drag of object {} cancelled by sink.",
                  source_object );
        break;
      }
    }

    // Finally we can do the drag.
    co_await drag_source.disown_dragged_object();
    co_await drag_sink.drop( source_object, sink_coord );
    // Drag happened successfully.
    lg.debug( "drag of object {} successful.", source_object );

    // Now call the post-successful-drag hook on the source. We
    // don't have one for the sink because the sink can just use
    // the "drop" method.
    co_await drag_source.post_successful_source(
        source_object,
        origin.with_new_origin( source_upper_left ) );

    // This will ensure that the drag source clears its locally
    // held dragging state so that it doesn't have to remember to
    // do it. This is not really that necessary, but could pre-
    // vent potential errors.
    drag_source.cancel_drag();

    co_return;
  }

  // Rubber-band back to starting point.
  drag_state->indicator = e_drag_status_indicator::none;
  drag_state->user_requests_input = false;

  Coord         start   = drag_state->where;
  Coord         end     = origin;
  double        percent = 0.0;
  AnimThrottler throttle( kAlmostStandardFrame );
  while( percent <= 1.0 ) {
    co_await throttle();
    drag_state->where =
        start + ( end - start ).multiply_and_round( percent );
    if( percent >= 1.0 ) break;
    percent += 0.15;
  }

  // The following will happen anyway at scope exit, but we need
  // to do this before showing a message so that the item will go
  // back to appearing the way it was before the drag begun.
  // TODO: if this (long) function is at some point factored out
  // into smaller functions that maybe we can remove these and
  // just let the scope-exit macros do this if we place the
  // closing braces in the right spot.
  drag_state = nothing;
  drag_source.cancel_drag();

  if( post_reject_message.has_value() )
    co_await gui.message_box( "{}", *post_reject_message );
}

} // namespace rn
