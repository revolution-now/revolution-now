/****************************************************************
**unit-stack.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-07.
*
* Description: Handles things related to stacks of units on the
*              same tile.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/unit-id.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct SSConst;

// For convenience. Uses the above comparator. Top unit will be
// first in the vector.
void sort_unit_stack( SSConst const&              ss,
                      std::vector<GenericUnitId>& units,
                      maybe<GenericUnitId>        force_top );

// For convenience. Uses the above comparator. Top unit will be
// first in the vector.
void sort_native_unit_stack( SSConst const&             ss,
                             std::vector<NativeUnitId>& units );

// For convenience. Uses the above comparator. Top unit will be
// first in the vector.
void sort_euro_unit_stack( SSConst const&       ss,
                           std::vector<UnitId>& units );

} // namespace rn
