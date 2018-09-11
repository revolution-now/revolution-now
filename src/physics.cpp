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

void DissipativeVelocity::set_bounds(
    double min_velocity, double max_velocity ) {
  ASSERT( min_velocity < max_velocity );
  min_velocity_ = min_velocity;
  max_velocity_ = max_velocity;
  if( velocity_ < min_velocity )
    velocity_ = min_velocity;
  if( velocity_ > max_velocity )
    velocity_ = max_velocity;
}

void DissipativeVelocity::set_accelerations(
    double accel, double drag_accel ) {
  accel_ = accel;
  drag_accel_ = drag_accel;
  // We must have 0 <= drag_accel_ < accel_
  ASSERT( drag_accel_ >= 0 );
  ASSERT( accel_ > drag_accel_ );
}

DissipativeVelocity::DissipativeVelocity(
    double min_velocity, double max_velocity,
    double initial_velocity, double acceleration,
    double drag_acceleration )
  : min_velocity_( min_velocity ), max_velocity_( max_velocity ),
    velocity_( initial_velocity ), accel_( acceleration ),
    drag_accel_( drag_acceleration )
{
  // These functions will do some assertions.
  set_bounds( min_velocity, max_velocity );
  set_accelerations( acceleration, drag_acceleration );
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
