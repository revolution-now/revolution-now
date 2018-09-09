/****************************************************************
* loops.hpp
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

namespace rn {

enum class ND e_eot_loop_result {
  none,
  quit
};

ND e_eot_loop_result loop_eot();

enum class ND e_orders_loop_result {
  wait,
  quit,
  moved
};

ND e_orders_loop_result loop_orders( UnitId id );

} // namespace rn
