/****************************************************************
**orders.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles the representation and application of
*              the orders that players can give to units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"

// Rnl
#include "rnl/orders.hpp"

// Flatbuffers
#include "fb/orders_generated.h"

// C++ standard library
#include <variant>

namespace rn {

void push_unit_orders( UnitId id, orders_t const& orders );
maybe<orders_t> pop_unit_orders( UnitId id );

} // namespace rn
