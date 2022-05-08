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
#include "coord.hpp"
#include "error.hpp"
#include "physics.hpp"
#include "wait.hpp"

// Rds
#include "viewport.rds.hpp"

namespace rn {

// This viewport also knows where it is located on screen.
class SmoothViewport {
 public:
  bool operator==( SmoothViewport const& ) const;

  SmoothViewport();

  void advance_state( Rect const& viewport_rect_pixels );

  // Tiles touched by the viewport (tiles at the edge may only be
  // partially visible).
  Rect covered_tiles() const;
  Rect fully_covered_tiles() const;

  bool is_fully_visible_x() const;
  bool is_fully_visible_y() const;

  // Will give us a rect of world pixels covered by the viewport.
  Rect covered_pixels() const;

  // This function will shift the viewport to make the tile
  // coordinate visible, but will avoid shifting if it is already
  // visible.
  void ensure_tile_visible( Coord const& coord );
  // Same as above but will animate the motion as opposed to a
  // sudden shift. The wait will be fulfilled when the given
  // tile becomes visible, but the scrolling may continue for a
  // bit after that.
  wait<> ensure_tile_visible_smooth( Coord const& coord );

  // This function will compute the rectangle in the source
  // viewport texture that should be rendered to the screen.
  // NOTE: this function assumes that only the covered_tiles()
  // will have been rendered to the texture. So mainly this
  // function deals with slightly shifting the rect within the
  // width of a single tile, along with some edge cases.
  Rect rendering_src_rect() const;
  // This function computes the rectangle on the screen to which
  // the viewport should be rendered. This would be trivial but
  // it also deals with the situation where the world is smaller
  // than the viewport, in which case it will center the rect in
  // the available area.
  Rect rendering_dest_rect() const;

  // This will provide the upper left corning where the GPU
  // should start rendering the landscape buffer (which could be
  // off screen) in order to make the covered area visible on
  // screen.
  Coord landscape_buffer_render_upper_left() const;

  // Given a screen pixel coordinate this will return the world
  // coordinate.
  maybe<Coord> screen_pixel_to_world_pixel(
      Coord pixel_coord ) const;

  maybe<Coord> world_pixel_to_screen_pixel(
      Coord world_pixel ) const;

  // Given a screen pixel coordinate this will return the world
  // tile coordinate.
  maybe<Coord> screen_pixel_to_world_tile(
      Coord pixel_coord ) const;

  // Given a screen pixel coordinate this will determine whether
  // it is in the viewport.
  bool screen_coord_in_viewport( Coord pixel_coord ) const;

  // This should be called once when constructing a new game
  // world.
  void set_max_viewable_size_tiles( Delta size );

  // Immediate change.
  void center_on_tile( Coord const& coords );

  void set_x_push( e_push_direction );
  void set_y_push( e_push_direction );
  // If provided, the maybe_seek_screen_coord represents a point
  // on the screen (typically, mouse cursor position) to which
  // the viewport center to should tend as the zoom is done. This
  // allows the user to zoom into a particular point by zooming
  // while the mouse cursor is over that point.
  void set_zoom_push( e_push_direction,
                      maybe<Coord> maybe_seek_screen_coord );

  void smooth_zoom_target(
      double target, maybe<Coord> maybe_seek_screen_coord = {} );
  void stop_auto_zoom();
  void stop_auto_panning();

  // Pixel in screen coords indicating the point toward which we
  // should scroll to whether zooming or not. This will override
  // zoom_point_seek_.
  void set_point_seek( Coord screen_pixel );

  // Return current zoom.
  double get_zoom() const;
  void   set_zoom( double new_zoom );

  // Move the center of the viewport by the given change in
  // screen coordinates. This means that the delta will be scaled
  // to world coordinates (using the zoom) before applying the
  // shift. Note that they don't have to yield a valid end result
  // since this function will e.g. stop panning when the left
  // edge of the viewport hids x=0.
  void pan_by_screen_coords( Delta delta );

  // This will ensure that the zoom value has at most four sig-
  // nificant decimal digits, since this seems to prevent blank
  // horizontal and vertical lines from randomly emerging in the
  // land view as it is zoomed in or out.
  void fix_zoom_rounding();

  // Will not throw or die if invariants are broken; instead, if
  // an invariant is broken it will be fixed, and this is a
  // normal part of the behavior of this class.
  void fix_invariants();

  // Implement refl::WrapsReflected.
  SmoothViewport( wrapped::SmoothViewport&& o );
  wrapped::SmoothViewport const&    refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "SmoothViewport";

 private:
  void advance( e_push_direction x_push, e_push_direction y_push,
                e_push_direction zoom_push );

  void advance_zoom_point_seek( DissipativeVelocity const& vel );

  template<typename C>
  friend bool are_tile_surroundings_as_fully_visible_as_can_be(
      SmoothViewport const& vp, Coord const& coords );

  bool need_to_scroll_to_reveal_tile( Coord const& coord ) const;

  double x_world_pixels_in_viewport() const;
  double y_world_pixels_in_viewport() const;

  // These are to avoid a direct dependency on the screen module
  // and its initialization code.
  Delta world_size_pixels() const;
  Rect  world_rect_pixels() const;
  Rect  world_rect_tiles() const;

  double start_x() const;
  double start_y() const;
  double end_x() const;
  double end_y() const;

  X start_tile_x() const;
  Y start_tile_y() const;

  Rect get_bounds() const;

  // Returns world coordinates of center in pixels, rounded
  // to the nearest pixel.
  Coord center_rounded() const;

  double width_tiles() const;
  double height_tiles() const;

  double minimum_zoom_for_viewport();

  void scale_zoom( double factor );
  void pan( double down_up, double left_right, bool scale );
  void center_on_tile_x( Coord const& coords );
  void center_on_tile_y( Coord const& coords );

  bool is_tile_fully_visible( Coord const& coords ) const;

  // ==================== Serialized Fields =====================

  wrapped::SmoothViewport o_;

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

  struct SmoothCenter {
    XD    x_target{};
    YD    y_target{};
    Coord tile_target{};
    // This promise will be fulfilled when the above tile becomes
    // visible, even if there is a bit more scrolling left to do;
    // the scrolling will still continue though.
    wait_promise<> promise{};
    bool operator==( SmoothCenter const& ) const = default;
  };
  // If this has a value then the viewport will attempt to scroll
  // to match it.
  maybe<SmoothCenter> coro_smooth_center_{};

  // Coord in world pixel coordinates indicating the point toward
  // which we should focus as we zoom (though only while zoom-
  // ing, and only if point_seek_ is not specified).
  maybe<Coord> zoom_point_seek_{};

  // Coord in world pixel coordinates indicating the point toward
  // which we should scroll to whether zooming or not. This will
  // override zoom_point_seek_.
  maybe<Coord> point_seek_{};

  Rect  viewport_rect_pixels_{};
  Delta world_size_tiles_{};
};
NOTHROW_MOVE( SmoothViewport );

} // namespace rn
