/****************************************************************
**harbor-view-rpt.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-20.
*
* Description: Recruit/Purchase/Train button UI elements within
*              the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "harbor-view-rpt.rds.hpp"

// Revolution Now
#include "harbor-view-entities.hpp"
#include "wait.hpp"

namespace rn {

struct HarborBackdrop;
struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborRptButtons
*****************************************************************/
struct HarborRptButtons : public ui::View, public HarborSubView {
  static PositionedHarborSubView<HarborRptButtons> create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      HarborBackdrop const& backdrop );

  HarborRptButtons( SS& ss, TS& ts, Player& player );

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
  static constexpr H kVerticalSpacing = 4;

  static Delta button_size();
  static Delta total_size();

  static Rect        button_rect( e_rpt_button button );
  static std::string button_text_markup( e_rpt_button button );

  static maybe<e_rpt_button> button_for_coord( Coord where );
};

} // namespace rn
