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
  void ensure_tile_surroundings_visible( Coord const& coord,
                                         bool         smooth );

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
  static void set_zoom_push( e_push_direction );

  void smooth_zoom_target( double target );
  void smooth_center_target( Coord screen_coord );
  void stop_auto_zoom();
  void stop_auto_panning();

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
};

SmoothViewport& viewport();

} // namespace rn
