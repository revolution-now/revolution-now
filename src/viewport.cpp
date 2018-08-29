/****************************************************************
* viewport.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Handling of panning and zooming of the world
*              viewport.
*
*****************************************************************/
#include "viewport.hpp"

#include "global-constants.hpp"
#include "macros.hpp"
#include "world.hpp"

namespace rn {

namespace {

double viewport_zoom = 1.0;

// Do these need to be doubles?
double world_viewport_width()      { return double( g_world_viewport_width_tiles*g_tile_width._ )/viewport_zoom; }
double world_viewport_height()     { return double( g_world_viewport_height_tiles*g_tile_height._ )/viewport_zoom; }

double viewport_center_x = world_viewport_width()/2.0;
double viewport_center_y = world_viewport_height()/2.0;

double world_viewport_start_x() { return viewport_center_x - world_viewport_width()/2.0; }
double world_viewport_start_y() { return viewport_center_y - world_viewport_height()/2.0; }
double world_viewport_end_x()   { return viewport_center_x + world_viewport_width()/2.0; }
double world_viewport_end_y()   { return viewport_center_y + world_viewport_height()/2.0; }

X world_viewport_start_tile_x() { return X(W(int( world_viewport_start_x() ))/g_tile_width); }
Y world_viewport_start_tile_y() { return Y(H(int( world_viewport_start_y() ))/g_tile_height); }

} // namespace

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double world_viewport_width_tiles() {
  int lower = round_down_to_nearest_int_multiple(
          world_viewport_start_x(), 32 );
  int upper = round_up_to_nearest_int_multiple(
          world_viewport_end_x(), 32 );
  return upper-lower;
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double world_viewport_height_tiles() {
  int lower = round_down_to_nearest_int_multiple(
          world_viewport_start_y(), 32 );
  int upper = round_up_to_nearest_int_multiple(
          world_viewport_end_y(), 32 );
  return upper-lower;
}

void world_viewport_apply_bounds() {
  if( viewport_zoom < .5 ) viewport_zoom = .5;
  auto [size_y, size_x] = world_size_tiles();
  size_y *= g_tile_height._;
  size_x *= g_tile_width._;
  if( world_viewport_start_x() < 0 )
      viewport_center_x = world_viewport_width()/2.0;
  if( world_viewport_start_y() < 0 )
      viewport_center_y = world_viewport_height()/2.0;
  if( world_viewport_end_x() > size_x )
      viewport_center_x = double( size_x )-double( world_viewport_width() )/2.0;
  if( world_viewport_end_y() > size_y )
      viewport_center_y = double( size_y )-double( world_viewport_height() )/2.0;
}

Rect viewport_covered_tiles() {
  X start_tile_x = world_viewport_start_tile_x();
  if( start_tile_x < 0 ) start_tile_x = 0;
  Y start_tile_y = world_viewport_start_tile_y();
  if( start_tile_y < 0 ) start_tile_y = 0;

  auto [size_y, size_x] = world_size_tiles();
  X end_tile_x = world_viewport_start_tile_x() + world_viewport_width_tiles();
  if( end_tile_x > X(0) + size_x ) end_tile_x = X(0) + size_x;
  Y end_tile_y = world_viewport_start_tile_y() + world_viewport_height_tiles();
  if( end_tile_y > Y(0) + size_y ) end_tile_y = Y(0) + size_y;
  return {X(start_tile_x), Y(start_tile_y),
    W(end_tile_x-start_tile_x), H(end_tile_y-start_tile_y)};
}

Rect get_viewport() {
  return {
    X(int( world_viewport_start_x() )),
    Y(int( world_viewport_start_y() )),
    W(int( world_viewport_width() )),
    H(int( world_viewport_height() ))
  };
}

Rect viewport_get_render_src_rect() {
  Rect viewport = get_viewport();
  auto [max_src_height, max_src_width] = world_size_pixels();
  Rect src;
  src.x = viewport.x < X(0) ? X(0) : X(0) + viewport.x % g_tile_width;
  src.y = viewport.y < Y(0) ? Y(0) : Y(0) + viewport.y % g_tile_height;
  src.w = viewport.w > max_src_width ? max_src_width : viewport.w;
  src.h = viewport.h > max_src_height ? max_src_height : viewport.h;
  return src;
}

Rect viewport_get_render_dest_rect() {
  Rect dest;
  dest.x = 0;
  dest.y = 0;
  dest.w = g_tile_width._*g_world_viewport_width_tiles;
  dest.h = g_tile_height._*g_world_viewport_height_tiles;
  Rect viewport = get_viewport();
  auto [max_src_height, max_src_width] = world_size_pixels();
  if( viewport.w > max_src_width ) {
    double delta = (double( viewport.w - max_src_width )/double( viewport.w ))*g_tile_width._*double( int( g_world_viewport_width_tiles ) )/2.0;
    dest.x += int( delta );
    dest.w -= int( delta*2 );
  }
  if( viewport.h > max_src_height ) {
    double delta = (double( viewport.h - max_src_height )/double( viewport.h ))*g_tile_height._*double( int( g_world_viewport_height_tiles ) )/2.0;
    dest.y += int( delta );
    dest.h -= int( delta*2 );
  }
  return dest;
}

void viewport_scale_zoom( double factor ) {
  viewport_zoom *= factor;
  world_viewport_apply_bounds();
}

void viewport_pan( double down_up, double left_right, bool scale ) {
  double factor = scale ? viewport_zoom : 1.0;
  viewport_center_x += left_right/factor;
  viewport_center_y += down_up/factor;
  world_viewport_apply_bounds();
}

} // namespace rn

