/****************************************************************
**physics.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-09.
*
* Description: Handles anything in the game that relies on
*physics or physics-like actions.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"

// Flatbuffers
#include "fb/physics_generated.h"

namespace rn {

enum class ND e_push_direction {
  negative = -1,
  none     = 0,
  positive = 1
};

// This is a class that represents a velocity under a force and a
// dissipative acceleration. It can be implicitly converted to a
// double, and that double is the velocity. When the object is
// "advanced" the acceleration is applied to the velocity de-
// pending on the direction of the acceleration. The acceleration
// is computed by reducing the magnitude of the specified force
// acceleration by the drag acceleration. When there is no forced
// acceleration (== 0.0) only the drag acceleration will apply
// (in the direction opposite the velocity).
class ND DissipativeVelocity {
 public:
  DissipativeVelocity() = default;
  DissipativeVelocity( double min_velocity, double max_velocity,
                       double initial_velocity,
                       // accerlerations are magnitudes
                       double mag_acceleration,
                       double mag_drag_acceleration );

  double to_double() const { return velocity_; }

  // Advances velocity by applying accelerations and then returns
  // the advanced value.
  double advance( e_push_direction direction );

  // The particle will hit a wall, e.g., velocity immediately
  // goes to zero.
  void hit_wall();

  void set_bounds( double min_velocity, double max_velocity );
  void set_accelerations( double accel, double drag_accel );

  bool operator==( DissipativeVelocity const& rhs ) const {
    return ( min_velocity_ == rhs.min_velocity_ ) && //
           ( max_velocity_ == rhs.max_velocity_ ) && //
           ( velocity_ == rhs.velocity_ ) &&         //
           ( accel_ == rhs.accel_ ) &&               //
           ( drag_accel_ == rhs.drag_accel_ );
  }

  valid_deserial_t check_invariants_safe() const;

 private:
  // clang-format off
  SERIALIZABLE_STRUCT_MEMBERS( DissipativeVelocity,
  ( double, min_velocity_ ),
  ( double, max_velocity_ ),
  ( double, velocity_     ),
  ( double, accel_        ),
  ( double, drag_accel_   ));
  // clang-format on
};
NOTHROW_MOVE( DissipativeVelocity );

} // namespace rn

DEFINE_FORMAT( ::rn::DissipativeVelocity,
               "DissipativeVelocity{{velocity={}}}",
               o.to_double() );
