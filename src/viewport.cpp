/****************************************************************
**viewport.cpp
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

#include "errors.hpp"
#include "globals.hpp"
#include "tiles.hpp"
#include "util.hpp"
#include "world.hpp"

using namespace std;

namespace rn {

// Config
constexpr double movement_speed      = 8.0;
constexpr double zoom_min            = 0.5;
constexpr double zoom_speed          = .08;
constexpr double zoom_accel          = 0.2 * zoom_speed;
constexpr double zoom_accel_drag     = 0.05 * zoom_speed;
constexpr double pan_accel_init      = 0.2 * movement_speed;
constexpr double pan_accel_drag_init = 0.1 * movement_speed;
constexpr double seek_percent_while_zooming = 1.0 / 7.0;

namespace {

e_push_direction x_push{e_push_direction::none};
e_push_direction y_push{e_push_direction::none};
e_push_direction zoom_push{e_push_direction::none};

} // namespace

SmoothViewport& viewport() {
  static SmoothViewport viewport;
  return viewport;
}

SmoothViewport::SmoothViewport()
  : x_vel_(
        /*min_velocity=*/-movement_speed,
        /*max_velocity=*/movement_speed,
        /*initial_velocity=*/0,
        /*mag_acceleration=*/pan_accel_init,
        /*mag_drag_acceleration=*/pan_accel_drag_init ),
    y_vel_(
        /*min_velocity=*/-movement_speed,
        /*max_velocity=*/movement_speed,
        /*initial_velocity=*/0,
        /*mag_acceleration=*/pan_accel_init,
        /*mag_drag_acceleration=*/pan_accel_drag_init ),
    zoom_vel_(
        /*min_velocity=*/-zoom_speed,
        /*max_velocity=*/zoom_speed,
        /*initial_velocity=*/0,
        /*mag_acceleration=*/zoom_accel,
        /*mag_drag_acceleration=*/zoom_accel_drag ),
    zoom_( 1.0 ),
    // zoom_ must be initialized before center_x_/y_
    center_x_( width_pixels() / 2 ),
    center_y_( height_pixels() / 2 ),
    smooth_zoom_target_{},
    smooth_center_x_target_{},
    smooth_center_y_target_{},
    zoom_point_seek_{} {
  enforce_invariants();
}

void SmoothViewport::advance( e_push_direction x_push,
                              e_push_direction y_push,
                              e_push_direction zoom_push ) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  double zoom_factor07 = pow( get_scale_zoom(), 0.7 );
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  double zoom_factor15  = pow( get_scale_zoom(), 1.5 );
  double pan_accel      = pan_accel_init;
  double pan_accel_drag = pan_accel_drag_init;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  pan_accel_drag = pan_accel_drag / pow( get_scale_zoom(), .75 );
  pan_accel      = pan_accel_drag +
              ( pan_accel - pan_accel_drag ) / zoom_factor15;
  x_vel_.set_accelerations( pan_accel, pan_accel_drag );
  y_vel_.set_accelerations( pan_accel, pan_accel_drag );
  x_vel_.set_bounds( -movement_speed / zoom_factor07,
                     movement_speed / zoom_factor07 );
  y_vel_.set_bounds( -movement_speed / zoom_factor07,
                     movement_speed / zoom_factor07 );

  x_vel_.advance( x_push );
  y_vel_.advance( y_push );
  zoom_vel_.advance( zoom_push );

  pan( 0, x_vel_.to_double(), false );
  pan( y_vel_.to_double(), 0, false );
  scale_zoom( 1.0 + zoom_vel_.to_double() );

  if( zoom_point_seek_ ) {
    center_x_ += zoom_point_seek_->w._ * zoom_vel_.to_double();
    center_y_ += zoom_point_seek_->h._ * zoom_vel_.to_double();
    enforce_invariants();
  }
}

template<typename T>
bool sign( T t ) {
  return t >= T();
}

struct TargetingRates {
  double rate;
  double shift;
  double linear_window;
};

// These numbers were chosen through iterative trials to yield
// the nicest looking result, which is to say that the transition
// will quickly start approaching its target (faster the farther
// away it is) and then slow down as it nears the target, then
// when it is within a critical distance it will start moving at
// a linear rate so that it doesn't take too long to get there.
constexpr TargetingRates translation_seeking_parameters{
    /*rate=*/.90,
    /*shift=*/1.0,
    /*linear_window=*/8.0};
constexpr TargetingRates zoom_seeking_parameters{
    /*rate=*/.95,
    /*shift=*/.001,
    /*linear_window=*/.015};

// This function will take a numerical value that is being
// gradually moved to a target value (in a somewhat asymptotic
// manner in that the movement slows as the target nears) and
// will advance it by one "frame". When it has reached the target
// this function will signal (through the output parameters) that
// the movement can stop.
template<typename T>
void advance_target_seeking( Opt<T>& maybe_target, double& val,
                             DissipativeVelocity&  vel,
                             TargetingRates const& params ) {
  if( !maybe_target.has_value() ) return;
  double target = double( *maybe_target );
  if( val == target ) return;
  auto old_val = val;

  if( fabs( val - target ) < params.linear_window )
    val += ( val < target ) ? params.shift : -params.shift;
  else
    val = target + ( val - target ) * params.rate;

  if( sign( val - target ) != sign( old_val - target ) ) {
    // The center has crossed the target mark; so since we are
    // trying to normalize it to the target, we take this
    // opporunity to make it precisely equal and to stop the
    // normalizing process. This avoid oscillations.
    val = target;
    vel.hit_wall();
    maybe_target = nullopt;
  }
}

void SmoothViewport::advance() {
  advance( x_push, y_push, zoom_push );

  advance_target_seeking( smooth_center_x_target_, center_x_,
                          x_vel_,
                          translation_seeking_parameters );
  advance_target_seeking( smooth_center_y_target_, center_y_,
                          y_vel_,
                          translation_seeking_parameters );
  advance_target_seeking( smooth_zoom_target_, zoom_, zoom_vel_,
                          zoom_seeking_parameters );

  x_push    = e_push_direction::none;
  y_push    = e_push_direction::none;
  zoom_push = e_push_direction::none;
  enforce_invariants();
}

void SmoothViewport::set_x_push( e_push_direction push ) {
  x_push = push;
}

void SmoothViewport::set_y_push( e_push_direction push ) {
  y_push = push;
}

void SmoothViewport::set_zoom_push(
    e_push_direction push, Opt<Coord> maybe_seek_screen_coord ) {
  zoom_push = push;

  zoom_point_seek_ = nullopt;
  // If the caller has specified a coordinate and if that
  // coordinate is in the viewport then record it so that the
  // viewport center can tend to that point as the zoom happens.
  if( maybe_seek_screen_coord ) {
    auto world_coord_seek =
        screen_pixel_to_world_pixel( *maybe_seek_screen_coord );
    if( world_coord_seek )
      zoom_point_seek_ = *world_coord_seek - center_rounded();
  }
}

void SmoothViewport::smooth_zoom_target( double target ) {
  smooth_zoom_target_ = target;
}
void SmoothViewport::stop_auto_zoom() {
  smooth_zoom_target_ = std::nullopt;
}

void SmoothViewport::stop_auto_panning() {
  smooth_center_x_target_ = std::nullopt;
  smooth_center_y_target_ = std::nullopt;
}

double SmoothViewport::width_pixels() const {
  return double( viewport_width_tiles() * g_tile_width ) / zoom_;
}
double SmoothViewport::height_pixels() const {
  return double( viewport_height_tiles() * g_tile_height ) /
         zoom_;
}

double SmoothViewport::start_x() const {
  return center_x_ - width_pixels() / 2;
}
double SmoothViewport::start_y() const {
  return center_y_ - height_pixels() / 2;
}
double SmoothViewport::end_x() const {
  return center_x_ + width_pixels() / 2;
}
double SmoothViewport::end_y() const {
  return center_y_ + height_pixels() / 2;
}
X SmoothViewport::start_tile_x() const {
  return X( int( start_x() ) ) / g_tile_width;
}
Y SmoothViewport::start_tile_y() const {
  return Y( int( start_y() ) ) / g_tile_height;
}

Rect SmoothViewport::get_bounds() const {
  return {X( int( start_x() ) ), Y( int( start_y() ) ),
          W( int( width_pixels() ) ),
          H( int( height_pixels() ) )};
}

Coord SmoothViewport::center_rounded() const {
  return Coord{X{int( lround( center_x_ ) )},
               Y{int( lround( center_y_ ) )}};
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double SmoothViewport::width_tiles() const {
  int lower = round_down_to_nearest_int_multiple(
      start_x(), g_tile_width._ );
  int upper = round_up_to_nearest_int_multiple( end_x(),
                                                g_tile_width._ );
  return upper - lower;
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double SmoothViewport::height_tiles() const {
  int lower = round_down_to_nearest_int_multiple(
      start_y(), g_tile_height._ );
  int upper = round_up_to_nearest_int_multiple(
      end_y(), g_tile_height._ );
  return upper - lower;
}

void SmoothViewport::enforce_invariants() {
  if( zoom_ < zoom_min ) zoom_ = zoom_min;
  auto [size_y, size_x] = world_size_tiles();
  size_y *= g_tile_height;
  size_x *= g_tile_width;
  if( start_x() < 0 ) center_x_ = width_pixels() / 2;
  if( start_y() < 0 ) center_y_ = height_pixels() / 2;
  if( end_x() > double( size_x ) )
    center_x_ = double( size_x ) - double( width_pixels() ) / 2;
  if( end_y() > double( size_y ) )
    center_y_ = double( size_y ) - double( height_pixels() ) / 2;
}

// Tiles touched by the viewport (tiles at the edge may only be
// partially visible).
Rect SmoothViewport::covered_tiles() const {
  CHECK( start_tile_x() >= 0 );
  CHECK( start_tile_y() >= 0 );

  auto [size_y, size_x] = world_size_tiles();
  X end_tile_x = start_tile_x() + lround( width_tiles() );
  // if( end_tile_x > 0_x + size_x ) end_tile_x = 0_x + size_x;
  CHECK( end_tile_x <= 0_x + size_x ); // can remove eventually
  Y end_tile_y = start_tile_y() + lround( height_tiles() );
  // if( end_tile_y > 0_y + size_y ) end_tile_y = 0_y + size_y;
  CHECK( end_tile_y <= 0_y + size_y ); // can remove eventually

  return {X( start_tile_x() ), Y( start_tile_y() ),
          W( end_tile_x - start_tile_x() ),
          H( end_tile_y - start_tile_y() )};
}

Rect SmoothViewport::rendering_src_rect() const {
  Rect viewport                        = get_bounds();
  auto [max_src_height, max_src_width] = world_size_pixels();
  Rect src;
  CHECK( !( viewport.x < 0_x ) );
  CHECK( !( viewport.y < 0_y ) );
  src.x =
      viewport.x < 0_x ? 0_x : 0_x + viewport.x % g_tile_width;
  src.y =
      viewport.y < 0_y ? 0_y : 0_y + viewport.y % g_tile_height;
  src.w =
      viewport.w > max_src_width ? max_src_width : viewport.w;
  src.h =
      viewport.h > max_src_height ? max_src_height : viewport.h;
  return src;
}

Rect SmoothViewport::rendering_dest_rect() const {
  Rect dest;
  dest.x        = 0;
  dest.y        = 0;
  dest.w        = viewport_width_tiles() * g_tile_width;
  dest.h        = viewport_height_tiles() * g_tile_height;
  Rect viewport = get_bounds();
  auto [max_src_height, max_src_width] = world_size_pixels();
  if( viewport.w > max_src_width ) {
    double delta = ( double( viewport.w - max_src_width ) /
                     double( viewport.w ) ) *
                   g_tile_width._ *
                   double( int( viewport_width_tiles() ) ) / 2;
    dest.x += int( delta );
    dest.w -= int( delta * 2 );
  }
  if( viewport.h > max_src_height ) {
    double delta = ( double( viewport.h - max_src_height ) /
                     double( viewport.h ) ) *
                   g_tile_height._ *
                   double( int( viewport_height_tiles() ) ) / 2;
    dest.y += int( delta );
    dest.h -= int( delta * 2 );
  }
  return dest;
}

Opt<Coord> SmoothViewport::screen_pixel_to_world_pixel(
    Coord pixel_coord ) const {
  Rect visible_on_screen = rendering_dest_rect();
  auto from_visible_start =
      pixel_coord - visible_on_screen.upper_left();
  if( from_visible_start.w < 0_w ||
      from_visible_start.h < 0_h ) {
    return nullopt;
  }
  if( from_visible_start.w > visible_on_screen.w ||
      from_visible_start.h > visible_on_screen.h ) {
    return nullopt;
  }

  double percent_x =
      double( from_visible_start.w._ ) / visible_on_screen.w._;
  double percent_y =
      double( from_visible_start.h._ ) / visible_on_screen.h._;

  Rect viewport = get_bounds();

  auto res =
      Coord{X{int( viewport.x._ + percent_x * viewport.w._ )},
            Y{int( viewport.y._ + percent_y * viewport.h._ )}};
  return res;
}

bool SmoothViewport::screen_coord_in_viewport(
    Coord pixel_coord ) const {
  return screen_pixel_to_world_pixel( pixel_coord ).has_value();
}

void SmoothViewport::scale_zoom( double factor ) {
  zoom_ *= factor;
  enforce_invariants();
}

double SmoothViewport::get_scale_zoom() const { return zoom_; }

void SmoothViewport::pan( double down_up, double left_right,
                          bool scale ) {
  double factor = scale ? zoom_ : 1.0;
  center_x_ += left_right / factor;
  center_y_ += down_up / factor;
  enforce_invariants();
}

void SmoothViewport::center_on_tile_x( Coord const& coords ) {
  center_x_ =
      double( coords.x * g_tile_width + g_tile_width._ / 2 );
  enforce_invariants();
}

void SmoothViewport::center_on_tile_y( Coord const& coords ) {
  center_y_ =
      double( coords.y * g_tile_height + g_tile_height._ / 2 );
  enforce_invariants();
}

void SmoothViewport::center_on_tile( Coord const& coords ) {
  center_on_tile_x( coords );
  center_on_tile_y( coords );
  enforce_invariants();
}

bool SmoothViewport::is_tile_fully_visible(
    Coord const& coords ) const {
  return coords.is_inside( covered_tiles().edges_removed() );
}

// Determines if the surroundings of the coordinate in the
// C-dimension are fully visible in the viewport.  This is
// non-trivial because we have to apply less stringent rules
// as the coordinate gets closer to the edges of the world.
template<typename C>
bool is_tile_surroundings_fully_visible(
    SmoothViewport const& vp, Coord const& coords ) {
  auto covered       = vp.covered_tiles();
  auto covered_inner = covered.edges_removed().edges_removed();
  // auto covered_inner_inner = covered_inner.edges_removed();

  auto& box = covered_inner;

  auto box_start = box.coordinate<C>();
  auto box_end   = box.coordinate<C>() + box.length<C>();

  return ( coords.coordinate<C>() >= box_start ) &&
         ( coords.coordinate<C>() < box_end );
}

void SmoothViewport::ensure_tile_visible( Coord const& coord,
                                          bool         smooth ) {
  if( !is_tile_surroundings_fully_visible<X>( *this, coord ) ) {
    if( smooth )
      smooth_center_x_target_ =
          XD{double( ( coord.x * g_tile_width )._ )};
    else
      center_on_tile_x( coord );
  }
  if( !is_tile_surroundings_fully_visible<Y>( *this, coord ) ) {
    if( smooth )
      smooth_center_y_target_ =
          YD{double( ( coord.y * g_tile_height )._ )};
    else
      center_on_tile_y( coord );
  }
}

} // namespace rn
