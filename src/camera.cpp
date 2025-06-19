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

/****************************************************************
** Helpers.
*****************************************************************/
bool eq( double const l, double const r,
         double const tolerance = .0001 ) {
  return ( abs( l - r ) <= tolerance );
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
Camera::Camera( IUserConfig const& user_config,
                Viewport& camera )
  : user_config_( user_config ), camera_( camera ) {}

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

void Camera::zoom_out() {
  SCOPE_EXIT { fix_broken_invariants(); };

  double const closest = exp2( floor( log2( camera_.zoom ) ) );
  if( !eq( log2( camera_.zoom ), log2( closest ), .2 ) ) {
    // Just moving it to the nearest factor of two has moved
    // it enough.
    camera_.zoom = closest;
    return;
  }
  camera_.zoom = exp2( log2( closest ) - 1 );
}

void Camera::zoom_in() {
  SCOPE_EXIT { fix_broken_invariants(); };

  double const closest = exp2( ceil( log2( camera_.zoom ) ) );
  if( !eq( log2( camera_.zoom ), log2( closest ), .2 ) ) {
    // Just moving it to the nearest factor of two has moved
    // it enough.
    camera_.zoom = closest;
    return;
  }
  camera_.zoom = exp2( log2( closest ) + 1 );
}

void Camera::center_on_tile( point const tile ) {
  SCOPE_EXIT { fix_broken_invariants(); };

  camera_.center_x = tile.x * 32 + 16;
  camera_.center_y = tile.y * 32 + 16;
}

} // namespace rn
