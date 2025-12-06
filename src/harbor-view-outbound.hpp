/****************************************************************
**harbor-view-outbound.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Outbound ships UI element within the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "drag-drop.hpp"
#include "harbor-view-entities.hpp"
#include "wait.hpp"

namespace rn {

struct HarborInPortShips;
struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborOutboundShips
*****************************************************************/
struct HarborOutboundShips
  : public ui::View,
    public HarborSubView,
    public IDragSource<HarborDraggableObject>,
    public IDragSink<HarborDraggableObject>,
    public IDragSinkCheck<HarborDraggableObject> {
  static PositionedHarborSubView<HarborOutboundShips> create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      HarborInPortShips const& in_port_ships );

  struct Layout {
    // Absolute coordinates.
    gfx::rect view = {}; // includes full white box.

    // Relative to origin of view.
    bool compact         = {};
    gfx::rect units_area = {};
    int label_top        = {};
    std::vector<gfx::rect> slots;
  };

  HarborOutboundShips( SS& ss, TS& ts, Player& player,
                       Layout layout );

 public: // ui::object.
  Delta delta() const override;

  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

 public: // IDraggableObjectsView
  maybe<int> entity() const override;

  maybe<DraggableObjectWithBounds<HarborDraggableObject>>
  object_here( Coord const& where ) const override;

 public: // HarborSubView
  ui::View& view() noexcept override;
  ui::View const& view() const noexcept override;

 public: // ui::AwaitView
  virtual wait<> perform_click(
      input::mouse_button_event_t const& ) override;

 public: // IDragSource
  bool try_drag( HarborDraggableObject const& a,
                 Coord const& where ) override;

  void cancel_drag() override;

  wait<> disown_dragged_object() override;

 public: // IDragSink
  maybe<CanReceiveDraggable<HarborDraggableObject>> can_receive(
      HarborDraggableObject const& a, int from_entity,
      Coord const& where ) const override;

  wait<> drop( HarborDraggableObject const& a,
               Coord const& where ) override;

  wait<> post_successful_sink( HarborDraggableObject const& o,
                               int from_entity,
                               Coord const& where ) override;

 public: // IDragSinkCheck.
  wait<base::valid_or<DragRejection>> sink_check(
      HarborDraggableObject const&, int from_entity,
      Coord const ) override;

  // In absolute coordinates.
  gfx::point frame_nw() const;

 private:
  struct UnitWithPosition {
    UnitId id = {};
    gfx::rect bounds;
  };

  // The coord is relative to the upper left of this view.
  maybe<UnitWithPosition> unit_at_location( Coord where ) const;

  std::vector<UnitWithPosition> units() const;

  maybe<UnitId> get_active_unit() const;
  void set_active_unit( UnitId unit_id );

  wait<> click_on_unit( UnitId unit_id );

  static Layout create_layout(
      gfx::rect canvas, HarborInPortShips const& in_port_ships );

  struct Draggable {
    UnitId unit_id = {};
  };

  maybe<Draggable> dragging_;
  Layout const layout_;
};

} // namespace rn
