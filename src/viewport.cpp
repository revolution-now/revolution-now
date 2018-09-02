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

namespace viewport {

namespace {

double zoom = 1.0;

// Do these need to be doubles?
double width_pixels()      { return double( g_viewport_width_tiles*g_tile_width._ )/zoom; }
double height_pixels()     { return double( g_viewport_height_tiles*g_tile_height._ )/zoom; }

double center_x = width_pixels()/2.0;
double center_y = height_pixels()/2.0;

double start_x() { return center_x - width_pixels()/2.0; }
double start_y() { return center_y - height_pixels()/2.0; }
double end_x()   { return center_x + width_pixels()/2.0; }
double end_y()   { return center_y + height_pixels()/2.0; }

X start_tile_x() { return X(W(int( start_x() ))/g_tile_width); }
Y start_tile_y() { return Y(H(int( start_y() ))/g_tile_height); }

Rect get_bounds() {
  return {
    X(int( start_x() )),
    Y(int( start_y() )),
    W(int( width_pixels() )),
    H(int( height_pixels() ))
  };
}

} // namespace

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double width_tiles() {
  int lower = round_down_to_nearest_int_multiple(
          start_x(), 32 );
  int upper = round_up_to_nearest_int_multiple(
          end_x(), 32 );
  return upper-lower;
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double height_tiles() {
  int lower = round_down_to_nearest_int_multiple(
          start_y(), 32 );
  int upper = round_up_to_nearest_int_multiple(
          end_y(), 32 );
  return upper-lower;
}

void enforce_invariants() {
  if( zoom < .5 ) zoom = .5;
  auto [size_y, size_x] = world_size_tiles();
  size_y *= g_tile_height._;
  size_x *= g_tile_width._;
  if( start_x() < 0 )
      center_x = width_pixels()/2.0;
  if( start_y() < 0 )
      center_y = height_pixels()/2.0;
  if( end_x() > size_x )
      center_x = double( size_x )-double( width_pixels() )/2.0;
  if( end_y() > size_y )
      center_y = double( size_y )-double( height_pixels() )/2.0;
}

// Tiles touched by the viewport (tiles at the edge may only be
// partially visible).
Rect covered_tiles() {
  // Probably can remove eventually
  ASSERT( start_tile_x() >= 0 );
  ASSERT( start_tile_y() >= 0 );

  auto [size_y, size_x] = world_size_tiles();
  X end_tile_x = start_tile_x() + width_tiles();
  //if( end_tile_x > X(0) + size_x ) end_tile_x = X(0) + size_x;
  ASSERT( end_tile_x <= X(0) + size_x ); // can remove eventually
  Y end_tile_y = start_tile_y() + height_tiles();
  //if( end_tile_y > Y(0) + size_y ) end_tile_y = Y(0) + size_y;
  ASSERT( end_tile_y <= Y(0) + size_y ); // can remove eventually

  return {X(start_tile_x()), Y(start_tile_y()),
    W(end_tile_x-start_tile_x()), H(end_tile_y-start_tile_y())};
}

Rect get_render_src_rect() {
  Rect viewport = get_bounds();
  auto [max_src_height, max_src_width] = world_size_pixels();
  Rect src;
  src.x = viewport.x < X(0) ? X(0) : X(0) + viewport.x % g_tile_width;
  src.y = viewport.y < Y(0) ? Y(0) : Y(0) + viewport.y % g_tile_height;
  src.w = viewport.w > max_src_width ? max_src_width : viewport.w;
  src.h = viewport.h > max_src_height ? max_src_height : viewport.h;
  return src;
}

Rect get_render_dest_rect() {
  Rect dest;
  dest.x = 0;
  dest.y = 0;
  dest.w = g_tile_width._*g_viewport_width_tiles;
  dest.h = g_tile_height._*g_viewport_height_tiles;
  Rect viewport = get_bounds();
  auto [max_src_height, max_src_width] = world_size_pixels();
  if( viewport.w > max_src_width ) {
    double delta = (double( viewport.w - max_src_width )/double( viewport.w ))*g_tile_width._*double( int( g_viewport_width_tiles ) )/2.0;
    dest.x += int( delta );
    dest.w -= int( delta*2 );
  }
  if( viewport.h > max_src_height ) {
    double delta = (double( viewport.h - max_src_height )/double( viewport.h ))*g_tile_height._*double( int( g_viewport_height_tiles ) )/2.0;
    dest.y += int( delta );
    dest.h -= int( delta*2 );
  }
  return dest;
}

void scale_zoom( double factor ) {
  zoom *= factor;
  enforce_invariants();
}

void pan( double down_up, double left_right, bool scale ) {
  double factor = scale ? zoom : 1.0;
  center_x += left_right/factor;
  center_y += down_up/factor;
  enforce_invariants();
}

void center_on_tile_x( Coord coords ) {
  center_x = double( coords.x*g_tile_width._ + g_tile_width/2 );
  enforce_invariants();
}

void center_on_tile_y( Coord coords ) {
  center_y = double( coords.y*g_tile_height._ + g_tile_height/2 );
  enforce_invariants();
}

void center_on_tile( Coord coords ) {
  center_on_tile_x( coords );
  center_on_tile_y( coords );
  enforce_invariants();
}

bool is_tile_fully_visible( Coord coords ) {
  return coords.is_inside( covered_tiles().edges_removed() );
}

// Determines if the surroundings of the coordinate in the
// C-dimension are fully visible in the viewport.  This is
// non-trivial because we have to apply less stringent rules
// as the coordinate gets closer to the edges of the world.
template<typename C>
bool is_tile_surroundings_fully_visible( Coord coords ) {
  auto covered = covered_tiles();
  auto covered_inner = covered.edges_removed();
  //auto covered_inner_inner = covered_inner.edges_removed();

  auto& box = covered_inner;

  auto box_start = box.coordinate<C>();
  auto box_end   = box.coordinate<C>() + box.length<C>();

  return (coords.coordinate<C>() >= box_start) &&
         (coords.coordinate<C>() <  box_end);
}

void ensure_tile_surroundings_visible( Coord coord ) {
  if( !is_tile_surroundings_fully_visible<X>( coord ) )
    center_on_tile_x( coord );
  if( !is_tile_surroundings_fully_visible<Y>( coord ) )
    center_on_tile_y( coord );
}

} // namespace viewport

} // namespace rn
