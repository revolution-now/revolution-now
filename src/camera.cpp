/****************************************************************
**camera.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-19.
*
* Description: Controls the camera on the land view.
*
*****************************************************************/
#include "camera.hpp"

// Revolution Now
#include "iuser-config.hpp"

// config
#include "config/land-view.rds.hpp"
#include "config/user.rds.hpp"

// ss
#include "ss/land-view.rds.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::gfx::size;

/****************************************************************
** Constants.
*****************************************************************/
double constexpr kLogZoomTolerance = .2;

/****************************************************************
** Helpers.
*****************************************************************/
bool eq( double const l, double const r,
         double const tolerance = .0001 ) {
  return abs( l - r ) <= tolerance;
}

bool zoom_equiv( double const l, double const r ) {
  return eq( log2( l ), log2( r ), kLogZoomTolerance );
}

double zoom_min() {
  static double const o =
      exp2( config_land_view.camera.zoom_log2_min );
  return o;
}

double zoom_max() {
  static double const o =
      exp2( config_land_view.camera.zoom_log2_max );
  return o;
}

} // namespace

/****************************************************************
** Camera.
*****************************************************************/
Camera::Camera( IUserConfig const& user_config, Viewport& camera,
                size const map_size_tiles )
  : map_size_tiles_( map_size_tiles ),
    user_config_( user_config ),
    camera_( camera ) {
  CHECK( map_size_tiles.area() > 0 );
}

auto const& Camera::static_config() {
  return config_land_view.camera;
}

auto const& Camera::user_config() {
  return user_config_.read().camera;
}

void Camera::fix_broken_invariants() {
  camera_.zoom = clamp( camera_.zoom, zoom_min(), zoom_max() );

  if( !user_config().can_zoom_positive )
    camera_.zoom = std::min( camera_.zoom, 1.0 );
}

ZoomChanged Camera::zoom_out() {
  double const before = camera_.zoom;

  double const closest = exp2( floor( log2( camera_.zoom ) ) );
  if( !zoom_equiv( camera_.zoom, closest ) )
    // Just moving it to the nearest factor of two has moved it
    // enough.
    camera_.zoom = closest;
  else
    camera_.zoom = closest / 2;

  fix_broken_invariants();
  return ZoomChanged{
    .value_changed  = ( camera_.zoom != before ),
    .bucket_changed = !zoom_equiv( camera_.zoom, before ) };
}

ZoomChanged Camera::zoom_in() {
  double const before = camera_.zoom;

  double const closest = exp2( ceil( log2( camera_.zoom ) ) );
  if( !zoom_equiv( camera_.zoom, closest ) )
    // Just moving it to the nearest factor of two has moved
    // it enough.
    camera_.zoom = closest;
  else
    camera_.zoom = closest * 2;

  fix_broken_invariants();
  return ZoomChanged{
    .value_changed  = ( camera_.zoom != before ),
    .bucket_changed = !zoom_equiv( camera_.zoom, before ) };
}

void Camera::center_on_tile( point const tile ) {
  SCOPE_EXIT { fix_broken_invariants(); };

  camera_.center_x = tile.x * 32 + 16;
  camera_.center_y = tile.y * 32 + 16;
}

size Camera::map_dimensions_tiles() const {
  return map_size_tiles_;
}

} // namespace rn
