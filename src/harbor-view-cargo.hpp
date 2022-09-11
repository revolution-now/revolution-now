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
#include "dragdrop.hpp"
#include "harbor-view-entities.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborCargo
*****************************************************************/
struct HarborCargo : public ui::View,
                     public HarborSubView,
                     public IDragSource,
                     public IDragSourceUserInput,
                     public IDragSink {
  static PositionedHarborSubView<HarborCargo> create(
      SS& ss, TS& ts, Player& player, Rect canvas );

  HarborCargo( SS& ss, TS& ts, Player& player );

  // Implement ui::Object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View&       view() noexcept override;
  ui::View const& view() const noexcept override;

  maybe<DraggableObjectWithBounds> object_here(
      Coord const& where ) const override;

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  // Implement IDragSource.
  bool try_drag( std::any const& a,
                 Coord const&    where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSourceUserInput.
  wait<std::unique_ptr<std::any>> user_edit_object()
      const override;

  // Implement IDragSource.
  void disown_dragged_object() override;

  // Impelement IDragSink.
  maybe<std::any> can_receive(
      std::any const& a, int from_entity,
      Coord const& where ) const override;

  // Impelement IDragSink.
  void drop( std::any const& a, Coord const& where ) override;

 private:
  maybe<UnitId> get_active_unit() const;

  maybe<HarborDraggableObject_t> draggable_in_cargo_slot(
      int slot ) const;

  maybe<int> slot_under_cursor( Coord where ) const;

  struct Draggable {
    int        slot = {};
    maybe<int> quantity; // if it's a commodity.
  };

  maybe<Draggable> dragging_;
};

} // namespace rn
