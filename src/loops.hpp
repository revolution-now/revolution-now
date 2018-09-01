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

enum k_loop_result {
  none,
  quit
};

k_loop_result loop_eot();

k_loop_result loop_orders( UnitId id );

} // namespace rn
