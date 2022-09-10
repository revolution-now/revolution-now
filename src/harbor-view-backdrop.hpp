/****************************************************************
**harbor-view-backdrop.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Backdrop image layout within the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "harbor-view-entities.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborBackdrop
*****************************************************************/
struct HarborBackdrop : public ui::View, public HarborSubView {
  static PositionedHarborSubView<HarborBackdrop> create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      Coord cargo_upper_right, Coord inport_upper_right );

  // Implement ui::Object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View&       view() noexcept override;
  ui::View const& view() const noexcept override;

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  struct DockUnitsLayout {
    Coord units_start_floor = {};
    W     dock_length       = {};
  };

  // Returns the lower right pixel of the l
  DockUnitsLayout dock_units_layout() const;

 private:
  static W const kDockEdgeThickness = 7;
  static W const kDockSegmentWidth  = 32;
  struct Layout {
    // Distance from the bottom to the horizon.
    H     horizon           = {};
    X     land_tip          = {};
    Coord dock_lower_right  = {};
    int   num_dock_segments = {};
  };

 public:
  HarborBackdrop( SS& ss, TS& ts, Player& player, Delta size,
                  Layout layout );

 private:
  static Layout recomposite( Delta size, Coord cargo_upper_right,
                             Coord inport_upper_right );

  Delta  size_;
  Layout layout_;
};

} // namespace rn
