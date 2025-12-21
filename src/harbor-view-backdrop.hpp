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

// Revolution Now
#include "harbor-view-entities.hpp"
#include "wait.hpp"

// gfx
#include "gfx/pixel.hpp"

namespace rn {

struct IRand;
struct SS;
struct TS;
struct Player;

enum class e_tile;

struct DockUnitsLayout {
  int right_edge  = {};
  int bottom_edge = {};

  gfx::point dock_row_start;
  gfx::point hill_row_start;
  std::vector<gfx::point> ground_rows;
};

/****************************************************************
** HarborBackdrop
*****************************************************************/
struct HarborBackdrop : public ui::View, public HarborSubView {
  static PositionedHarborSubView<HarborBackdrop> create(
      IEngine& engine, SS& ss, TS& ts, IRand& rand,
      Player& player, Rect canvas );

  // Implement ui::object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View& view() noexcept override;
  ui::View const& view() const noexcept override;

  // Implement ui::object.
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  // This is called by the dock units view so that we can draw it
  // over the units.
  void draw_dock_overlay( rr::Renderer& renderer,
                          gfx::point where ) const;

  // Returns the lower right pixel of the l
  DockUnitsLayout const& dock_units_layout() const;

  // Distance from the bottom of the screen to the top of the
  // houses on the horizon.
  gfx::point horizon_center() const;

  int extra_space_for_ships() const;

 private:
  static W const kDockEdgeThickness = 7;
  static W const kDockSegmentWidth  = 32;
  struct Layout {
    gfx::pixel sky_color = gfx::pixel::from_hex_rgb( 0x86c2d3 );
    gfx::rect ocean;
    gfx::point clouds_origin;
    std::vector<std::pair<gfx::size, e_tile>> clouds;
    gfx::rect sun;
    gfx::point land_origin;
    gfx::point houses_origin;
    gfx::point flag_origin;
    gfx::point dock_physical_nw;
    gfx::point dock_sprite_nw;
    gfx::point dock_board_nw;
    int extra_space_for_ships = {};

    // Distance from the bottom to the horizon.
    int horizon_height        = {};
    gfx::point horizon_center = {};

    // Unit positioning.
    DockUnitsLayout dock_units;

    struct BirdsLayout {
      gfx::point p;
      e_tile tile = {};
    };
    std::vector<BirdsLayout> birds_states;
  };

 public:
  HarborBackdrop( IEngine& engine, SS& ss, TS& ts,
                  Player& player, Delta size, Layout layout );

 private:
  static Layout recomposite( IRand& rand, gfx::size size );

  static void insert_clouds( Layout& l, gfx::size shift );

  Delta const size_;
  Layout const layout_;

  // Birds animation.
  struct BirdsFrameState {
    int frame = {};

    static int constexpr kVisTotal = 100;
    int visible                    = {};
  };
  struct BirdsState {
    std::vector<BirdsFrameState> frame_states = {};
  };
  maybe<BirdsState> birds_state_;
  wait<> birds_thread_;
  wait<> birds_thread();

  // Smoke animation.
  struct SmokeState {
    double l_stage = {};
    double m_stage = {};
    double r_stage = {};
  };
  SmokeState smoke_state_;
  wait<> smoke_thread_;
  wait<> smoke_thread();

  // Flag animation.
  struct FlagState {
    double l_stage = {};
    double r_stage = {};
  };
  FlagState flag_state_;
  wait<> flag_thread_;
  wait<> flag_thread();
};

} // namespace rn
