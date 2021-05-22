/****************************************************************
**colview-entities.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-12.
*
* Description: The various UI sections/entities in Colony view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "cargo.hpp"
#include "commodity.hpp"
#include "coord.hpp"
#include "id.hpp"
#include "view.hpp"
#include "waitable.hpp"

// Rnl
#include "rnl/colview-entities.hpp"

namespace rn {

// TODO: Keep this generic and move it into the ui namespace
// eventually.
struct AwaitableView {
  virtual ~AwaitableView() = default;

  virtual waitable<> perform_click( Coord ) {
    return make_waitable<>();
  }
};

class ColonySubView;

struct PositionedColSubView {
  ColonySubView* col_view;
  Coord          upper_left;
};

struct ColViewObjectWithBounds {
  ColViewObject_t obj;
  Rect            bounds;
};

struct IColViewDragSource {
protected:
  struct [[nodiscard]] ScopedDragCanceller {
    ScopedDragCanceller( IColViewDragSource* src_ )
      : src( src_ ) {}
    ~ScopedDragCanceller() noexcept { src->cancel_drag(); }
    IColViewDragSource* src;
  };

  virtual void cancel_drag() = 0;

public:
  virtual maybe<ScopedDragCanceller> try_drag(
      ColViewObject_t const& o ) = 0;
};

struct IColViewDragSink {
  // Coordinates are relative to view's upper left corner.
  virtual bool can_receive( ColViewObject_t const& o,
                            Coord const&           where ) = 0;
  // Coordinates are relative to view's upper left corner. Re-
  // turns true if the drop succeeded.
  virtual waitable<bool> drop( ColViewObject_t const& o,
                               Coord const& where ) = 0;
};

class ColonySubView : public AwaitableView {
public:
  ColonySubView() = default;

  // virtual e_colview_entity entity_id() const = 0;

  // All ColonySubView's will also be unspecified subclassess of
  // ui::View.
  virtual ui::View&       view() noexcept       = 0;
  virtual ui::View const& view() const noexcept = 0;

  // Coordinate will be relative to the upper-left of the view.
  // Should only be called if the coord is within the bounds of
  // the view.
  virtual maybe<PositionedColSubView> view_here( Coord ) {
    return PositionedColSubView{ this, Coord{} };
  }

  virtual maybe<ColViewObjectWithBounds> object_here(
      Coord const& /*where*/ ) const {
    return nothing;
  }

  // For convenience.
  maybe<IColViewDragSource&> drag_source();
  maybe<IColViewDragSink&>   drag_sink();

private:
  ColonyId id_;
};

// The pointer returned from these will be invalidated if
// set_colview_colony is called with a new colony id.
ColonySubView& colview_entity( e_colview_entity entity );
ColonySubView& colview_top_level();

// Must be called before any other method in this module.
void set_colview_colony( ColonyId id );

} // namespace rn
