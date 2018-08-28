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

double viewport_center_x = 0;
double viewport_center_y = 0;
double viewport_zoom = 1.0;
  
double world_viewport_width()      { return g_world_viewport_width_tiles*32/viewport_zoom; }
double world_viewport_height()     { return g_world_viewport_height_tiles*32/viewport_zoom; }

double world_viewport_start_x() { return viewport_center_x - world_viewport_width()/2.0; }
double world_viewport_start_y() { return viewport_center_y - world_viewport_height()/2.0; }
double world_viewport_end_x()   { return viewport_center_x + world_viewport_width()/2.0; }
double world_viewport_end_y()   { return viewport_center_y + world_viewport_height()/2.0; }

int world_viewport_start_tile_x() { return int( world_viewport_start_x() )/32; }
int world_viewport_start_tile_y() { return int( world_viewport_start_y() )/32; }

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
  size_y *= g_tile_height;
  size_x *= g_tile_width;
  if( world_viewport_start_x() < 0 )
      viewport_center_x = world_viewport_width()/2.0;
  if( world_viewport_start_y() < 0 )
      viewport_center_y = world_viewport_height()/2.0;
  if( world_viewport_end_x() > size_x )
      viewport_center_x = size_x-world_viewport_width()/2.0;
  if( world_viewport_end_y() > size_y )
      viewport_center_y = size_y-world_viewport_height()/2.0;
}

STARTUP() { world_viewport_apply_bounds(); }

Rect viewport_covered_tiles() {
  int start_tile_x = world_viewport_start_tile_x();
  if( start_tile_x < 0 ) start_tile_x = 0;
  int start_tile_y = world_viewport_start_tile_y();
  if( start_tile_y < 0 ) start_tile_y = 0;

  auto [size_y, size_x] = world_size_tiles();
  int end_tile_x = world_viewport_start_tile_x() + world_viewport_width_tiles();
  if( end_tile_x > size_x ) end_tile_x = size_x;
  int end_tile_y = world_viewport_start_tile_y() + world_viewport_height_tiles();
  if( end_tile_y > size_y ) end_tile_y = size_y;
  return {start_tile_x, start_tile_y,
    end_tile_x-start_tile_x, end_tile_y-start_tile_y};
}

Rect get_viewport() {
  return {
    int( world_viewport_start_x() ),
    int( world_viewport_start_y() ),
    int( world_viewport_width() ),
    int( world_viewport_height() )
  };
}

::SDL_Rect viewport_get_render_src_rect() {
  Rect viewport = get_viewport();
  auto [max_src_height, max_src_width] = world_size_pixels();
  ::SDL_Rect src;
  src.x = viewport.x < 0 ? 0 : viewport.x % g_tile_width;
  src.y = viewport.y < 0 ? 0 : viewport.y % g_tile_height;
  src.w = viewport.w > max_src_width ? max_src_width : viewport.w;
  src.h = viewport.h > max_src_height ? max_src_height : viewport.h;
  return src;
}

::SDL_Rect viewport_get_render_dest_rect() {
  ::SDL_Rect dest;
  dest.x = dest.y = 0;
  dest.w = g_tile_width*g_world_viewport_width_tiles;
  dest.h = g_tile_height*g_world_viewport_height_tiles;
  Rect viewport = get_viewport();
  auto [max_src_height, max_src_width] = world_size_pixels();
  if( viewport.w > max_src_width ) {
    double delta = ((double( viewport.w ) - max_src_width )/viewport.w)*g_tile_width*g_world_viewport_width_tiles/2.0;
    dest.x += int( delta );
    dest.w -= int( delta*2 );
  }
  if( viewport.h > max_src_height ) {
    double delta = ((double( viewport.h ) - max_src_height )/viewport.h)*g_tile_height*g_world_viewport_height_tiles/2.0;
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

