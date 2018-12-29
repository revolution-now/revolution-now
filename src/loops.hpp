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

// Revolution Now
#include "orders.hpp"
#include "physics.hpp"
#include "unit.hpp"

// C++ standard library
#include <functional>

namespace rn {

enum class ND e_eot_loop_result { none, quit_game };

ND e_eot_loop_result loop_eot();

struct orders_loop_result {
  // This should just give a high-level description of the
  // outcome of the orders loop.
  enum class e_type { none, quit_game, orders_received } type;
  PlayerUnitOrders orders;
};

ND orders_loop_result loop_orders();

bool loop_mv_unit( double&              percent,
                   DissipativeVelocity& percent_vel );

void frame_throttler( std::function<bool()> frame );

} // namespace rn
