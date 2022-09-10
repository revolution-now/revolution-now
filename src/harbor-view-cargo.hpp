/****************************************************************
**harbor-view-cargo.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-09.
*
* Description: Cargo UI element within the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "harbor-view-entities.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborCargo
*****************************************************************/
struct HarborCargo : public ui::View, public HarborSubView {
  static PositionedHarborSubView create( SS& ss, TS& ts,
                                         Player& player,
                                         Rect    canvas );

  HarborCargo( SS& ss, TS& ts, Player& player );

  // Implement ui::Object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View&       view() noexcept override;
  ui::View const& view() const noexcept override;

  maybe<DraggableObjectWithBounds> object_here(
      Coord const& where ) const override;

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

 private:
  maybe<UnitId> get_active_unit() const;

  maybe<HarborDraggableObject_t> draggable_in_cargo_slot(
      int slot ) const;

  struct Draggable {
    int slot = {};
  };

  maybe<Draggable> dragging_;
};

} // namespace rn
