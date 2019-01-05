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
#include "aliases.hpp"
#include "geo-types.hpp"
#include "physics.hpp"

// SDL
#include "SDL.h"

namespace rn {

class SmoothViewport {
public:
  SmoothViewport();

  void advance();

  // Tiles touched by the viewport (tiles at the edge may only be
  // partially visible).
  Rect covered_tiles() const;

  // This function will shift the viewport to make the tile
  // coordinate visible, but will avoid shifting if it is already
  // visible. If smooth == true then it will animate the motion
  // as opposed to a sudden shift.
  void ensure_tile_visible( Coord const& coord, bool smooth );

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

  // Given a screen pixel coordinate this will return the world
  // coordinate.
  Opt<Coord> screen_pixel_to_world_pixel(
      Coord pixel_coord ) const;

  // Given a screen pixel coordinate this will determine whether
  // it is in the viewport.
  bool screen_coord_in_viewport( Coord pixel_coord ) const;

  static void set_x_push( e_push_direction );
  static void set_y_push( e_push_direction );
  // If provided, the maybe_seek_screen_coord represents a point
  // on the screen (typically, mouse cursor position) to which
  // the viewport center to should tend as the zoom is done. This
  // allows the user to zoom into a particular point by zooming
  // while the mouse cursor is over that point.
  void set_zoom_push( e_push_direction,
                      Opt<Coord> maybe_seek_screen_coord );

  void smooth_zoom_target( double target );
  void stop_auto_zoom();
  void stop_auto_panning();

  // Move the center of the viewport by the given change in
  // screen coordinates. This means that the delta will be scaled
  // to world coordinates (using the zoom) before applying the
  // shift. Note that they don't have to yield a valid end result
  // since this function will e.g. stop panning when the left
  // edge of the viewport hids x=0.
  void pan_by_screen_coords( Delta delta );

private:
  void enforce_invariants();

  void advance( e_push_direction x_push, e_push_direction y_push,
                e_push_direction zoom_push );

  double width_pixels() const;
  double height_pixels() const;

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

  double get_scale_zoom() const;

  void scale_zoom( double factor );
  void pan( double down_up, double left_right, bool scale );
  void center_on_tile_x( Coord const& coords );
  void center_on_tile_y( Coord const& coords );
  void center_on_tile( Coord const& coords );

  bool is_tile_fully_visible( Coord const& coords ) const;

  DissipativeVelocity x_vel_;
  DissipativeVelocity y_vel_;
  DissipativeVelocity zoom_vel_;

  double zoom_{};
  double center_x_{};
  double center_y_{};

  // If these have values then the viewport will attempt to move
  // the values to them smoothly.
  Opt<double> smooth_zoom_target_{};
  Opt<XD>     smooth_center_x_target_{};
  Opt<YD>     smooth_center_y_target_{};

  // Delta in world pixel coordinates indicating the direction in
  // which the viewport center should tend as we zoom in.
  Opt<Delta> zoom_point_seek_{};
};

SmoothViewport& viewport();

} // namespace rn
