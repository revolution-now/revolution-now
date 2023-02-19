/****************************************************************
**colview-population.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-05.
*
* Description: Population view UI within the colony view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colview-entities.hpp"

namespace rn {

struct Player;

/****************************************************************
** PopulationView
*****************************************************************/
class PopulationView : public ui::View, public ColonySubView {
 public:
  static std::unique_ptr<PopulationView> create( SS& ss, TS& ts,
                                                 Player& player,
                                                 Colony& colony,
                                                 Delta   size ) {
    return std::make_unique<PopulationView>( ss, ts, player,
                                             colony, size );
  }

  PopulationView( SS& ss, TS& ts, Player& player, Colony& colony,
                  Delta size )
    : ColonySubView( ss, ts, player, colony ), size_( size ) {}

  Delta delta() const override { return size_; }

  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::population );
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

 private:
  void draw_sons_of_liberty( rr::Renderer& renderer,
                             Coord         coord ) const;

  Delta size_;
};

} // namespace rn
