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

#include "physics.hpp"
#include "util.hpp"

#include <SDL.h>

namespace rn {

class SmoothViewport {
public:
  SmoothViewport();

  void advance( e_push_direction x_push, e_push_direction y_push,
                e_push_direction zoom_push );

  // Tiles touched by the viewport (tiles at the edge may only be
  // partially visible).
  Rect covered_tiles() const;

  void ensure_tile_surroundings_visible( Coord const& coord );

  Rect get_render_src_rect() const;
  Rect get_render_dest_rect() const;

private:
  void enforce_invariants();

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
};

SmoothViewport& viewport();

} // namespace rn
