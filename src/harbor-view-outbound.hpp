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
#include "harbor-view-entities.hpp"
#include "wait.hpp"

namespace rn {

struct HarborMarketCommodities;
struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborOutboundShips
*****************************************************************/
struct HarborOutboundShips : public ui::View,
                             public HarborSubView {
  static PositionedHarborSubView create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      HarborMarketCommodities const& market_commodities,
      Coord                          harbor_inport_upper_left );

  HarborOutboundShips( SS& ss, TS& ts, Player& player,
                       bool is_wide );

  // Implement ui::Object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View&       view() noexcept override;
  ui::View const& view() const noexcept override;

  maybe<DraggableObjectWithBounds> object_here(
      Coord const& where ) const override;

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  // Implement ui::AwaitView.
  virtual wait<> perform_click(
      input::mouse_button_event_t const& ) override;

 private:
  struct UnitWithPosition {
    UnitId id;
    Coord  pixel_coord;
  };

  std::vector<UnitWithPosition> units( Coord origin ) const;

  // The coord is relative to the upper left of this view.
  maybe<UnitWithPosition> unit_at_location( Coord where ) const;

  maybe<UnitId> get_active_unit() const;
  void          set_active_unit( UnitId unit_id );

  wait<> click_on_unit( UnitId unit_id );

  static Delta size_blocks( bool is_wide );
  static Delta size_pixels( bool is_wide );

  struct Draggable {
    UnitId unit_id = {};
  };

  maybe<Draggable> dragging_;
  bool             is_wide_ = false;
};

} // namespace rn
