/****************************************************************
**harbor-view-status.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-02.
*
* Description: Status bar in harbor view.
*
*****************************************************************/
#pragma once

// rds
#include "harbor-view-status.rds.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "harbor-view-entities.hpp"
#include "wait.hpp"

// gfx
#include "gfx/pixel.hpp"

namespace rn {

struct Player;
struct SS;
struct TS;

/****************************************************************
** HarborStatusBar
*****************************************************************/
struct HarborStatusBar : public ui::View, public HarborSubView {
  static PositionedHarborSubView<HarborStatusBar> create(
      SS& ss, TS& ts, Player& player, Rect canvas );

  HarborStatusBar( HarborStatusBar&& ) = delete;

  struct Layout {
    // Absolute coordinates. The nw of this view is the nw of the
    // status bar which is also the nw of the screen, so absolute
    // and logical coordinates are the same in this view. The
    // size of the view is just the wood bar area, though the
    // view may decide to do some rendering below that.
    gfx::rect view         = {};
    gfx::point text_center = {};
  };

  HarborStatusBar( SS& ss, TS& ts, Player& player,
                   Layout layout );

 public: // ui::object
  Delta delta() const override;

  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

 public: // ui::AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& ) override;

 public: // IDraggableObjectsView.
  maybe<int> entity() const override;

 public: // HarborSubView
  ui::View& view() noexcept override;

  ui::View const& view() const noexcept override;

 public:
  void inject_message( HarborStatusMsg const& msg );

 private:
  static Layout create_layout( gfx::rect canvas );

  wait<> status_generator();

  wait<HarborStatusMsg> wait_for_override();

  void draw_text( rr::Renderer& renderer ) const;

  std::string build_status_normal() const;

  Layout const layout_;

  // By default the standard status bar text is generated and
  // displayed each frame, though it can be overridden by this if
  // present.
  maybe<std::string> status_override_;
  maybe<gfx::pixel> text_color_override_;
  co::stream<HarborStatusMsg> injected_msgs_;
  wait<> status_generator_thread_;
};

} // namespace rn
