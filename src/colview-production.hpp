/****************************************************************
**colview-production.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-12.
*
* Description: Production view UI within the colony view.
*
*****************************************************************/
#pragma once

// rds
#include "colview-production.rds.hpp"

// Revolution Now
#include "colview-entities.hpp"
#include "spread-render.rds.hpp"

// config
#include "config/colony-constants.hpp"

// C++ standard library
#include <array>

namespace rn {

enum class e_tile;

struct ProductionViewButton {
  gfx::rect bounds;
  e_tile tile = {};
};

/****************************************************************
** ProductionView
*****************************************************************/
class ProductionView : public ui::View, public ColonySubView {
 public:
  static std::unique_ptr<ProductionView> create( SS& ss, TS& ts,
                                                 Player& player,
                                                 Colony& colony,
                                                 Delta size );

  using e_mode = e_colview_production_mode;

  struct Layout {
    gfx::size size = {};
    // The below are relative to view origin.
    int margin = {};

    // Hammer spread.
    gfx::rect hammer_spread_rect = {};
    using HammerArray = std::array<TileSpreadRenderPlan,
                                   kNumHammerRowsInColonyView>;
    HammerArray hammer_spreads;
    e_tile hammer_tile      = {};
    int hammer_row_interval = {};

    // Buttons.
    gfx::rect buttons_area_rect = {};
    refl::enum_map<e_mode, ProductionViewButton> buttons;
  };

  ProductionView( SS& ss, TS& ts, Player& player, Colony& colony,
                  Layout layout );

 public: // ui::Object
  Delta delta() const override { return layout_.size; }

  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

 public: // ColonySubView
  ui::View& view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void update_this_and_children() override;

 public: // AwaitView
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override;

 public: // IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::production );
  }

 private:
  static Layout create_layout( gfx::size sz );

  void create_hammer_spreads( Layout::HammerArray& out ) const;

  void update_hammer_spread();

  void draw_mode_production( rr::Renderer& renderer ) const;
  void draw_mode_units( rr::Renderer& renderer ) const;
  void draw_mode_construction( rr::Renderer& renderer ) const;

  Layout layout_;
  // Make this static because we want to preserve it from one
  // colony view to another (and when changing resolutions) but
  // we don't need it to be serialized.
  inline static e_mode mode_ = e_mode::production;
};

} // namespace rn
