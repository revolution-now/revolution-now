/****************************************************************
**frame.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-13.
*
* Description: Frames and frame rate.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <chrono>
#include <functional>

namespace rn {

using Frames = std::chrono::duration<int, std::ratio<1, 60>>;

void frame_loop( bool                  poll_input,
                 std::function<bool()> finished );

double avg_frame_rate();

uint64_t total_frame_count();

} // namespace rn
