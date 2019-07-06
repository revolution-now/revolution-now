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

// Revolution Now
#include "compositor.hpp"
#include "config-files.hpp"
#include "errors.hpp"
#include "screen.hpp"
#include "terrain.hpp"
#include "tiles.hpp"
#include "util.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

using namespace std;

namespace rn {

namespace {

e_push_direction x_push{e_push_direction::none};
e_push_direction y_push{e_push_direction::none};
e_push_direction zoom_push{e_push_direction::none};

double pan_accel_init() {
  return config_rn.viewport.pan_accel_init_coeff *
         config_rn.viewport.pan_speed;
}

double pan_accel_drag_init() {
  return config_rn.viewport.pan_accel_drag_init_coeff *
         config_rn.viewport.pan_speed;
}

Delta viewport_size_pixels() {
  return compositor::section( compositor::e_section::viewport )
      .delta();
}

} // namespace

SmoothViewport& viewport() {
  static SmoothViewport viewport{{0_x, 16_y}};
  return viewport;
}

SmoothViewport::SmoothViewport( Coord origin_on_screen )
  : origin_on_screen_( origin_on_screen ),
    x_vel_(
        /*min_velocity=*/-config_rn.viewport.pan_speed,
        /*max_velocity=*/config_rn.viewport.pan_speed,
        /*initial_velocity=*/0,
        /*mag_acceleration=*/pan_accel_init(),
        /*mag_drag_acceleration=*/pan_accel_drag_init() ),
    y_vel_(
        /*min_velocity=*/-config_rn.viewport.pan_speed,
        /*max_velocity=*/config_rn.viewport.pan_speed,
        /*initial_velocity=*/0,
        /*mag_acceleration=*/pan_accel_init(),
        /*mag_drag_acceleration=*/pan_accel_drag_init() ),
    zoom_vel_(
        /*min_velocity=*/-config_rn.viewport.zoom_speed,
        /*max_velocity=*/config_rn.viewport.zoom_speed,
        /*initial_velocity=*/0,
        /*mag_acceleration=*/
        config_rn.viewport.zoom_accel_coeff *
            config_rn.viewport.zoom_speed,
        /*mag_drag_acceleration=*/
        config_rn.viewport.zoom_accel_drag_coeff *
            config_rn.viewport.zoom_speed ),
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
  double zoom_factor07 = pow( get_zoom(), 0.7 );
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  double zoom_factor15  = pow( get_zoom(), 1.5 );
  double pan_accel      = pan_accel_init();
  double pan_accel_drag = pan_accel_drag_init();
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  pan_accel_drag = pan_accel_drag / pow( get_zoom(), .75 );
  pan_accel =
      pan_accel_drag +
      ( pan_accel - pan_accel_drag_init() ) / zoom_factor15;
  x_vel_.set_accelerations( pan_accel, pan_accel_drag );
  y_vel_.set_accelerations( pan_accel, pan_accel_drag );
  x_vel_.set_bounds(
      -config_rn.viewport.pan_speed / zoom_factor07,
      config_rn.viewport.pan_speed / zoom_factor07 );
  y_vel_.set_bounds(
      -config_rn.viewport.pan_speed / zoom_factor07,
      config_rn.viewport.pan_speed / zoom_factor07 );

  x_vel_.advance( x_push );
  y_vel_.advance( y_push );
  zoom_vel_.advance( zoom_push );

  pan( 0, x_vel_.to_double(), false );
  pan( y_vel_.to_double(), 0, false );
  scale_zoom( 1.0 + zoom_vel_.to_double() );

  if( zoom_point_seek_ ) {
    auto log_zoom_change = log( 1.0 + zoom_vel_.to_double() );
    // .98 found empirically, makes it slightly better, where
    // "better" means that the same pixel stays under the cursor
    // as the zooming happens.
    auto delta_x =
        .98 * ( ( zoom_point_seek_->x - center_rounded().x )._ *
                log_zoom_change );
    auto delta_y =
        .98 * ( ( zoom_point_seek_->y - center_rounded().y )._ *
                log_zoom_change );
    center_x_ += delta_x;
    center_y_ += delta_y;
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
  if( maybe_seek_screen_coord )
    zoom_point_seek_ =
        screen_pixel_to_world_pixel( *maybe_seek_screen_coord );
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
  return double( viewport_size_pixels().w ) / zoom_;
}
double SmoothViewport::height_pixels() const {
  return double( viewport_size_pixels().h ) / zoom_;
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
  if( zoom_ < config_rn.viewport.zoom_min )
    zoom_ = config_rn.viewport.zoom_min;
  auto [size_x, size_y] = world_size_tiles();
  size_y *= g_tile_height;
  size_x *= g_tile_width;
  // For each dimension we say the following: if we are zoomed in
  // sufficiently such that the entire world is not fully visible
  // (in the given dimension) then we enforce the invariants that
  // the viewport window must be fully inside the world.
  //
  // If on the other hand we are zoomed out far enough that the
  // entire world is visible (in the given dimension) then we
  // allow the edges of the viewport to go off of the world.
  if( width_pixels() <= size_x ) {
    if( start_x() < 0 ) center_x_ = width_pixels() / 2;
    if( end_x() > double( size_x ) )
      center_x_ =
          double( size_x ) - double( width_pixels() ) / 2;
  } else {
    center_x_ = double( 0_x + world_size_pixels().w / 2_sx );
  }
  if( height_pixels() <= size_y ) {
    if( start_y() < 0 ) center_y_ = height_pixels() / 2;
    if( end_y() > double( size_y ) )
      center_y_ =
          double( size_y ) - double( height_pixels() ) / 2;
  } else {
    center_y_ = double( 0_y + world_size_pixels().h / 2_sy );
  }
}

// Tiles touched by the viewport (tiles at the edge may only be
// partially visible).
Rect SmoothViewport::covered_tiles() const {
  return Rect{X( start_tile_x() ), Y( start_tile_y() ),
              W{static_cast<int>( lround( width_tiles() ) )},
              H{static_cast<int>( lround( height_tiles() ) )}}
      .clamp( world_rect_tiles() );
}

Rect SmoothViewport::covered_pixels() const {
  X x_start{static_cast<int>( lround( start_x() ) )};
  Y y_start{static_cast<int>( lround( start_y() ) )};
  X x_end{static_cast<int>( lround( end_x() ) )};
  Y y_end{static_cast<int>( lround( end_y() ) )};

  return Rect{x_start, y_start, x_end - x_start, y_end - y_start}
      .clamp( world_rect_pixels() );
}

Rect SmoothViewport::rendering_src_rect() const {
  Rect viewport = get_bounds();
  return Rect::from(
             Coord{} + viewport.upper_left() % g_tile_scale,
             viewport.delta() )
      .clamp( world_rect_pixels() );
}

Rect SmoothViewport::rendering_dest_rect() const {
  Rect dest;
  dest.x        = origin_on_screen_.x;
  dest.y        = origin_on_screen_.y;
  dest.w        = viewport_size_pixels().w;
  dest.h        = viewport_size_pixels().h;
  Rect viewport = get_bounds();
  auto [max_src_width, max_src_height] = world_size_pixels();
  if( viewport.w > max_src_width ) {
    double delta = ( double( viewport.w - max_src_width ) /
                     double( viewport.w ) ) *
                   double( viewport_size_pixels().w._ ) / 2;
    dest.x += int( delta );
    dest.w -= int( delta * 2 );
  }
  if( viewport.h > max_src_height ) {
    double delta = ( double( viewport.h - max_src_height ) /
                     double( viewport.h ) ) *
                   double( viewport_size_pixels().h._ ) / 2;
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
  if( from_visible_start.w >= visible_on_screen.w ||
      from_visible_start.h >= visible_on_screen.h ) {
    return nullopt;
  }

  double percent_x =
      double( from_visible_start.w._ ) / visible_on_screen.w._;
  double percent_y =
      double( from_visible_start.h._ ) / visible_on_screen.h._;

  DCHECK( percent_x >= 0 );
  DCHECK( percent_y >= 0 );

  auto viewport_or_world =
      get_bounds().clamp( world_rect_pixels() );

  auto res = Coord{X{int( viewport_or_world.x._ +
                          percent_x * viewport_or_world.w._ )},
                   Y{int( viewport_or_world.y._ +
                          percent_y * viewport_or_world.h._ )}};

  DCHECK( res.x >= 0_x && res.y >= 0_y );
  return res;
}

Opt<Coord> SmoothViewport::screen_pixel_to_world_tile(
    Coord pixel_coord ) const {
  auto maybe_pixel = screen_pixel_to_world_pixel( pixel_coord );
  if( !maybe_pixel.has_value() ) return {};
  return maybe_pixel.value() / g_tile_scale;
}

bool SmoothViewport::screen_coord_in_viewport(
    Coord pixel_coord ) const {
  return screen_pixel_to_world_pixel( pixel_coord ).has_value();
}

void SmoothViewport::scale_zoom( double factor ) {
  zoom_ *= factor;
  enforce_invariants();
}

double SmoothViewport::get_zoom() const { return zoom_; }

void SmoothViewport::pan( double down_up, double left_right,
                          bool scale ) {
  double factor = scale ? zoom_ : 1.0;
  center_x_ += left_right / factor;
  center_y_ += down_up / factor;
  enforce_invariants();
}

void SmoothViewport::pan_by_screen_coords( Delta delta ) {
  pan( double( delta.h ), double( delta.w ), /*scale=*/true );
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

// Determines if the square is fully visible in the viewport with
// respect to the coordinate in the C-dimension.
template<typename C>
bool is_tile_fully_visible( SmoothViewport const& vp,
                            Coord const&          coords ) {
  auto tile_rect       = Rect::from( coords, {1_w, 1_h} );
  auto tile_pixel_rect = tile_rect * g_tile_scale;
  auto covered         = vp.covered_pixels();
  return tile_pixel_rect.is_inside( covered );
}

// Determines if the surroundings of the coordinate in the
// C-dimension are fully visible in the viewport.  This is
// non-trivial because we have to apply less stringent rules
// as the coordinate gets closer to the edges of the world.
template<typename C>
bool are_tile_surroundings_as_fully_visible_as_can_be(
    SmoothViewport const& vp, Coord const& coords ) {
  auto is_in = [&coords]( Rect const& rect ) {
    auto start = rect.coordinate<C>();
    auto end   = rect.coordinate<C>() + rect.length<C>();
    return coords.coordinate<C>() >= start &&
           coords.coordinate<C>() < end;
  };

  bool visible_in_viewport       = is_in( vp.covered_tiles() );
  bool visible_in_inner_viewport = is_in(
      vp.covered_tiles().edges_removed().edges_removed() );
  bool in_inner_world = is_in(
      world_rect_tiles().edges_removed().edges_removed() );

  // If the unit is not at all visible to the player then
  // obviously we must return true.
  if( !visible_in_viewport ) return false;

  // The unit is visible somewhere on screen, but is it inside
  // the trimmed viewport (i.e., are its surrounding visible on
  // screen as well)? If so, then return true since that means
  // the tile and surroundings are visible.
  if( visible_in_inner_viewport ) return true;

  if( !in_inner_world ) {
    // At this point the tile surroundings may not be fully vis-
    // ible (as "visible" is defined here, meaning that it is in-
    // side a trimmed viewport) but if we are on the world's edge
    // then we can't do any better unless part of the tile itself
    // is not visible, in which case we can at least reveal that.
    // So there we return true if the tile is is not fully visi-
    // ble, false otherwise.
    return is_tile_fully_visible<C>( vp, coords );
  }

  // Here we have the case where the coord is somewhere in the
  // innards of the world (i.e., not at the edges), it is visible
  // on screen, but its surrounding squares are not visible on
  // screen. So by definition we return false.
  return false;
}

void SmoothViewport::ensure_tile_visible( Coord const& coord,
                                          bool         smooth ) {
  // Our approach here is to say the following: if the location
  // of the coord in a given dimension (either X or Y) is such
  // that its position (plus two surrounding squares) could be
  // better brought into view by panning the viewport then we
  // will pan the viewport in _both_ coordinates to center on the
  // unit. Panning both coordinates together makes for more
  // natural panning behavior when a unit is close to the corner
  // of the viewport.
  if( !are_tile_surroundings_as_fully_visible_as_can_be<X>(
          *this, coord ) ||
      !are_tile_surroundings_as_fully_visible_as_can_be<Y>(
          *this, coord ) ) {
    if( smooth ) {
      smooth_center_x_target_ =
          XD{double( ( coord.x * g_tile_width )._ )};
      smooth_center_y_target_ =
          YD{double( ( coord.y * g_tile_height )._ )};
    } else {
      center_on_tile_x( coord );
      center_on_tile_y( coord );
    }
  }
}

} // namespace rn
