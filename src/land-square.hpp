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
#include "aliases.hpp"
#include "errors.hpp"
#include "fb.hpp"

// Flatbuffers
#include "fb/land-square_generated.h"

namespace rn {

enum class e_crust { water, land };
SERIALIZABLE_ENUM( e_crust );

struct LandSquare {
  expect<> check_invariants_safe() const;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, LandSquare,
  ( e_crust, crust ));
  // clang-format on
};
NOTHROW_MOVE( LandSquare );

} // namespace rn
