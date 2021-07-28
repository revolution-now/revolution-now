/****************************************************************
**land-square.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-02-01.
*
* Description: Represents a single square of land.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "fb.hpp"

// Rds
#include "rds/land-square.hpp"

// Flatbuffers
#include "fb/land-square_generated.h"

namespace rn {

struct LandSquare {
  valid_deserial_t check_invariants_safe() const;

  bool operator==( LandSquare const& ) const = default;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, LandSquare,
  ( e_surface, surface ));
  // clang-format on
};
NOTHROW_MOVE( LandSquare );

} // namespace rn
