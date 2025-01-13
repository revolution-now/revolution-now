/****************************************************************
**harbor-view-dock.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Units on dock UI element within the harbor view.
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
struct DockUnitsLayout;
struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborDockUnits
*****************************************************************/
struct HarborDockUnits
  : public ui::View,
    public HarborSubView,
    public IDragSource<HarborDraggableObject>,
    public IDragSink<HarborDraggableObject> {
  static PositionedHarborSubView<HarborDockUnits> create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      HarborBackdrop const& backdrop );

  struct Layout {
    // Absolute coords.
    gfx::rect view;
  };

  HarborDockUnits( SS& ss, TS& ts, Player& player,
                   HarborBackdrop const& backdrop,
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
  wait<> perform_click(
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

  // Should be called every time something happens that changes
  // the units on the dock. To be safe, we just call it on scope
  // exit of any non-const method.
  void update_units();

 private:
  static Layout create_layout( HarborBackdrop const& backdrop );

  struct UnitWithRect {
    UnitId id;
    gfx::rect bounds;
  };

  // The coord is relative to the upper left of this view.
  maybe<UnitWithRect> unit_at_location( gfx::point where ) const;

  wait<> click_on_unit( UnitId unit_id );

  struct Draggable {
    UnitId unit_id = {};
  };
  maybe<Draggable> dragging_;

  HarborBackdrop const& backdrop_;
  Layout const layout_;
  std::vector<UnitWithRect> units_;
};

} // namespace rn
