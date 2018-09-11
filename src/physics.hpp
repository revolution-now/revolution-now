/****************************************************************
* physics.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-09.
*
* Description: Handles anything in the game that relies on physics
*              or physics-like actions.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

enum class ND e_push_direction {
  negative,
  none,
  positive
};

// This is a class that represents a velocity under a force and a
// dissipative acceleration. It can be implicitly converted to a
// double, and that double is the velocity. When the object is
// "advanced" the acceleration is applied to the velocity de-
// pending on the direction of the acceleration. If the accelera-
// tion is positive or negative then the net acceleration will be
// the sum of the forced acceleration plus the drag acceleration.
// When there is no forced acceleration only the drag accelera-
// tion will apply.
class ND DissipativeVelocity {

public:
  DissipativeVelocity( double min_velocity,
                       double max_velocity,
                       double initial_velocity,
                       double acceleration,
                       double drag_acceleration );

  operator double() const { return velocity_; }

  double advance( e_push_direction direction );

private:
  double const min_velocity_;
  double const max_velocity_;
  double velocity_;
  double const accel_;
  double const drag_accel_;
};

} // namespace rn

