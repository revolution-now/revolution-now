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
#include "spread-render.rds.hpp"

namespace rn {

struct Player;

/****************************************************************
** PopulationView
*****************************************************************/
class PopulationView : public ui::View, public ColonySubView {
 public:
  static std::unique_ptr<PopulationView> create( IEngine& engine,
                                                 SS& ss, TS& ts,
                                                 Player& player,
                                                 Colony& colony,
                                                 Delta size );

  struct Layout {
    gfx::size size = {};
    TileSpreadRenderPlans production_spreads;
    // Relative to view origin.
    int spread_margin        = {};
    gfx::point spread_origin = {};
  };

  PopulationView( IEngine& engine, SS& ss, TS& ts,
                  Player& player, Colony& colony, Layout layout )
    : ColonySubView( engine, ss, ts, player, colony ),
      layout_( layout ) {}

 public: // ui::Object.
  Delta delta() const override { return layout_.size; }

  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::population );
  }

 public: // ColonySubView.
  ui::View& view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void update_this_and_children() override;

 private:
  static Layout create_layout( IEngine& engine,
                               SSConst const& ss, gfx::size sz );

  void draw_sons_of_liberty( rr::Renderer& renderer,
                             Coord coord ) const;

  void draw_production_spreads( rr::Renderer& renderer,
                                Coord coord ) const;

  Layout layout_;
};

} // namespace rn
