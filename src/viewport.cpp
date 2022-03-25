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
#include "config-files.hpp"
#include "error.hpp"
#include "math.hpp"
#include "tiles.hpp"

// Revolution Now (config)
#include "../config/rcl/rn.inl"

using namespace std;

namespace rn {

namespace {

double pan_accel_init() {
  return config_rn.viewport.pan_accel_init_coeff *
         config_rn.viewport.pan_speed;
}

double pan_accel_drag_init() {
  return config_rn.viewport.pan_accel_drag_init_coeff *
         config_rn.viewport.pan_speed;
}

} // namespace

SmoothViewport::SmoothViewport()
  : SmoothViewport( wrapped::SmoothViewport{
        .zoom     = 1.0,
        .center_x = 0.0,
        .center_y = 0.0,
    } ) {}

SmoothViewport::SmoothViewport( wrapped::SmoothViewport&& o )
  : o_( std::move( o ) ),
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
    smooth_zoom_target_{},
    smooth_center_{},
    zoom_point_seek_{},
    viewport_rect_pixels_{},
    world_size_tiles_{} {
  // We don't call fix_invariants here because that method de-
  // pends on some of the other (non-serialized) state being set,
  // such as world_size_tiles_.
}

base::valid_or<string> wrapped::SmoothViewport::validate()
    const {
  REFL_VALIDATE( zoom >= 0.0, "zoom must be larger than zero" );
  REFL_VALIDATE( zoom >= 0.0, "zoom must be less than one" );
  REFL_VALIDATE( center_x >= 0.0,
                 "x center must be larger than 0" );
  REFL_VALIDATE( center_y >= 0.0,
                 "y center must be larger than 0" );
  return base::valid;
}

void SmoothViewport::advance( e_push_direction x_push,
                              e_push_direction y_push,
                              e_push_direction zoom_push ) {
  double zoom_factor07  = pow( get_zoom(), 0.7 );
  double zoom_factor15  = pow( get_zoom(), 1.5 );
  double pan_accel      = pan_accel_init();
  double pan_accel_drag = pan_accel_drag_init();
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
    o_.center_x += delta_x;
    o_.center_y += delta_y;
    fix_invariants();
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
NOTHROW_MOVE( TargetingRates );

// These numbers were chosen through iterative trials to yield
// the nicest looking result, which is to say that the transition
// will quickly start approaching its target (faster the farther
// away it is) and then slow down as it nears the target, then
// when it is within a critical distance it will start moving at
// a linear rate so that it doesn't take too long to get there.
constexpr TargetingRates translation_seeking_parameters{
    /*rate=*/.90,
    /*shift=*/1.0,
    /*linear_window=*/8.0 };
constexpr TargetingRates zoom_seeking_parameters{
    /*rate=*/.95,
    /*shift=*/.001,
    /*linear_window=*/.015 };

// This function will take a numerical value that is being gradu-
// ally moved to a target value (in a somewhat asymptotic manner
// in that the movement slows as the target nears) and will ad-
// vance it by one "frame". When it has reached the target this
// function will signal (by returning true) that the movement can
// stop.
template<typename T>
bool advance_target_seeking( T target_T, double& val,
                             DissipativeVelocity&  vel,
                             TargetingRates const& params ) {
  double target = double( target_T );
  if( val == target ) return true;
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
    return true;
  }
  return false;
}

void SmoothViewport::advance_state(
    Rect const& viewport_rect_pixels ) {
  viewport_rect_pixels_ = viewport_rect_pixels;
  fix_invariants();

  advance( x_push_, y_push_, zoom_push_ );

  if( smooth_center_ ) {
    advance_target_seeking( smooth_center_->x_target,
                            o_.center_x, x_vel_,
                            translation_seeking_parameters );
    advance_target_seeking( smooth_center_->y_target,
                            o_.center_y, y_vel_,
                            translation_seeking_parameters );
    if( is_tile_fully_visible( smooth_center_->tile_target ) )
      smooth_center_->promise.set_value_emplace_if_not_set();
  }

  if( smooth_zoom_target_ ) {
    if( advance_target_seeking( *smooth_zoom_target_, o_.zoom,
                                zoom_vel_,
                                zoom_seeking_parameters ) )
      smooth_zoom_target_ = nothing;
  }

  x_push_    = e_push_direction::none;
  y_push_    = e_push_direction::none;
  zoom_push_ = e_push_direction::none;
  fix_invariants();
}

void SmoothViewport::set_x_push( e_push_direction push ) {
  x_push_ = push;
}

void SmoothViewport::set_y_push( e_push_direction push ) {
  y_push_ = push;
}

void SmoothViewport::set_zoom_push(
    e_push_direction push,
    maybe<Coord>     maybe_seek_screen_coord ) {
  zoom_push_ = push;

  zoom_point_seek_ = nothing;
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
  smooth_zoom_target_ = nothing;
}

void SmoothViewport::stop_auto_panning() {
  if( !smooth_center_ ) return;
  smooth_center_->promise.set_value_emplace_if_not_set();
  smooth_center_ = nothing;
}

// Computes the critical zoom point below which (i.e., if you
// were to zoom out a bit further) would be revealed space around
// the map.
double SmoothViewport::minimum_zoom_for_viewport() {
  auto min_zoom_for_x =
      double( viewport_rect_pixels_.delta().w ) /
      double( ( world_size_tiles_ * g_tile_scale ).w );
  auto min_zoom_for_y =
      double( viewport_rect_pixels_.delta().h ) /
      double( ( world_size_tiles_ * g_tile_scale ).h );
  return std::max( min_zoom_for_x, min_zoom_for_y );
}

double SmoothViewport::x_world_pixels_in_viewport() const {
  return double( viewport_rect_pixels_.delta().w ) / o_.zoom;
}
double SmoothViewport::y_world_pixels_in_viewport() const {
  return double( viewport_rect_pixels_.delta().h ) / o_.zoom;
}

double SmoothViewport::start_x() const {
  return o_.center_x - x_world_pixels_in_viewport() / 2;
}
double SmoothViewport::start_y() const {
  return o_.center_y - y_world_pixels_in_viewport() / 2;
}
double SmoothViewport::end_x() const {
  return o_.center_x + x_world_pixels_in_viewport() / 2;
}
double SmoothViewport::end_y() const {
  return o_.center_y + y_world_pixels_in_viewport() / 2;
}
X SmoothViewport::start_tile_x() const {
  return X( int( start_x() ) ) / g_tile_width;
}
Y SmoothViewport::start_tile_y() const {
  return Y( int( start_y() ) ) / g_tile_height;
}

Rect SmoothViewport::get_bounds() const {
  return { X( int( start_x() ) ), Y( int( start_y() ) ),
           W( int( x_world_pixels_in_viewport() ) ),
           H( int( y_world_pixels_in_viewport() ) ) };
}

Coord SmoothViewport::center_rounded() const {
  return Coord{ X{ int( lround( o_.center_x ) ) },
                Y{ int( lround( o_.center_y ) ) } };
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double SmoothViewport::width_tiles() const {
  int lower = round_down_to_nearest_int_multiple(
      start_x(), g_tile_width._ );
  int upper = round_up_to_nearest_int_multiple( end_x(),
                                                g_tile_width._ );
  return ( upper - lower ) / g_tile_width._;
}

// Number of tiles needed to be drawn in order to subsume the
// half-open viewport range [start, end).
double SmoothViewport::height_tiles() const {
  int lower = round_down_to_nearest_int_multiple(
      start_y(), g_tile_height._ );
  int upper = round_up_to_nearest_int_multiple(
      end_y(), g_tile_height._ );
  return ( upper - lower ) / g_tile_height._;
}

void SmoothViewport::set_max_viewable_size_tiles( Delta size ) {
  world_size_tiles_ = size;
}

// These are to avoid a direct dependency on the screen module
// and its initialization code.
Delta SmoothViewport::world_size_pixels() const {
  return world_size_tiles_ * Scale{ 32 };
}

Rect SmoothViewport::world_rect_pixels() const {
  return Rect::from( Coord{}, world_size_pixels() );
}

Rect SmoothViewport::world_rect_tiles() const {
  return Rect::from( Coord{}, world_size_tiles_ );
}

void SmoothViewport::fix_invariants() {
  o_.zoom = std::max( o_.zoom, config_rn.viewport.zoom_min );
  if( !config_rn.viewport.can_reveal_space_around_map )
    o_.zoom = std::max( o_.zoom, minimum_zoom_for_viewport() );
  auto [size_x, size_y] = world_size_tiles_;
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
  if( x_world_pixels_in_viewport() <= size_x ) {
    if( start_x() < 0 )
      o_.center_x = x_world_pixels_in_viewport() / 2;
    if( end_x() > double( size_x ) )
      o_.center_x = double( size_x ) -
                    double( x_world_pixels_in_viewport() ) / 2;
  } else {
    o_.center_x =
        double( 0_x + this->world_size_pixels().w / 2_sx );
  }
  if( y_world_pixels_in_viewport() <= size_y ) {
    if( start_y() < 0 )
      o_.center_y = y_world_pixels_in_viewport() / 2;
    if( end_y() > double( size_y ) )
      o_.center_y = double( size_y ) -
                    double( y_world_pixels_in_viewport() ) / 2;
  } else {
    o_.center_y =
        double( 0_y + this->world_size_pixels().h / 2_sy );
  }
}

// Tiles touched by the viewport (tiles at the edge may only be
// partially visible).
Rect SmoothViewport::covered_tiles() const {
  return Rect{
      X( start_tile_x() ), Y( start_tile_y() ),
      W{ static_cast<int>( lround( width_tiles() ) ) },
      H{ static_cast<int>( lround( height_tiles() ) ) } }
      .clamp( this->world_rect_tiles() );
}

// Tiles that are fully visible. The rect returned here will be
// within the covered_tiles in general.
Rect SmoothViewport::fully_covered_tiles() const {
  // First round to the nearest pixel, then move the rectangle
  // inward to the nearest tile boundary (if we are not already
  // on one.
  Coord upper_left(
      X{ round_up_to_nearest_int_multiple( lround( start_x() ),
                                           g_tile_width._ ) },
      Y{ round_up_to_nearest_int_multiple( lround( start_y() ),
                                           g_tile_height._ ) } );
  Coord lower_right(
      X{ round_down_to_nearest_int_multiple( lround( end_x() ),
                                             g_tile_width._ ) },
      Y{ round_down_to_nearest_int_multiple(
          lround( end_y() ), g_tile_height._ ) } );
  return Rect::from( upper_left, lower_right ) / g_tile_scale;
}

Rect SmoothViewport::covered_pixels() const {
  X x_start{ static_cast<int>( start_x() ) };
  Y y_start{ static_cast<int>( start_y() ) };
  X x_end{ static_cast<int>( lround( end_x() ) ) };
  Y y_end{ static_cast<int>( lround( end_y() ) ) };

  return Rect{ x_start, y_start, x_end - x_start,
               y_end - y_start }
      .clamp( this->world_rect_pixels() );
}

Rect SmoothViewport::rendering_src_rect() const {
  Rect viewport = get_bounds();
  return Rect::from(
             Coord{} + viewport.upper_left() % g_tile_scale,
             viewport.delta() )
      .clamp( this->world_rect_pixels() );
}

Rect SmoothViewport::rendering_dest_rect() const {
  Rect dest     = viewport_rect_pixels_;
  Rect viewport = get_bounds();
  auto [max_src_width, max_src_height] =
      this->world_size_pixels();
  if( viewport.w > max_src_width ) {
    double delta = ( double( viewport.w - max_src_width ) /
                     double( viewport.w ) ) *
                   double( viewport_rect_pixels_.delta().w._ ) /
                   2;
    dest.x += int( delta );
    dest.w -= int( delta * 2 );
  }
  if( viewport.h > max_src_height ) {
    double delta = ( double( viewport.h - max_src_height ) /
                     double( viewport.h ) ) *
                   double( viewport_rect_pixels_.delta().h._ ) /
                   2;
    dest.y += int( delta );
    dest.h -= int( delta * 2 );
  }
  return dest;
}

maybe<Coord> SmoothViewport::screen_pixel_to_world_pixel(
    Coord pixel_coord ) const {
  Rect visible_on_screen = rendering_dest_rect();
  auto from_visible_start =
      pixel_coord - visible_on_screen.upper_left();
  if( from_visible_start.w < 0_w ||
      from_visible_start.h < 0_h ) {
    return nothing;
  }
  if( from_visible_start.w >= visible_on_screen.w ||
      from_visible_start.h >= visible_on_screen.h ) {
    return nothing;
  }

  double percent_x =
      double( from_visible_start.w._ ) / visible_on_screen.w._;
  double percent_y =
      double( from_visible_start.h._ ) / visible_on_screen.h._;

  DCHECK( percent_x >= 0 );
  DCHECK( percent_y >= 0 );

  auto viewport_or_world =
      get_bounds().clamp( this->world_rect_pixels() );

  auto res =
      Coord{ X{ int( viewport_or_world.x._ +
                     percent_x * viewport_or_world.w._ ) },
             Y{ int( viewport_or_world.y._ +
                     percent_y * viewport_or_world.h._ ) } };

  DCHECK( res.x >= 0_x && res.y >= 0_y );
  return res;
}

maybe<Coord> SmoothViewport::screen_pixel_to_world_tile(
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
  o_.zoom *= factor;
  fix_invariants();
}

double SmoothViewport::get_zoom() const { return o_.zoom; }

void SmoothViewport::pan( double down_up, double left_right,
                          bool scale ) {
  double factor = scale ? o_.zoom : 1.0;
  o_.center_x += left_right / factor;
  o_.center_y += down_up / factor;
  fix_invariants();
}

void SmoothViewport::pan_by_screen_coords( Delta delta ) {
  pan( double( delta.h ), double( delta.w ), /*scale=*/true );
}

void SmoothViewport::center_on_tile_x( Coord const& coords ) {
  o_.center_x =
      double( coords.x * g_tile_width + g_tile_width._ / 2 );
  fix_invariants();
}

void SmoothViewport::center_on_tile_y( Coord const& coords ) {
  o_.center_y =
      double( coords.y * g_tile_height + g_tile_height._ / 2 );
  fix_invariants();
}

void SmoothViewport::center_on_tile( Coord const& coords ) {
  center_on_tile_x( coords );
  center_on_tile_y( coords );
  fix_invariants();
}

bool SmoothViewport::is_tile_fully_visible(
    Coord const& coord ) const {
  Rect box = Rect::from( coord * g_tile_scale, g_tile_delta );
  return box.is_inside( get_bounds() );
}

// Determines if the square is fully visible in the viewport with
// respect to the coordinate in the C-dimension.
template<typename C>
bool is_tile_fully_visible( SmoothViewport const& vp,
                            Coord const&          coords ) {
  auto tile_rect       = Rect::from( coords, { 1_w, 1_h } );
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

  // The use of fully_covered_tiles will ensure that we eliminate
  // any partial tiles at the boundary, then remove another layer
  // of edge.
  bool visible_in_inner_viewport =
      is_in( vp.fully_covered_tiles().edges_removed() );
  bool on_world_border =
      !is_in( vp.world_rect_tiles().edges_removed() );

  if( visible_in_inner_viewport ) return true;

  if( on_world_border ) {
    // We on are the world's border. The best we can do here is
    // to make sure that the tile itself is fully visible; we
    // can't do much about the surroundings.
    return is_tile_fully_visible<C>( vp, coords );
  }

  // Here we have the case where the coord is somewhere in the
  // innards of the world (i.e., not at the edges), it is visible
  // on screen, but its surrounding squares are not visible on
  // screen. So by definition we return false.
  return false;
}

bool SmoothViewport::need_to_scroll_to_reveal_tile(
    Coord const& coord ) const {
  // Our approach here is to say the following: if the location
  // of the coord in a given dimension (either X or Y) is such
  // that its position (plus two surrounding squares) could be
  // better brought into view by panning the viewport then we
  // will pan the viewport in _both_ coordinates to center on the
  // unit. Panning both coordinates together makes for more
  // natural panning behavior when a unit is close to the corner
  // of the viewport.
  return !are_tile_surroundings_as_fully_visible_as_can_be<X>(
             *this, coord ) ||
         !are_tile_surroundings_as_fully_visible_as_can_be<Y>(
             *this, coord );
}

void SmoothViewport::ensure_tile_visible( Coord const& coord ) {
  stop_auto_panning();
  if( !need_to_scroll_to_reveal_tile( coord ) ) return;
  center_on_tile_x( coord );
  center_on_tile_y( coord );
}

wait<> SmoothViewport::ensure_tile_visible_smooth(
    Coord const& coord ) {
  // FIXME: this seems to never finish if the viewport is zoomed
  // in too far (to where a unit and its surroundings are not
  // fully visible).
  stop_auto_panning();
  if( !need_to_scroll_to_reveal_tile( coord ) )
    return make_wait<>();
  smooth_center_ = SmoothCenter{
      .x_target = XD{ double( ( coord.x * g_tile_width )._ ) },
      .y_target = YD{ double( ( coord.y * g_tile_height )._ ) },
      .tile_target = coord,
      .promise     = {} };
  return smooth_center_->promise.wait();
}

} // namespace rn