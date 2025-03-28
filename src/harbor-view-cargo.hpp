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
#include "drag-drop.hpp"
#include "harbor-view-entities.hpp"

// C++ standard library
#include <array>

namespace rn {

struct SS;
struct TS;
struct Player;
struct HarborStatusBar;

/****************************************************************
** HarborCargo
*****************************************************************/
struct HarborCargo
  : public ui::View,
    public HarborSubView,
    public IDragSource<HarborDraggableObject>,
    public IDragSourceUserEdit<HarborDraggableObject>,
    public IDragSink<HarborDraggableObject> {
  struct Layout {
    gfx::rect view;
    // Relative to view nw.
    gfx::point cargohold_nw;
    std::array<gfx::rect, 6> slots;
    int drag_box_buffer = {};
    std::array<gfx::rect, 6> slot_drag_boxes;
    // Gives the rect that covers the pixel area occupied by the
    // dividing wall to the left of a slot. This is needed so
    // that said walls can be removed when we have an overflow
    // slot. Note that the first slot does not have a left di-
    // viding wall so is not relevant/populated.
    std::array<gfx::rect, 6> left_wall;
  };

  static PositionedHarborSubView<HarborCargo> create(
      SS& ss, TS& ts, Player& player, gfx::rect canvas,
      HarborStatusBar& harbor_status_bar );

  HarborCargo( SS& ss, TS& ts, Player& player,
               HarborStatusBar& harbor_status_bar,
               Layout const& layout );

  // Implement ui::Object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View& view() noexcept override;
  ui::View const& view() const noexcept override;

  maybe<DraggableObjectWithBounds<HarborDraggableObject>>
  object_here( Coord const& where ) const override;

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  // Implement IDragSource.
  bool try_drag( HarborDraggableObject const& a,
                 Coord const& where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSourceUserEdit.
  wait<maybe<HarborDraggableObject>> user_edit_object()
      const override;

  // Implement IDragSource.
  wait<> disown_dragged_object() override;

  // Impelement IDragSink.
  maybe<CanReceiveDraggable<HarborDraggableObject>> can_receive(
      HarborDraggableObject const& a, int from_entity,
      Coord const& where ) const override;

  // Impelement IDragSink.
  wait<> drop( HarborDraggableObject const& a,
               Coord const& where ) override;

 private:
  static Layout create_layout( gfx::rect canvas );

  // For managing the status bar.
  void send_sell_info_to_status_bar( e_commodity comm );
  void clear_status_bar_msg() const;

  maybe<UnitId> get_active_unit() const;

  maybe<HarborDraggableObject> draggable_in_cargo_slot(
      int slot ) const;

  maybe<int> slot_under_cursor( Coord where ) const;

  struct Draggable {
    int slot = {};
    maybe<int> quantity; // if it's a commodity.
  };

  maybe<Draggable> dragging_;
  HarborStatusBar& harbor_status_bar_;
  Layout const layout_;
};

} // namespace rn
