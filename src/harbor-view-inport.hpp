/****************************************************************
**harbor-view-inport.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: In-port ships UI element within the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "drag-drop.hpp"
#include "harbor-view-entities.hpp"
#include "wait.hpp"

namespace rn {

struct HarborBackdrop;
struct HarborMarketCommodities;
struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborInPortShips
*****************************************************************/
struct HarborInPortShips
  : public ui::View,
    public HarborSubView,
    public IDragSource<HarborDraggableObject>,
    public IDragSourceCheck<HarborDraggableObject>,
    public IDragSink<HarborDraggableObject> {
  static PositionedHarborSubView<HarborInPortShips> create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      HarborBackdrop const& backdrop,
      HarborMarketCommodities& harbor_market_commodities );

  struct Layout {
    // Absolute coordinates.
    gfx::rect view = {}; // includes only the units.

    // Relative to origin of view.
    gfx::rect white_box  = {}; // larger than view.
    gfx::rect label_area = {};
    std::vector<gfx::rect> slots;
  };

  HarborInPortShips(
      SS& ss, TS& ts, Player& player,
      HarborMarketCommodities& harbor_market_commodities,
      Layout layout );

  // Implement ui::object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View& view() noexcept override;
  ui::View const& view() const noexcept override;

  maybe<DraggableObjectWithBounds<HarborDraggableObject>>
  object_here( Coord const& where ) const override;

  // Implement ui::object.
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  // Implement ui::AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& ) override;

  // Implement IDragSource.
  bool try_drag( HarborDraggableObject const& a,
                 Coord const& where ) override;

  // Implement IDragSourceCheck.
  wait<base::valid_or<DragRejection>> source_check(
      HarborDraggableObject const& a, Coord const ) override;

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
    UnitId id = {};
    gfx::rect bounds;
  };

  // The coord is relative to the upper left of this view.
  maybe<UnitWithPosition> unit_at_location( Coord where ) const;

  static Layout create_layout( HarborBackdrop const& backdrop );

  std::vector<UnitWithPosition> units() const;

  maybe<UnitId> get_active_unit() const;
  void set_active_unit( UnitId unit_id );

  wait<> click_on_unit( UnitId unit_id );

  struct Draggable {
    UnitId unit_id = {};
  };

  HarborMarketCommodities& harbor_market_commodities_;
  maybe<Draggable> dragging_;
  Layout const layout_;
};

} // namespace rn
