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

enum class e_eot_loop_result {
  none,
  quit
};

e_eot_loop_result loop_eot();

enum class e_orders_loop_result {
  wait,
  quit,
  moved
};

e_orders_loop_result loop_orders( UnitId id );

} // namespace rn
