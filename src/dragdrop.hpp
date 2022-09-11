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
#include "co-combinator.hpp"
#include "input.hpp"
#include "wait.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/attributes.hpp"
#include "base/error.hpp"
#include "base/function-ref.hpp"

// C++ standard library
#include <any>

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

struct DragState {
  std::any                object;
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
// FIXME: find a way to get rid of the std::any in this inter-
// face, it causes various problems and is very error prone.
// Maybe the interface can be made into templates and the algo-
// rithm can be given a bunch of callbacks to use to call the
// methods. Currently the framework is set up so that it accepts
// an arbitrary object from the user and holds it, then gives it
// back. But maybe this is not necessary. Maybe the individual
// sink/source classes can hold their own info. ??

// Interface for views that support prompting a user for informa-
// tion on the parameters of a drag.
struct IDragSourceUserInput {
  virtual ~IDragSourceUserInput() = default;

  // This will only be called if the user requests it, typically
  // by holding down a modifier key such as shift when releasing
  // the drag. Using this method, the view has the opportunity to
  // edit the dragged object before it is proposed to the drag
  // target view. If it edits it, then it will update its in-
  // ternal record. Either way, it will return the current object
  // being dragged after editing (which could be unchanged). If
  // it returns a nullptr then the drag is considered to be can-
  // celled. FIXME: we should be returning a wait<maybe<any>>
  // here, but that causes strange compile errors where its con-
  // cept constraints end up being circular (same with std::op-
  // tional, and same with wait<std::any> since it internally
  // will create a maybe<std::any>). So we will wrap the std::any
  // in a unique_ptr in order to prevent that error, and will use
  // unique_ptr's nullptr state to signify a non-result, but need
  // to remember to check it.
  virtual wait<std::unique_ptr<std::any>> user_edit_object()
      const = 0;
};

// Interface for drag sources that can/might need to do some fur-
// ther checks before the drag is greenlighted that require some
// user input or message boxes. E.g., if the player is trying to
// buy a commodity from europe but they don't have enough money
// and we want to pop up a message box telling them that before
// rejecting it.
struct IDragSourceCheck {
  virtual ~IDragSourceCheck() = default;

  virtual wait<base::valid_or<DragRejection>> source_check(
      std::any const& a, Coord const ) const = 0;
};

// Interface for views that can be the source for dragging. The
// idea here is that the view must keep track of what is being
// dragged.
struct IDragSource {
  virtual ~IDragSource() = default;
  // Decides whether a drag can happen on the given object. Note
  // that in some cases the coordinate parameter will not be
  // needed. If this returns true, this means that the view has
  // recorded the object and assumes that the drag has begun.
  // Note: this function may be called when a drag is already in
  // progress in order to adjust the object being dragged.
  virtual bool try_drag( std::any const& o,
                         Coord const&    where ) = 0;

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
  maybe<IDragSourceUserInput const&> drag_user_input() const;

  maybe<IDragSourceCheck const&> drag_check() const;

  // This will permanently remove the object from the source
  // view's ownership, so should only be called just before the
  // drop is to take effect.
  virtual void disown_dragged_object() = 0;

  // Optional. After a successful drag this will be called to do
  // anything that needs to be done on the source side post-drag.
  virtual wait<> post_successful_source( std::any const&,
                                         Coord const& );
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
struct IDragSinkCheck {
  virtual ~IDragSinkCheck() = default;

  virtual wait<base::valid_or<DragRejection>> sink_check(
      std::any const&, int from_entity, Coord const ) const = 0;
};

// Interface for views that can accept dragged items.
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
  virtual maybe<std::any> can_receive(
      std::any const& o, int from_entity,
      Coord const& where ) const = 0;

  maybe<IDragSinkCheck const&> drag_check() const;

  // Coordinates are relative to view's upper left corner. The
  // sink MUST accept the object as-is.
  virtual void drop( std::any const& o, Coord const& where ) = 0;

  // Optional. After a successful drag this will be called to do
  // anything that needs to be done on the sink side post-drag.
  virtual wait<> post_successful_sink( std::any const&,
                                       int /*from_entity*/,
                                       Coord const& );
};

struct IDraggableObjectsView;

struct DraggableObjectWithBounds {
  std::any obj;
  Rect     bounds;
};

struct PositionedDraggableSubView {
  IDraggableObjectsView* drag_view;
  Coord                  upper_left;
};

// Interface for a view that has objects within it that can po-
// tentially be dragged. This interface acts as a kind of entry
// point into the view to extract references to the above more
// specific interfaces.
struct IDraggableObjectsView {
  virtual ~IDraggableObjectsView() = default;

  virtual maybe<int> entity() const = 0;

  // Coordinate will be relative to the upper-left of the view.
  // Should only be called if the coord is within the bounds of
  // the view.
  virtual maybe<PositionedDraggableSubView> view_here(
      Coord ) = 0;

  virtual maybe<DraggableObjectWithBounds> object_here(
      Coord const& where ) const = 0;

  // For convenience.
  maybe<IDragSource&> drag_source();
  maybe<IDragSink&>   drag_sink();
};

/****************************************************************
** Public API
*****************************************************************/
using ObjectStringifier =
    base::function_ref<std::string( std::any const& )>;
using EntityStringifier = base::function_ref<std::string( int )>;

wait<> drag_drop_routine( co::stream<input::event_t>& input,
                          IDraggableObjectsView&      top_view,
                          maybe<DragState>&           drag_state,
                          IGui&                       gui,
                          input::mouse_drag_event_t const& event,
                          ObjectStringifier obj_str_func );

} // namespace rn
