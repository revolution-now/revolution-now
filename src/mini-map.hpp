/****************************************************************
**mini-map.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-11.
*
* Description: Draws the mini-map on the panel.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"
#include "view.hpp"

// ss
#include "ss/ref.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

struct SS;
struct TS;
struct Visibility;

/****************************************************************
** MiniMap
*****************************************************************/
struct MiniMap {
  MiniMap( SS& ss, Delta available_size );

  // This should be called when any external state changes that
  // could affect the minimap.
  void update();

  void zoom_in();

  void zoom_out();

  void center_box_on_tile( Coord where );

  void drag_map( Delta mouse_delta );

  void drag_box( Delta mouse_delta );

  gfx::size pixels_occupied() const { return pixels_occupied_; }

  // White box rect relative to this view in pixels.
  gfx::rect white_box_pixels() const;

  // Given a cursor position relative to the origin of this view
  // it will compute which map tile it is on, if any.
  maybe<gfx::point> tile_under_cursor( gfx::point p ) const;

  gfx::drect tiles_visible_on_minimap() const;

  // This gives the upper left map tile that is currently on the
  // minimap.
  gfx::dpoint upper_left_visible_tile() const;

  // This is the covered tiles, meaning the part inside the white
  // box, first computed in terms of fractional tiles, then trun-
  // cated.
  gfx::drect fractional_tiles_inside_white_box() const;

 private:
  // Any non-const method should call this at the end.
  void fix_invariants();

  SS& ss_;
  // Size in pixels of the mini-map.
  gfx::size pixels_occupied_;
};

/****************************************************************
** MiniMapView
*****************************************************************/
struct MiniMapView : ui::View {
  MiniMapView( SS& ss, TS& ts, Delta available )
    : ss_( ss ), ts_( ts ), mini_map_( ss, available ) {}

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  // Implement ui::Object.
  ND Delta delta() const override;

  // Override ui::Object.
  void advance_state() override;

  // Override ui::Object.
  [[nodiscard]] bool on_wheel(
      input::mouse_wheel_event_t const& event ) override;

  // Override ui::Object.
  [[nodiscard]] bool on_mouse_drag(
      input::mouse_drag_event_t const& event ) override;

  // Override ui::Object.
  [[nodiscard]] bool on_mouse_button(
      input::mouse_button_event_t const& event ) override;

 private:
  enum class e_mini_map_drag { map, white_box };

  // Any non-const method should call this at the end.
  void fix_invariants();

  void draw_impl( rr::Renderer&     renderer,
                  Visibility const& viz ) const;

  SS&                    ss_;
  TS&                    ts_;
  MiniMap                mini_map_;
  maybe<e_mini_map_drag> drag_state_;
};

} // namespace rn
