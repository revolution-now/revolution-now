/****************************************************************
* physics.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-09.
*
* Description: Handles anything in the game that relies on physics
*              or physics-like actions.
*
*****************************************************************/
#include "physics.hpp"

#include "base-util.hpp"
#include "macros.hpp"

namespace rn {

namespace {


  
} // namespace

DissipativeVelocity::DissipativeVelocity( double min_velocity,
                                          double max_velocity,
                                          double initial_velocity,
                                          double acceleration,
                                          double drag_acceleration )
  : min_velocity_( min_velocity ),
    max_velocity_( max_velocity ),
    velocity_( initial_velocity ),
    accel_( acceleration ),
    drag_accel_( drag_acceleration )
{
  ASSERT( velocity_ >= min_velocity );
  ASSERT( velocity_ <= max_velocity );

  // We must have 0 <= drag_accel_ < accel_
  ASSERT( drag_accel_ >= 0 );
  ASSERT( accel_ > drag_accel_ );
}

double DissipativeVelocity::advance( e_push_direction direction ) {
  switch( direction ) {
    case e_push_direction::negative:
      velocity_ = (velocity_ <= min_velocity_)
                ? min_velocity_
                : (velocity_-(accel_-drag_accel_));
      break;
    case e_push_direction::positive:
      velocity_ = (velocity_ >= max_velocity_)
                ? max_velocity_
                : (velocity_+(accel_-drag_accel_));
      break;
    case e_push_direction::none:
      // In this case there is no "force" acting on the value, so
      // we apply a "drag" force to decrease the magnitude of the
      // velocity.
      if( velocity_ > 0 ) {
        velocity_ -= drag_accel_;
        if( velocity_ < 0 ) velocity_ = 0;
      } else if( velocity_ < 0 ) {
        velocity_ += drag_accel_;
        if( velocity_ > 0 ) velocity_ = 0;
      }
      break;
  };
  return velocity_;
}

} // namespace rn
