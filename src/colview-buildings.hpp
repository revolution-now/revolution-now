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

// config
#include "config/colony-enums.rds.hpp" // FIXME: to be moved.

// Rds
#include "colony-buildings.rds.hpp"

namespace rn {

struct IGui;
struct Player;

/****************************************************************
** Buildings
*****************************************************************/
class ColViewBuildings : public ui::View,
                         public ColonySubView,
                         public IColViewDragSink,
                         public IColViewDragSinkCheck,
                         public IColViewDragSource {
 public:
  Delta delta() const override { return size_; }

  // Implement ColonySubView.
  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::buildings;
  }

  // Implement ColonySubView.
  ui::View& view() noexcept override { return *this; }

  // Implement ColonySubView.
  ui::View const& view() const noexcept override {
    return *this;
  }

  // Implement IColViewDragSink.
  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, e_colview_entity from,
      Coord const& where ) const override;

  // Implement IColViewDragSink.
  void drop( ColViewObject_t const& o,
             Coord const&           where ) override;

  // Implement IColViewDragSinkCheck.
  wait<base::valid_or<IColViewDragSinkCheck::Rejection>> check(
      ColViewObject_t const&, e_colview_entity from,
      Coord const ) const override;

  // Implement IColViewDragSource.
  bool try_drag( ColViewObject_t const& o,
                 Coord const&           where ) override;

  // Implement IColViewDragSource.
  void cancel_drag() override;

  // Implement IColViewDragSource.
  void disown_dragged_object() override;

  // Implement ColonySubView.
  maybe<ColViewObjectWithBounds> object_here(
      Coord const& /*where*/ ) const override;

 private:
  Rect rect_for_slot( e_colony_building_slot slot ) const;

  Rect visible_rect_for_unit_in_slot(
      e_colony_building_slot slot, int unit_idx ) const;

  Rect sprite_rect_for_unit_in_slot( e_colony_building_slot slot,
                                     int unit_idx ) const;

  maybe<e_colony_building_slot> slot_for_coord(
      Coord where ) const;

 public:
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  static std::unique_ptr<ColViewBuildings> create(
      Delta size, Colony& colony, Player const& player,
      IGui& gui ) {
    return std::make_unique<ColViewBuildings>( size, colony,
                                               player, gui );
  }

  ColViewBuildings( Delta size, Colony& colony,
                    Player const& player, IGui& gui )
    : size_( size ),
      colony_( colony ),
      player_( player ),
      gui_( gui ) {}

 private:
  struct Dragging {
    UnitId                 id   = {};
    e_colony_building_slot slot = {};
  };

  Delta           size_;
  Colony&         colony_;
  Player const&   player_;
  IGui&           gui_;
  maybe<Dragging> dragging_ = {};
};

} // namespace rn
