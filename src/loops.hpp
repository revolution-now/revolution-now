/****************************************************************
**loops.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description:
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "unit.hpp"

#include <functional>

namespace rn {

enum class ND e_eot_loop_result { none, quit };

ND e_eot_loop_result loop_eot();

enum class ND e_orders_loop_result {
  wait,
  offboard,
  quit,
  moved
};

// `prioritize` is a function that should be called if it is de-
// cided (in the loop_orders function) that a unit needs to be
// bumped to the front of the queue for accepting orders.
ND e_orders_loop_result loop_orders(
    UnitId id, std::function<void( UnitId )> const& prioritize );

void loop_mv_unit( UnitId id, Coord const& target );

} // namespace rn
