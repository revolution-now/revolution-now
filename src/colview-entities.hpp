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
#include "tx.hpp"
#include "view.hpp"

// Rnl
#include "rnl/colview-entities.hpp"

namespace rn {

struct ColViewObjectUnderCursor {
  ColViewDraggableObject_t obj;
  // Bounds of the above object as if cursor coord were origin.
  Rect bounds;
};

enum class e_colview_entity {
  commodities,
  land,
  title_bar,
  population,
  cargo,
  production
};

class ColViewEntityView : public ui::View {
public:
  ColViewEntityView( ColonyId id ) : id_( id ) {}
  virtual ~ColViewEntityView() = default;

  ColViewEntityView( ColViewEntityView const& ) = delete;
  ColViewEntityView& operator=( ColViewEntityView const& ) =
      delete;

  // Implement ui::Object
  void children_under_coord( Coord, ObjectSet& ) override {}

  virtual e_colview_entity entity_id() const = 0;

  ColonyId colony_id() const { return id_; }

  // Coordinate will be relative to the upper-left of the view.
  // Should only be called if the coord is within the bounds of
  // the view.
  virtual maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const = 0;

private:
  ColonyId id_;
};

// The pointer returned will be invalidated if set_colview_colony
// is called with a new colony id.
ColViewEntityView const* colview_entity(
    e_colview_entity entity );

ui::View const* colview_top_level();

void set_colview_colony( ColonyId id );

} // namespace rn
