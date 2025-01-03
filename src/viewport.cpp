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
#include "co-wait.hpp"
#include "error.hpp"
#include "tiles.hpp"

// ss
#include "ss/land-view.rds.hpp"
#include "ss/terrain.hpp"

// config
#include "config/rn.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/math.hpp"
#include "base/scope-exit.hpp"

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

ViewportController::ViewportController(
    TerrainState const& terrain, Viewport& o,
    gfx::rect const viewport_rect_pixels )
  : terrain_( terrain ),
    o_( o ),
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
    zoom_point_seek_{},
    point_seek_{} {
  if( o_.zoom == 0.0 )
    // This likely means that the Viewport serialized state has
    // not been initialized, so we'll do a favor here and ini-
    // tialize it, even though this is supposed to have been done
    // when creating a new game.
    o_.zoom = 1.0;
  update_logical_rect_cache( viewport_rect_pixels );
  fix_invariants();
}

void ViewportController::update_logical_rect_cache(
    gfx::rect const viewport_rect_pixels ) {
  viewport_rect_pixels_ = viewport_rect_pixels;
  fix_invariants();
}

void ViewportController::set_point_seek( Coord world_pixel ) {
  // Need to do this to stop any coro-based smooth panning.
  stop_auto_panning();
  point_seek_      = world_pixel;
  zoom_point_seek_ = nothing;
}

void ViewportController::set_point_seek_from_screen_pixel(
    Coord screen_pixel ) {
  maybe<Coord> world_pixel =
      screen_pixel_to_world_pixel( screen_pixel );
  if( !world_pixel ) return;
  set_point_seek( *world_pixel );
}

void ViewportController::advance_zoom_point_seek(
    DissipativeVelocity const& actual_zoom_vel ) {
  maybe<Coord> point_to_seek;
  DissipativeVelocity vel_to_use = actual_zoom_vel;
  if( point_seek_.has_value() ) {
    // We want to let the actual zoom velocity dictate the pan-
    // ning velocity while zooming in order for the animation to
    // look nice, but if the zooming has stopped then we still
    // need a boost to make sure that we reach the target.
    if( vel_to_use.to_double() < .3 )
      vel_to_use.set_velocity( .3 );
    point_to_seek = point_seek_;
  } else if( zoom_point_seek_.has_value() ) {
    // If we're only panning while zooming then don't manually
    // adjust the zoom velocity if it is zero since otherwise we
    // wouldn't stop panning even as the zooming stops.
    point_to_seek = zoom_point_seek_;
  } else {
    return;
  }
  auto log_zoom_change = log( 1.0 + vel_to_use.to_double() );
  // .98 found empirically, makes it slightly better, where "bet-
  // ter" means that the same pixel stays under the cursor as the
  // zooming happens.
  auto delta_x =
      .98 * ( ( point_to_seek->x - center_rounded().x ) *
              log_zoom_change );
  auto delta_y =
      .98 * ( ( point_to_seek->y - center_rounded().y ) *
              log_zoom_change );
  o_.center_x += delta_x;
  o_.center_y += delta_y;
  fix_invariants();
}

void ViewportController::advance( e_push_direction x_push,
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
                             DissipativeVelocity& vel,
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

void ViewportController::advance_state() {
  double const old_zoom = o_.zoom;

  advance( x_push_, y_push_, zoom_push_ );

  if( coro_smooth_scroll_.has_value() ) {
    advance_target_seeking( coro_smooth_scroll_->x_target,
                            o_.center_x, x_vel_,
                            translation_seeking_parameters );
    advance_target_seeking( coro_smooth_scroll_->y_target,
                            o_.center_y, y_vel_,
                            translation_seeking_parameters );
    if( is_tile_fully_visible(
            coro_smooth_scroll_->tile_target ) ) {
      // The promise will be nulled out after the associated
      // coroutine ends, but we may still be scrolling.
      if( coro_smooth_scroll_->promise != nullptr )
        coro_smooth_scroll_->promise
            ->set_value_emplace_if_not_set();
    }
  }

  if( smooth_zoom_target_ ) {
    if( advance_target_seeking( *smooth_zoom_target_, o_.zoom,
                                zoom_vel_,
                                zoom_seeking_parameters ) )
      smooth_zoom_target_ = nothing;
  }

  // Now if the player is requesting that the viewport pan to a
  // particular point on the map while zooming (the key here is
  // "while zoom"; regular panning is handled above) then we will
  // advance that here. The way it works is that the panning ve-
  // locity at any given time is tied to the zooming velocity so
  // that the animation looks natural. So we need to get the
  // zooming velocity. But we can't just use zoom_vel_ as is,
  // since that does not always hold the actual zoom velocity, it
  // only holds the zoom velocity from the push/acceleration
  // mechanism, and will not reflect changes to zoom made by the
  // "smooth zoom target" mechanism above. Therefore we just com-
  // pute the effective zoom velocity (which, unlike cartesian
  // coordinates, is not the difference but the ratio).
  double const new_zoom                  = o_.zoom;
  DissipativeVelocity effective_zoom_vel = zoom_vel_;
  // new_zoom = old_zoom * (1 + zoom_vel);
  effective_zoom_vel.set_velocity( new_zoom / old_zoom - 1.0 );
  advance_zoom_point_seek( effective_zoom_vel );

  x_push_    = e_push_direction::none;
  y_push_    = e_push_direction::none;
  zoom_push_ = e_push_direction::none;
  fix_invariants();
}

void ViewportController::set_x_push( e_push_direction push ) {
  x_push_ = push;
}

void ViewportController::set_y_push( e_push_direction push ) {
  y_push_ = push;
}

void ViewportController::set_zoom_push(
    e_push_direction push,
    maybe<Coord> maybe_seek_screen_coord ) {
  zoom_push_ = push;

  zoom_point_seek_ = nothing;
  point_seek_      = nothing;

  if( maybe_seek_screen_coord )
    zoom_point_seek_ =
        screen_pixel_to_world_pixel( *maybe_seek_screen_coord );
}

void ViewportController::set_zoom( double new_zoom ) {
  set_zoom_push( e_push_direction::none,
                 /*maybe_seek_screen_coord=*/nothing );
  o_.zoom = new_zoom;
  fix_invariants();
}

void ViewportController::smooth_zoom_target(
    double target, maybe<Coord> maybe_seek_screen_coord ) {
  smooth_zoom_target_ = target;

  if( maybe_seek_screen_coord )
    zoom_point_seek_ =
        screen_pixel_to_world_pixel( *maybe_seek_screen_coord );
}
void ViewportController::stop_auto_zoom() {
  smooth_zoom_target_ = nothing;
}

void ViewportController::stop_auto_panning() {
  if( coro_smooth_scroll_ ) {
    if( coro_smooth_scroll_->promise != nullptr )
      coro_smooth_scroll_->promise
          ->set_value_emplace_if_not_set();
    coro_smooth_scroll_ = nothing;
  }
  point_seek_ = nothing;
}

double ViewportController::min_zoom_for_no_border() const {
  auto min_zoom_for_x =
      double( viewport_rect_pixels_.delta().w ) /
      double( ( world_size_tiles() * g_tile_delta ).w );
  auto min_zoom_for_y =
      double( viewport_rect_pixels_.delta().h ) /
      double( ( world_size_tiles() * g_tile_delta ).h );
  return std::max( min_zoom_for_x, min_zoom_for_y );
}

double ViewportController::min_zoom_allowed() const {
  // Can this at 1.0. This is optional, but without this, for
  // super small maps (e.g. 4x4) the min zoom could be larger
  // than one, and would not allow the player to zoom out to 1.0
  // to make the sprites appear normal sized.
  return std::min( 1.0, optimal_min_zoom() /
                            config_rn.viewport.zoom_min_factor );
}

double ViewportController::x_world_pixels_in_viewport() const {
  return double( viewport_rect_pixels_.delta().w ) / o_.zoom;
}
double ViewportController::y_world_pixels_in_viewport() const {
  return double( viewport_rect_pixels_.delta().h ) / o_.zoom;
}

double ViewportController::start_x() const {
  return o_.center_x - x_world_pixels_in_viewport() / 2;
}
double ViewportController::start_y() const {
  return o_.center_y - y_world_pixels_in_viewport() / 2;
}
double ViewportController::end_x() const {
  return o_.center_x + x_world_pixels_in_viewport() / 2;
}
double ViewportController::end_y() const {
  return o_.center_y + y_world_pixels_in_viewport() / 2;
}
gfx::drect ViewportController::get_bounds() const {
  return {
    .origin = gfx::dpoint{ .x = start_x(), .y = start_y() },
    .size   = gfx::dsize{ .w = x_world_pixels_in_viewport(),
                          .h = y_world_pixels_in_viewport() } };
}

Rect ViewportController::get_bounds_rounded() const {
  gfx::drect dres = get_bounds();
  return Rect{ .x = int( dres.origin.x ),
               .y = int( dres.origin.y ),
               .w = int( dres.size.w ),
               .h = int( dres.size.h ) };
}

Coord ViewportController::center_rounded() const {
  return Coord{ X{ int( long( o_.center_x ) ) },
                Y{ int( long( o_.center_y ) ) } };
}

// These are to avoid a direct dependency on the screen module
// and its initialization code.
Delta ViewportController::world_size_pixels() const {
  return world_size_tiles() * 32;
}

Rect ViewportController::world_rect_pixels() const {
  return Rect::from( Coord{}, world_size_pixels() );
}

Rect ViewportController::world_rect_tiles() const {
  return terrain_.world_rect_tiles();
}

Delta ViewportController::world_size_tiles() const {
  return terrain_.world_size_tiles();
}

void ViewportController::fix_invariants() {
  o_.zoom = std::max( o_.zoom, min_zoom_allowed() );
  if( !config_rn.viewport.can_reveal_space_around_map )
    o_.zoom = std::max( o_.zoom, min_zoom_for_no_border() );
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
  if( x_world_pixels_in_viewport() <= size_x ) {
    if( start_x() < 0 )
      o_.center_x = x_world_pixels_in_viewport() / 2;
    if( end_x() > double( size_x ) )
      o_.center_x = double( size_x ) -
                    double( x_world_pixels_in_viewport() ) / 2;
  } else {
    o_.center_x = double( 0 + this->world_size_pixels().w / 2 );
  }
  if( y_world_pixels_in_viewport() <= size_y ) {
    if( start_y() < 0 )
      o_.center_y = y_world_pixels_in_viewport() / 2;
    if( end_y() > double( size_y ) )
      o_.center_y = double( size_y ) -
                    double( y_world_pixels_in_viewport() ) / 2;
  } else {
    o_.center_y = double( 0 + this->world_size_pixels().h / 2 );
  }
}

// Tiles touched by the viewport (tiles at the edge may only be
// partially visible).
Rect ViewportController::covered_tiles() const {
  // This needs to be consistent with covered_pixels(), hence we
  // compute it based on that.
  gfx::drect const pixels = covered_pixels();
  Rect res;
  res.x = int( pixels.origin.x / 32.0 );
  res.y = int( pixels.origin.y / 32.0 );
  res.w = int( pixels.right() / 32.0 ) - res.x;
  res.h = int( pixels.bottom() / 32.0 ) - res.y;
  if( fmod( pixels.right(), 32.0 ) > 0 ) ++res.w;
  if( fmod( pixels.bottom(), 32.0 ) > 0 ) ++res.h;
  res = res.clamp( world_rect_tiles() );
  return res;
}

bool ViewportController::are_surroundings_visible() const {
  return is_fully_visible_x() || is_fully_visible_y();
}

bool ViewportController::is_fully_visible_x() const {
  // Need to use pixels here instead of tiles because it is more
  // accurate.
  gfx::drect const covered = covered_pixels();
  return covered.origin.x == 0 &&
         covered.size.w == world_rect_pixels().w;
}

bool ViewportController::is_fully_visible_y() const {
  // Need to use pixels here instead of tiles because it is more
  // accurate.
  gfx::drect const covered = covered_pixels();
  return covered.origin.y == 0 &&
         covered.size.h == world_rect_pixels().h;
}

// Tiles that are fully visible. The rect returned here will be
// within the covered_tiles in general.
Rect ViewportController::fully_covered_tiles() const {
  // First round to the nearest pixel, then move the rectangle
  // inward to the nearest tile boundary (if we are not already
  // on one.
  Coord upper_left{ .x = base::round_up_to_nearest_int_multiple(
                        long( start_x() ), g_tile_width ),
                    .y = base::round_up_to_nearest_int_multiple(
                        long( start_y() ), g_tile_height ) };
  Coord lower_right{
    .x = base::round_down_to_nearest_int_multiple(
        long( end_x() ), g_tile_width ),
    .y = base::round_down_to_nearest_int_multiple(
        long( end_y() ), g_tile_height ) };
  return Rect::from( upper_left, lower_right ) / g_tile_delta;
}

gfx::drect ViewportController::covered_pixels() const {
  gfx::drect res =
      gfx::drect{ .origin = { .x = start_x(), .y = start_y() },
                  .size   = { .w = end_x() - start_x(),
                              .h = end_y() - start_y() } };
  UNWRAP_CHECK(
      clipped,
      res.clipped_by(
          gfx::rect( world_rect_pixels() ).to_double() ) );
  return clipped;
}

Rect ViewportController::covered_pixels_rounded() const {
  gfx::drect dres = covered_pixels();
  return Rect{ .x = int( dres.origin.x ),
               .y = int( dres.origin.y ),
               .w = int( dres.size.w ),
               .h = int( dres.size.h ) };
}

gfx::drect ViewportController::rendering_dest_rect() const {
  gfx::drect const viewport(
      gfx::rect( viewport_rect_pixels_ ).to_double() );
  gfx::dsize const world_size_screen_pixels =
      gfx::size( world_size_pixels() ).to_double() * get_zoom();
  gfx::dpoint const centered =
      centered_in( world_size_screen_pixels, viewport );
  gfx::drect res = viewport;
  if( is_fully_visible_x() ) {
    res.origin.x = centered.x;
    res.size.w   = world_size_screen_pixels.w;
  }
  if( is_fully_visible_y() ) {
    res.origin.y = centered.y;
    res.size.h   = world_size_screen_pixels.h;
  }
  CHECK_GE( res.origin.x, 0 );
  CHECK_GE( res.origin.y, 0 );
  CHECK_LE( res.right(), viewport_rect_pixels_.right_edge() );
  CHECK_LE( res.bottom(), viewport_rect_pixels_.bottom_edge() );
  return res;
}

Rect ViewportController::rendering_dest_rect_rounded() const {
  gfx::drect dres = rendering_dest_rect();
  return Rect{ .x = int( dres.origin.x ),
               .y = int( dres.origin.y ),
               .w = int( dres.size.w ),
               .h = int( dres.size.h ) };
}

gfx::dpoint
ViewportController::landscape_buffer_render_upper_left() const {
  gfx::drect const dest(
      gfx::rect( viewport_rect_pixels_ ).to_double() );
  gfx::dpoint const centered = centered_in(
      gfx::size( world_size_pixels() ).to_double() * get_zoom(),
      dest );
  gfx::dpoint res = centered;
  if( !is_fully_visible_x() )
    res.x = viewport_rect_pixels_.x - start_x() * get_zoom();
  if( !is_fully_visible_y() )
    res.y = viewport_rect_pixels_.y - start_y() * get_zoom();
  return res;
}

maybe<Coord> ViewportController::screen_pixel_to_world_pixel(
    Coord pixel_coord ) const {
  Rect visible_on_screen = rendering_dest_rect_rounded();
  auto from_visible_start =
      pixel_coord - visible_on_screen.upper_left();
  if( from_visible_start.w < 0 || from_visible_start.h < 0 ) {
    return nothing;
  }
  if( from_visible_start.w >= visible_on_screen.w ||
      from_visible_start.h >= visible_on_screen.h ) {
    return nothing;
  }

  double percent_x =
      double( from_visible_start.w + .5 ) / visible_on_screen.w;
  double percent_y =
      double( from_visible_start.h + .5 ) / visible_on_screen.h;

  DCHECK( percent_x >= 0 );
  DCHECK( percent_y >= 0 );

  auto viewport_or_world =
      get_bounds_rounded().clamp( this->world_rect_pixels() );

  auto res = Coord{
    X{ int( long( viewport_or_world.x +
                  percent_x * viewport_or_world.w ) ) },
    Y{ int( long( viewport_or_world.y +
                  percent_y * viewport_or_world.h ) ) } };

  DCHECK( res.x >= 0 && res.y >= 0 );
  return res;
}

double ViewportController::optimal_min_zoom() const {
  Delta world_pixels    = world_size_pixels();
  Delta viewport_pixels = viewport_rect_pixels_.delta();
  double optimal_zoom_for_x =
      double( viewport_pixels.w ) / world_pixels.w;
  double optimal_zoom_for_y =
      double( viewport_pixels.h ) / world_pixels.h;
  double res =
      std::min( optimal_zoom_for_x, optimal_zoom_for_y );
  // Leave some border around it.
  return res * .93;
}

Coord ViewportController::world_tile_to_world_pixel_center(
    Coord world_tile ) const {
  return world_tile * g_tile_delta + g_tile_delta / 2;
}

maybe<Coord> ViewportController::world_tile_to_screen_pixel(
    Coord world_tile ) const {
  return world_pixel_to_screen_pixel(
      world_tile_to_world_pixel_center( world_tile ) );
}

maybe<Coord> ViewportController::world_pixel_to_screen_pixel(
    Coord world_pixel ) const {
  Rect covered_pixels = this->covered_pixels_rounded();
  if( !world_pixel.is_inside( covered_pixels ) ) return nothing;
  double x_percent =
      double( ( world_pixel.x - covered_pixels.x ) ) /
      covered_pixels.w;
  double y_percent =
      double( ( world_pixel.y - covered_pixels.y ) ) /
      covered_pixels.h;
  Rect dst = rendering_dest_rect_rounded();
  dst.w    = static_cast<int>( dst.w * x_percent );
  dst.h    = static_cast<int>( dst.h * y_percent );
  return dst.lower_right();
}

maybe<Coord> ViewportController::screen_pixel_to_world_tile(
    Coord pixel_coord ) const {
  auto maybe_pixel = screen_pixel_to_world_pixel( pixel_coord );
  if( !maybe_pixel.has_value() ) return {};
  return maybe_pixel.value() / g_tile_delta;
}

bool ViewportController::screen_coord_in_viewport(
    Coord pixel_coord ) const {
  return screen_pixel_to_world_pixel( pixel_coord ).has_value();
}

void ViewportController::scale_zoom( double factor ) {
  o_.zoom *= factor;
  fix_invariants();
}

double ViewportController::get_zoom() const { return o_.zoom; }

void ViewportController::pan( double down_up, double left_right,
                              bool scale ) {
  double factor = scale ? o_.zoom : 1.0;
  o_.center_x += left_right / factor;
  o_.center_y += down_up / factor;
  fix_invariants();
}

void ViewportController::pan_by_screen_coords( Delta delta ) {
  pan( double( delta.h ), double( delta.w ), /*scale=*/true );
}

void ViewportController::pan_by_world_coords( Delta delta ) {
  pan( double( delta.h ), double( delta.w ), /*scale=*/false );
}

void ViewportController::pan_by_world_coords( gfx::dsize size ) {
  pan( size.h, size.w, /*scale=*/false );
}

void ViewportController::center_on_tile_x(
    Coord const& coords ) {
  o_.center_x =
      double( coords.x * g_tile_width + g_tile_width / 2 );
  fix_invariants();
}

void ViewportController::center_on_tile_y(
    Coord const& coords ) {
  o_.center_y =
      double( coords.y * g_tile_height + g_tile_height / 2 );
  fix_invariants();
}

void ViewportController::center_on_tile( Coord const& coords ) {
  center_on_tile_x( coords );
  center_on_tile_y( coords );
  fix_invariants();
}

wait<> ViewportController::center_on_tile_smooth( Coord coord ) {
  stop_auto_panning();
  if( is_tile_too_far( coord ) ) {
    // If it's too far from the current view then we'll forgo
    // smooth scrolling otherwise it can get annoying to the
    // player.
    center_on_tile( coord );
    co_return;
  }
  wait_promise<> p;
  coro_smooth_scroll_ = SmoothScroll{
    .x_target =
        double( ( coord.x * g_tile_width + g_tile_width / 2 ) ),
    .y_target = double(
        ( coord.y * g_tile_height + g_tile_height / 2 ) ),
    .tile_target = coord,
    .promise     = &p };
  // When the coroutine ends we will null out the promise so that
  // no one tries to access it (because it will be gone) but we
  // don't reset the coro_smooth_scroll_ variable, that way it
  // can continue to finish scrolling. That said, we need to be
  // defensive in how we do that because it is possible that
  // someone could call e.g. stop_auto_panning while this corou-
  // tine is alive, which would reset coro_smooth_scroll_. Also,
  // it is possible that someone invoked another instance of this
  // coroutine while we were waiting, in which case the promise
  // will be theirs and hence we wouldn't want to reset it.
  // TODO: think about potentially a better way to do this. It is
  // a bit tricky because we want the scrolling to slow as it
  // nears its final point, but we don't want to hold up the
  // coroutine for all that time, hence current behavior.
  // TODO: also think about a better way to handle when multiple
  // of these requires are fired concurrently; we probably want
  // to represent the state in a map or stack.
  SCOPE_EXIT {
    if( coro_smooth_scroll_.has_value() &&
        coro_smooth_scroll_->promise == &p )
      coro_smooth_scroll_->promise = nullptr;
  };
  co_await p.wait();
}

bool ViewportController::is_tile_fully_visible(
    Coord const& coord ) const {
  Rect box = Rect::from( coord * g_tile_delta, g_tile_delta );
  return box.is_inside( get_bounds_rounded() );
}

// Determines if the square is fully visible in the viewport with
// respect to the coordinate in the C-dimension.
template<typename C>
bool is_tile_fully_visible( ViewportController const& vp,
                            Coord const& coords ) {
  auto tile_rect = Rect::from( coords, Delta{ .w = 1, .h = 1 } );
  auto tile_pixel_rect = tile_rect * g_tile_delta;
  auto covered         = vp.covered_pixels_rounded();
  return tile_pixel_rect.is_inside( covered );
}

// Determines if two tiles of surroundings in each direction of
// the coordinate in the C-dimension are fully visible in the
// viewport. This is non-trivial because we have to apply less
// stringent rules as the coordinate gets closer to the edges of
// the world.
template<typename C>
bool are_tile_surroundings_as_fully_visible_as_can_be(
    ViewportController const& vp, Coord const& coords ) {
  auto is_in = [&coords]( Rect const& rect ) {
    auto start = rect.coordinate<C>();
    auto end   = rect.coordinate<C>() + rect.length<C>();
    return coords.coordinate<C>() >= start &&
           coords.coordinate<C>() < end;
  };

  // We have to remove edges in the same way from both the vis-
  // ible region and the world rect in order for the below to
  // work properly at the world's edge.
  auto remove_edges = []( Rect r ) {
    return r.edges_removed(); //.edges_removed();
  };

  bool visible_in_inner_viewport =
      is_in( remove_edges( vp.fully_covered_tiles() ) );
  bool on_world_border =
      !is_in( remove_edges( vp.world_rect_tiles() ) );

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

bool ViewportController::need_to_scroll_to_reveal_tile(
    Coord const& coord ) const {
  // Our approach here is to say the following: if the location
  // of the coord in a given dimension (either X or Y) is such
  // that its position (plus some surrounding squares) could be
  // better brought into view by panning the viewport then we
  // will pan the viewport in _both_ coordinates to center on the
  // unit. Panning both coordinates together makes for more nat-
  // ural panning behavior when a unit is close to the corner of
  // the viewport.
  return !are_tile_surroundings_as_fully_visible_as_can_be<
             DimensionX>( *this, coord ) ||
         !are_tile_surroundings_as_fully_visible_as_can_be<
             DimensionY>( *this, coord );
}

void ViewportController::ensure_tile_visible(
    Coord const& coord ) {
  stop_auto_panning();
  if( !need_to_scroll_to_reveal_tile( coord ) ) return;
  // FIXME: we should not be centering here, we should only be
  // scrolling enough to make the unit visible, otherwise two
  // units that are currently moving that could fit within one
  // screen (but are far enough apart) can cause a thrashing be-
  // havior as the viewport alternately centers on each one as
  // each takes its turn. See similar comment below.
  center_on_tile_x( coord );
  center_on_tile_y( coord );
}

bool ViewportController::is_tile_too_far( Coord tile ) const {
  gfx::rect const covered = covered_tiles();
  if( tile.is_inside( covered.with_edges_removed() ) )
    return false;
  double const dist = ( tile - covered.center() ).diagonal();
  double const threshold =
      covered.size.pythagorean() / 2.0 *
      config_rn.viewport.smooth_scroll_threshold;
  return dist > threshold;
}

wait<> ViewportController::ensure_tile_visible_smooth(
    Coord const& coord ) {
  if( !need_to_scroll_to_reveal_tile( coord ) ) co_return;
  // FIXME: we should not be centering here, we should only be
  // scrolling enough to make the unit visible, otherwise two
  // units that are currently moving that could fit within one
  // screen (but are far enough apart) can cause a thrashing be-
  // havior as the viewport alternately centers on each one as
  // each takes its turn. See similar comment above.
  co_await center_on_tile_smooth( coord );
}

bool ViewportController::operator==(
    ViewportController const& rhs ) const {
  // Don't include transient fields here since they won't be de-
  // serialized, thus if we include them in the comparison then
  // this object won't have equality before/after saving/loading.
  return o_ == rhs.o_;
}

} // namespace rn