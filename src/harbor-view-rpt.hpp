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

// refl
#include "refl/enum-map.hpp"

namespace rn {

struct HarborBackdrop;
struct HarborDockUnits;
struct HarborInPortShips;
struct Player;
struct SS;
struct TS;

enum class e_tile;

// For some strange reason we need to keep this outside of the
// below class otherwise it fails the enum_map's test for default
// constructibility. It has something to do with the struct not
// being marked as "complete" since it is nested. See this bug:
//
//   https://bugs.llvm.org/show_bug.cgi?id=38374
//
// Ideally we would have nested this inside Layout.
struct RptButton {
  gfx::rect bounds = {};
  e_tile text_tile = {};
};

/****************************************************************
** HarborRptButtons
*****************************************************************/
struct HarborRptButtons : public ui::View, public HarborSubView {
  static PositionedHarborSubView<HarborRptButtons> create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      HarborBackdrop const& backdrop,
      HarborDockUnits& harbor_dock_units );

  struct Layout {
    // Absolute coordinates. The nw of this view is the left-most
    // point of the sign post (the one extending to the left).
    gfx::rect view = {};
    // Relative to the nw of the view.
    gfx::point post_left_point    = {};
    gfx::point post_sprite_origin = {};

    refl::enum_map<e_rpt_button, RptButton> buttons = {};
  };

  HarborRptButtons( SS& ss, TS& ts, Player& player,
                    HarborDockUnits& harbor_dock_units,
                    Layout layout );

 public: // ui::Object
  Delta delta() const override;

  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  bool on_mouse_move(
      input::mouse_move_event_t const& event ) override;

  void on_mouse_leave( Coord from ) override;
  void on_mouse_enter( Coord from ) override;

 public: // IDraggableObjectsView.
  maybe<int> entity() const override;

 public: // HarborSubView
  ui::View& view() noexcept override;

  ui::View const& view() const noexcept override;

 public: // ui::AwaitView.
  virtual wait<> perform_click(
      input::mouse_button_event_t const& event ) override;

  virtual wait<bool> perform_key(
      input::key_event_t const& event ) override;

 private:
  // Mouse position is relative to this view.
  void update_mouse_hover( gfx::point mouse_pos );

  maybe<e_rpt_button> button_for_coord( gfx::point where ) const;

  static Layout create_layout( gfx::rect canvas,
                               HarborBackdrop const& backdrop );

  HarborDockUnits& harbor_dock_units_;
  Layout const layout_;
  // The reason that we need to store the hover state instead of
  // just measuring it directly in the draw method to determine
  // button highlighting state is that we want this hover be-
  // havior to respect the presence of modal windows and other
  // views that are on top of this one, which we do by hooking
  // into the usual input events and recording the state that
  // way, rather than querying the input device directly.
  maybe<e_rpt_button> mouse_hover_;
};

} // namespace rn
