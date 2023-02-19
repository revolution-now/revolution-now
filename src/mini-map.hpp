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
// This object contains the logic that evolves the layout of the
// mini-map and the operations that can be performed on it by the
// user, such as dragging the map, dragging the viewport, click-
// ing, and zooming.
struct MiniMap {
  MiniMap( SS& ss, gfx::size available_size );

  void set_origin( gfx::dpoint p );

  void drag_map( gfx::size mouse_delta );

  void drag_box( gfx::size mouse_delta );

  void advance_auto_pan();

  // This gives the map coordinate in fractional tiles that is
  // currently in the upper left corner of the mini-map.
  gfx::dpoint origin() const;

  gfx::size size_screen_pixels() const {
    return size_screen_pixels_;
  }

  gfx::drect tiles_visible_on_minimap() const;

  // This is the part inside the white box in fractional tiles.
  gfx::drect fractional_tiles_inside_white_box() const;

  // Just for testing.
  void set_animation_speed( double speed ) {
    animation_speed_ = speed;
  }

 private:
  // This needs to be called anytime the mini-map origin changes.
  void fix_invariants();

  SS& ss_;
  // Size in pixels of the mini-map.
  gfx::size size_screen_pixels_;

  // FIXME: this animation speed needs to be made
  // frame-rate-independent.
  double animation_speed_ = 3.0;
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
  // White box rect relative to this view in pixels.
  gfx::rect white_box_pixels() const;

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
