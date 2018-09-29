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

#include "globals.hpp"
#include "macros.hpp"
#include "world.hpp"

namespace rn {

// Config
constexpr double movement_speed      = 8.0;
constexpr double zoom_speed          = .08;
constexpr double zoom_accel          = 0.2*zoom_speed;
constexpr double zoom_accel_drag     = 0.05*zoom_speed;
constexpr double pan_accel_init      = 0.2*movement_speed;
constexpr double pan_accel_drag_init = 0.1*movement_speed;

namespace {

} // namespace

SmoothViewport& viewport() {
  static SmoothViewport viewport;
  return viewport;
}

SmoothViewport::SmoothViewport() :
  x_vel_(
    /*min_velocity=*/-movement_speed,
    /*max_velocity=*/movement_speed,
    /*initial_vel=*/0,
    /*mag_acceleration=*/pan_accel_init,
    /*mag_drag_acceleration=*/pan_accel_drag_init ),
  y_vel_(
    /*min_velocity=*/-movement_speed,
    /*max_velocity=*/movement_speed,
    /*initial_vel=*/0,
    /*mag_acceleration=*/pan_accel_init,
    /*mag_drag_acceleration=*/pan_accel_drag_init ),
  zoom_vel_(
    /*min_velocity=*/-zoom_speed,
    /*max_velocity=*/zoom_speed,
    /*initial_vel=*/0,
    /*mag_acceleration=*/zoom_accel,
    /*mag_drag_acceleration=*/zoom_accel_drag ),
  zoom_( 1.0 ),
  // zoom_ must be initialized before center_x_/y_
  center_x_( width_pixels()/2.0 ),
  center_y_( height_pixels()/2.0 )
{
  enforce_invariants();
}

void SmoothViewport::advance( e_push_direction x_push,
                              e_push_direction y_push,
                              e_push_direction zoom_push ) {

  double zoom_factor07 = pow( get_scale_zoom(), 0.7 );
  double zoom_factor15 = pow( get_scale_zoom(), 1.5 );
  double pan_accel       = pan_accel_init;
  double pan_accel_drag  = pan_accel_drag_init;
  pan_accel_drag = pan_accel_drag / pow( get_scale_zoom(), .75 );
  pan_accel = pan_accel_drag +
    (pan_accel-pan_accel_drag)/zoom_factor15;
  x_vel_.set_accelerations( pan_accel, pan_accel_drag );
  y_vel_.set_accelerations( pan_accel, pan_accel_drag );
  x_vel_.set_bounds( -movement_speed/zoom_factor07,
                      movement_speed/zoom_factor07 );
  y_vel_.set_bounds( -movement_speed/zoom_factor07,
                      movement_speed/zoom_factor07 );

  x_vel_.advance( x_push );
  y_vel_.advance( y_push );
  zoom_vel_.advance( zoom_push );

  pan( 0, x_vel_, false );
  pan( y_vel_, 0, false );
  scale_zoom( 1.0+zoom_vel_ );
}

double SmoothViewport::width_pixels() const {
  return double( viewport_width_tiles()*g_tile_width._ )/zoom_; }
double SmoothViewport::height_pixels() const {
  return double( viewport_height_tiles()*g_tile_height._ )/zoom_; }
double SmoothViewport::start_x() const {
  return center_x_ - width_pixels()/2.0; }
double SmoothViewport::start_y() const {
  return center_y_ - height_pixels()/2.0; }
double SmoothViewport::end_x() const {
  return center_x_ + width_pixels()/2.0; }
double SmoothViewport::end_y() const {
  return center_y_ + height_pixels()/2.0; }
X SmoothViewport::start_tile_x() const {
  return X(W(int( start_x() ))/g_tile_width); }
Y SmoothViewport::start_tile_y() const {
  return Y(H(int( start_y() ))/g_tile_height); }

Rect SmoothViewport::get_bounds() const {
  return {
    X(int( start_x() )),
    Y(int( start_y() )),
    W(int( width_pixels() )),
    H(int( height_pixels() ))
  };
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double SmoothViewport::width_tiles() const {
  int lower = round_down_to_nearest_int_multiple(
          start_x(), 32 );
  int upper = round_up_to_nearest_int_multiple(
          end_x(), 32 );
  return upper-lower;
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double SmoothViewport::height_tiles() const {
  int lower = round_down_to_nearest_int_multiple(
          start_y(), 32 );
  int upper = round_up_to_nearest_int_multiple(
          end_y(), 32 );
  return upper-lower;
}

void SmoothViewport::enforce_invariants() {
  if( zoom_ < .5 ) zoom_ = .5;
  auto [size_y, size_x] = world_size_tiles();
  size_y *= g_tile_height._;
  size_x *= g_tile_width._;
  if( start_x() < 0 )
      center_x_ = width_pixels()/2.0;
  if( start_y() < 0 )
      center_y_ = height_pixels()/2.0;
  if( end_x() > size_x )
      center_x_ = double( size_x )-double( width_pixels() )/2.0;
  if( end_y() > size_y )
      center_y_ = double( size_y )-double( height_pixels() )/2.0;
}

// Tiles touched by the viewport (tiles at the edge may only be
// partially visible).
Rect SmoothViewport::covered_tiles() const {
  CHECK( start_tile_x() >= 0 );
  CHECK( start_tile_y() >= 0 );

  auto [size_y, size_x] = world_size_tiles();
  X end_tile_x = start_tile_x() + width_tiles();
  //if( end_tile_x > X(0) + size_x ) end_tile_x = X(0) + size_x;
  CHECK( end_tile_x <= X(0) + size_x ); // can remove eventually
  Y end_tile_y = start_tile_y() + height_tiles();
  //if( end_tile_y > Y(0) + size_y ) end_tile_y = Y(0) + size_y;
  CHECK( end_tile_y <= Y(0) + size_y ); // can remove eventually

  return {X(start_tile_x()), Y(start_tile_y()),
    W(end_tile_x-start_tile_x()), H(end_tile_y-start_tile_y())};
}

Rect SmoothViewport::get_render_src_rect() const {
  Rect viewport = get_bounds();
  auto [max_src_height, max_src_width] = world_size_pixels();
  Rect src;
  src.x = viewport.x < X(0) ? X(0) : X(0) + viewport.x % g_tile_width;
  src.y = viewport.y < Y(0) ? Y(0) : Y(0) + viewport.y % g_tile_height;
  src.w = viewport.w > max_src_width ? max_src_width : viewport.w;
  src.h = viewport.h > max_src_height ? max_src_height : viewport.h;
  return src;
}

Rect SmoothViewport::get_render_dest_rect() const {
  Rect dest;
  dest.x = 0;
  dest.y = 0;
  dest.w = g_tile_width._*viewport_width_tiles();
  dest.h = g_tile_height._*viewport_height_tiles();
  Rect viewport = get_bounds();
  auto [max_src_height, max_src_width] = world_size_pixels();
  if( viewport.w > max_src_width ) {
    double delta =
      (double( viewport.w - max_src_width )/
       double( viewport.w ))
      *g_tile_width._*double( int( viewport_width_tiles() ) )/2.0;
    dest.x += int( delta );
    dest.w -= int( delta*2 );
  }
  if( viewport.h > max_src_height ) {
    double delta =
      (double( viewport.h - max_src_height )/
       double( viewport.h ))
      *g_tile_height._*double( int( viewport_height_tiles() ) )/2.0;
    dest.y += int( delta );
    dest.h -= int( delta*2 );
  }
  return dest;
}

void SmoothViewport::scale_zoom( double factor ) {
  zoom_ *= factor;
  enforce_invariants();
}

double SmoothViewport::get_scale_zoom() const {
  return zoom_;
}

void SmoothViewport::pan(
    double down_up, double left_right, bool scale ) {
  double factor = scale ? zoom_ : 1.0;
  center_x_ += left_right/factor;
  center_y_ += down_up/factor;
  enforce_invariants();
}

void SmoothViewport::center_on_tile_x( Coord coords ) {
  center_x_ = double( coords.x*g_tile_width._ + g_tile_width/2 );
  enforce_invariants();
}

void SmoothViewport::center_on_tile_y( Coord coords ) {
  center_y_ = double( coords.y*g_tile_height._ + g_tile_height/2 );
  enforce_invariants();
}

void SmoothViewport::center_on_tile( Coord coords ) {
  center_on_tile_x( coords );
  center_on_tile_y( coords );
  enforce_invariants();
}

bool SmoothViewport::is_tile_fully_visible( Coord coords ) const {
  return coords.is_inside( covered_tiles().edges_removed() );
}

// Determines if the surroundings of the coordinate in the
// C-dimension are fully visible in the viewport.  This is
// non-trivial because we have to apply less stringent rules
// as the coordinate gets closer to the edges of the world.
template<typename C>
bool is_tile_surroundings_fully_visible( SmoothViewport const& vp,
                                         Coord coords ) {
  auto covered = vp.covered_tiles();
  auto covered_inner = covered.edges_removed();
  //auto covered_inner_inner = covered_inner.edges_removed();

  auto& box = covered_inner;

  auto box_start = box.coordinate<C>();
  auto box_end   = box.coordinate<C>() + box.length<C>();

  return (coords.coordinate<C>() >= box_start) &&
         (coords.coordinate<C>() <  box_end);
}

void SmoothViewport::ensure_tile_surroundings_visible( Coord coord ) {
  if( !is_tile_surroundings_fully_visible<X>( *this, coord ) )
    center_on_tile_x( coord );
  if( !is_tile_surroundings_fully_visible<Y>( *this, coord ) )
    center_on_tile_y( coord );
}

} // namespace rn
