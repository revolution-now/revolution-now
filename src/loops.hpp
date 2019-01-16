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

void frame_throttler( bool                  poll_input,
                      std::function<bool()> finished );

double avg_frame_rate();

} // namespace rn
