/****************************************************************
**harbor-view-inbound.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Inbound ships UI element within the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "drag-drop.hpp"
#include "harbor-view-entities.hpp"
#include "wait.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;
struct HarborOutboundShips;

/****************************************************************
** HarborInboundShips
*****************************************************************/
struct HarborInboundShips
  : public ui::View,
    public HarborSubView,
    public IDragSource<HarborDraggableObject>,
    public IDragSink<HarborDraggableObject> {
  static PositionedHarborSubView<HarborInboundShips> create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      HarborOutboundShips const& outbound_ships );

  struct Layout {
    // Absolute coordinates.
    gfx::rect view = {}; // includes full white box.

    // Relative to origin of view.
    bool compact         = {};
    gfx::rect units_area = {};
    int label_top        = {};
    std::vector<gfx::rect> slots;
  };

  HarborInboundShips( SS& ss, TS& ts, Player& player,
                      Layout layout );

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

  // Implement ui::AwaitView.
  virtual wait<> perform_click(
      input::mouse_button_event_t const& ) override;

  // Implement IDragSource.
  bool try_drag( HarborDraggableObject const& a,
                 Coord const& where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSource.
  wait<> disown_dragged_object() override;

  // Impelement IDragSink.
  maybe<CanReceiveDraggable<HarborDraggableObject>> can_receive(
      HarborDraggableObject const& a, int from_entity,
      Coord const& where ) const override;

  // Impelement IDragSink.
  wait<> drop( HarborDraggableObject const& a,
               Coord const& where ) override;

  // In absolute coordinates.
  gfx::point frame_nw() const;

 private:
  struct UnitWithPosition {
    UnitId id;
    gfx::rect bounds;
  };

  // The coord is relative to the upper left of this view.
  maybe<UnitWithPosition> unit_at_location( Coord where ) const;

  std::vector<UnitWithPosition> units() const;

  maybe<UnitId> get_active_unit() const;
  void set_active_unit( UnitId unit_id );

  wait<> click_on_unit( UnitId unit_id );

  static Layout create_layout(
      gfx::rect canvas,
      HarborOutboundShips const& outbound_ships );

  struct Draggable {
    UnitId unit_id = {};
  };

  maybe<Draggable> dragging_;
  Layout const layout_;
};

} // namespace rn
