/****************************************************************
**colview-buildings.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Buildings view UI within the colony view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colview-entities.hpp"

// gs
#include "ss/colony-enums.rds.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

/****************************************************************
** Buildings
*****************************************************************/
class ColViewBuildings : public ui::View,
                         public ColonySubView,
                         public IDragSink<ColViewObject_t>,
                         public IDragSinkCheck<ColViewObject_t>,
                         public IDragSource<ColViewObject_t> {
 public:
  static std::unique_ptr<ColViewBuildings> create(
      SS& ss, TS& ts, Colony& colony, Delta size,
      Player const& player ) {
    return std::make_unique<ColViewBuildings>( ss, ts, colony,
                                               size, player );
  }

  ColViewBuildings( SS& ss, TS& ts, Colony& colony, Delta size,
                    Player const& player )
    : ColonySubView( ss, ts, colony ),
      size_( size ),
      colony_( colony ),
      player_( player ) {}

  Delta delta() const override { return size_; }

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::buildings );
  }

  // Implement ColonySubView.
  ui::View& view() noexcept override { return *this; }

  // Implement ColonySubView.
  ui::View const& view() const noexcept override {
    return *this;
  }

  // Implement IDragSink.
  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, int from_entity,
      Coord const& where ) const override;

  // Implement IDragSink.
  wait<> drop( ColViewObject_t const& o,
               Coord const&           where ) override;

  // Implement IDragSinkCheck.
  wait<base::valid_or<DragRejection>> sink_check(
      ColViewObject_t const&, int from_entity,
      Coord const ) const override;

  // Implement IDragSource.
  bool try_drag( ColViewObject_t const& o,
                 Coord const&           where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSource.
  void disown_dragged_object() override;

  // Implement ColonySubView.
  maybe<DraggableObjectWithBounds<ColViewObject_t>> object_here(
      Coord const& /*where*/ ) const override;

  // Implement AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override;

 private:
  Rect rect_for_slot( e_colony_building_slot slot ) const;

  Rect visible_rect_for_unit_in_slot(
      e_colony_building_slot slot, int unit_idx ) const;

  Rect sprite_rect_for_unit_in_slot( e_colony_building_slot slot,
                                     int unit_idx ) const;

  maybe<e_colony_building_slot> slot_for_coord(
      Coord where ) const;

 private:
  struct Dragging {
    UnitId                 id   = {};
    e_colony_building_slot slot = {};
  };

  Delta           size_;
  Colony&         colony_;
  Player const&   player_;
  maybe<Dragging> dragging_ = {};
};

} // namespace rn
