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

struct ColViewObjectUnderCursor {
  ColViewDraggableObject_t obj;
  // Bounds of the above object as if cursor coord were origin.
  Rect bounds;
};

class ColonySubView : public AwaitableView {
public:
  ColonySubView() = default;

  // virtual e_colview_entity entity_id() const = 0;

  // Coordinate will be relative to the upper-left of the view.
  // Should only be called if the coord is within the bounds of
  // the view.
  virtual maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const = 0;

private:
  ColonyId id_;
};

struct ColViewEntityPtrs {
  // These two pointers refer to the same object, just dynami-
  // cally casted to different parents in the sense of
  // multiple-inheritance.
  ColonySubView* col_view;
  ui::View*      view;
};

// The pointer returned from these will be invalidated if
// set_colview_colony is called with a new colony id.
ColViewEntityPtrs colview_entity( e_colview_entity entity );
ColViewEntityPtrs colview_top_level();

// Must be called before any other method in this module.
void set_colview_colony( ColonyId id );

} // namespace rn
