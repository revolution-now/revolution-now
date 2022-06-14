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
#include "colview-buildings.rds.hpp"

namespace rn {

/****************************************************************
** Buildings
*****************************************************************/
class ColViewBuildings : public ui::View, public ColonySubView {
 public:
  Delta delta() const override { return size_; }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::buildings;
  }

  ui::View& view() noexcept override { return *this; }

  ui::View const& view() const noexcept override {
    return *this;
  }

 private:
  Rect rect_for_slot( e_colony_building_slot slot ) const;

  maybe<e_colony_building> building_for_slot(
      e_colony_building_slot slot ) const;

 public:
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  static std::unique_ptr<ColViewBuildings> create(
      Delta size, Colony& colony ) {
    return std::make_unique<ColViewBuildings>( size, colony );
  }

  ColViewBuildings( Delta size, Colony& colony )
    : size_( size ), colony_( colony ) {}

 private:
  Delta   size_;
  Colony& colony_;
};

} // namespace rn
