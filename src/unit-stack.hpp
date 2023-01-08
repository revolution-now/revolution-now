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

// These are the functions that contains the logic that deter-
// mines the order in which units are visually stacked when there
// are multiple on a square, and also how a defending unit is
// chosen when attacking a square with multiple units. This is
// factored out into a common location because it needs to be
// consistent across various places in the code base in order to
// ensure a consistent UI/UX.

// Top unit will be first in the vector.
void sort_unit_stack( SSConst const&              ss,
                      std::vector<GenericUnitId>& units );

// Top unit will be first in the vector.
void sort_native_unit_stack( SSConst const&             ss,
                             std::vector<NativeUnitId>& units );

// Top unit will be first in the vector.
void sort_euro_unit_stack( SSConst const&       ss,
                           std::vector<UnitId>& units );

} // namespace rn
