/****************************************************************
**harbor-view-exit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Exit button UI element within the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "harbor-view-entities.hpp"
#include "wait.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborExitButton
*****************************************************************/
struct HarborExitButton : public ui::View, public HarborSubView {
  static PositionedHarborSubView create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      Coord market_upper_right, Coord market_lower_right );

  HarborExitButton( SS& ss, TS& ts, Player& player );

  // Implement ui::Object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View&       view() noexcept override;
  ui::View const& view() const noexcept override;

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  // Implement ui::AwaitView.
  virtual wait<> perform_click(
      input::mouse_button_event_t const& ) override;

 private:
  static constexpr Delta kExitBlockPixels{ .w = 26, .h = 26 };
};

} // namespace rn
