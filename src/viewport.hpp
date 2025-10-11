/****************************************************************
**viewport.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Handling of panning and zooming of the world
*              viewport.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"
#include "physics.hpp"
#include "wait.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/coord.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

struct IEngine;
struct Viewport;
struct TerrainState;

// FIXME: Since this module has become a mess, we should plan to
// migrate/rewrite its functionality gradually into the camera
// module with unit tests, progress on which has already begun.

// This viewport also knows where it is located on screen.
struct ViewportController {
  bool operator==( ViewportController const& ) const;

  ViewportController( IEngine& engine,
                      TerrainState const& terrain,
                      Viewport& viewport,
                      gfx::rect viewport_rect_pixels );

  void advance_state();

  // Tiles touched by the viewport (tiles at the edge may only be
  // partially visible).
  Rect covered_tiles() const;
  Rect fully_covered_tiles() const;

  // Are we zoomed out enough so that some of the space outside
  // of the map is visible on any side?
  bool are_surroundings_visible() const;

  bool is_fully_visible_x() const;
  bool is_fully_visible_y() const;

  // Will give us a rect of world pixels covered by the viewport.
  gfx::drect covered_pixels() const;
  Rect covered_pixels_rounded() const;

  // This function will shift the viewport to make the tile coor-
  // dinate visible plus some surrounding squares, but will avoid
  // shifting if it is already visible in that sense.
  void ensure_tile_visible( gfx::point const& coord );
  // Same as above but will animate the motion as opposed to a
  // sudden shift. The wait will be fulfilled when the given tile
  // becomes visible, but the scrolling may continue for a bit
  // after that. If the target area of the map is too far from
  // the current area then it will just jump immediately there to
  // avoid too much scrolling.
  wait<> ensure_tile_visible_smooth( gfx::point const& coord );

  // This function computes the rectangle on the screen to which
  // the viewport should be rendered. This would be trivial but
  // it also deals with the situation where the world is smaller
  // than the viewport, in which case it will center the rect in
  // the available area.
  gfx::drect rendering_dest_rect() const;
  Rect rendering_dest_rect_rounded() const;

  // Computes the zoom required so that the entire map is visible
  // with a bit of map surrounds visible as well.
  double optimal_min_zoom() const;

  // Computes the critical zoom point below which (i.e., if you
  // were to zoom out a bit further) would be revealed space
  // around the map.
  double min_zoom_for_no_border() const;

  double min_zoom_allowed() const;

  Rect world_rect_pixels() const;
  Rect world_rect_tiles() const;
  Delta world_size_pixels() const;
  Delta world_size_tiles() const;

  void update_logical_rect_cache(
      gfx::rect viewport_rect_pixels );

  // This will provide the upper left corning where the GPU
  // should start rendering the landscape buffer (which could be
  // off screen) in order to make the covered area visible on
  // screen.
  gfx::dpoint landscape_buffer_render_upper_left() const;

  // Given a screen pixel coordinate this will return the world
  // coordinate.
  maybe<gfx::point> screen_pixel_to_world_pixel(
      gfx::point pixel_coord ) const;

  gfx::point world_tile_to_world_pixel_center(
      gfx::point world_tile ) const;

  maybe<gfx::point> world_tile_to_screen_pixel(
      gfx::point world_tile ) const;

  maybe<gfx::point> world_pixel_to_screen_pixel(
      gfx::point world_pixel ) const;

  // Given a screen pixel coordinate this will return the world
  // tile coordinate.
  maybe<gfx::point> screen_pixel_to_world_tile(
      gfx::point pixel_coord ) const;

  // These will always return a value, even if it is negative or
  // otherwise off of the map. In other words, it behaves as if
  // the map is infinitely large but with the origin tile where
  // it actually is. So the returned value could have negative
  // coordinates, or could be beyond the real map area.
  gfx::point screen_pixel_to_hypothetical_world_pixel(
      gfx::point pixel_coord ) const;
  gfx::point screen_pixel_to_hypothetical_world_tile(
      gfx::point pixel_coord ) const;

  // Given a screen pixel coordinate this will determine whether
  // it is in the viewport.
  bool screen_coord_in_viewport( gfx::point pixel_coord ) const;

  // Immediate change.
  void center_on_tile( gfx::point const& coords );
  wait<> center_on_tile_smooth( gfx::point coord );

  void set_x_push( e_push_direction );
  void set_y_push( e_push_direction );
  // If provided, the maybe_seek_screen_coord represents a point
  // on the screen (typically, mouse cursor position) to which
  // the viewport center to should tend as the zoom is done. This
  // allows the user to zoom into a particular point by zooming
  // while the mouse cursor is over that point.
  void set_zoom_push(
      e_push_direction,
      maybe<gfx::point> maybe_seek_screen_coord );

  void smooth_zoom_target(
      double target,
      maybe<gfx::point> maybe_seek_screen_coord = {} );
  void stop_auto_zoom();
  void stop_auto_panning();

  // Pixel indicating the point toward which we should scroll to
  // whether zooming or not. This will override zoom_point_seek_.
  void set_point_seek( gfx::point world_pixel );
  void set_point_seek_from_screen_pixel(
      gfx::point screen_pixel );

  // Return current zoom.
  double get_zoom() const;
  void set_zoom( double new_zoom );

  // Move the center of the viewport by the given change in
  // screen coordinates. This means that the delta will be scaled
  // to world coordinates (using the zoom) before applying the
  // shift. Note that they don't have to yield a valid end result
  // since this function will e.g. stop panning when the left
  // edge of the viewport hids x=0.
  void pan_by_screen_coords( Delta delta );

  // No scaling.
  void pan_by_world_coords( Delta delta );

  // When more precision is needed.
  void pan_by_world_coords( gfx::dsize size );

  // Will not throw or die if invariants are broken; instead, if
  // an invariant is broken it will be fixed, and this is a
  // normal part of the behavior of this class.
  void fix_invariants();

 private:
  void advance( e_push_direction x_push, e_push_direction y_push,
                e_push_direction zoom_push );

  void advance_zoom_point_seek( DissipativeVelocity const& vel );

  bool is_tile_too_far( gfx::point tile ) const;

  template<typename C>
  friend bool are_tile_surroundings_as_fully_visible_as_can_be(
      ViewportController const& vp, Coord const& coords );

  bool need_to_scroll_to_reveal_tile(
      gfx::point const& coord ) const;

  double x_world_pixels_in_viewport() const;
  double y_world_pixels_in_viewport() const;

  double start_x() const;
  double start_y() const;
  double end_x() const;
  double end_y() const;

  gfx::drect get_bounds() const;
  Rect get_bounds_rounded() const;

  // Returns world coordinates of center in pixels, rounded
  // to the nearest pixel.
  gfx::point center_rounded() const;

  void scale_zoom( double factor );
  void pan( double down_up, double left_right, bool scale );
  void center_on_tile_x( gfx::point const& coords );
  void center_on_tile_y( gfx::point const& coords );

  bool is_tile_fully_visible( gfx::point const& coords ) const;

  // ==================== Serialized Fields =====================

  IEngine& engine_;
  TerrainState const& terrain_;
  Viewport& o_;

  // ============ Transient Fields (not serialized) =============

  DissipativeVelocity x_vel_{};
  DissipativeVelocity y_vel_{};
  DissipativeVelocity zoom_vel_{};

  e_push_direction x_push_{ e_push_direction::none };
  e_push_direction y_push_{ e_push_direction::none };
  e_push_direction zoom_push_{ e_push_direction::none };

  // If this has a value then the viewport will attempt to zoom
  // to match it.
  maybe<double> smooth_zoom_target_{};

  struct SmoothScroll {
    double x_target{};
    double y_target{};
    gfx::point tile_target{};
    // This promise will be fulfilled when the above tile becomes
    // visible, even if there is a bit more scrolling left to do;
    // the scrolling will still continue though.
    wait_promise<>* promise                      = nullptr;
    bool operator==( SmoothScroll const& ) const = default;
  };
  // If this has a value then the viewport will attempt to scroll
  // to match it.
  maybe<SmoothScroll> coro_smooth_scroll_{};

  // Coord in world pixel coordinates indicating the point toward
  // which we should focus as we zoom (though only while zoom-
  // ing, and only if point_seek_ is not specified).
  maybe<gfx::point> zoom_point_seek_{};

  // Coord in world pixel coordinates indicating the point toward
  // which we should scroll to whether zooming or not. This will
  // override zoom_point_seek_.
  maybe<gfx::point> point_seek_{};

  Rect viewport_rect_pixels_{};
};

} // namespace rn
