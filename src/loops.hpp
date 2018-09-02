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

#include "unit.hpp"

namespace rn {

enum class k_eot_loop_result {
  none,
  quit
};

k_eot_loop_result loop_eot();

enum class k_orders_loop_result {
  wait,
  quit,
  moved
};

k_orders_loop_result loop_orders( UnitId id );

} // namespace rn
