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

// Revolution Now
#include "core-config.hpp"
#include "math.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <chrono>
#include <functional>

namespace rn {

using Frames = std::chrono::duration<int, std::ratio<1, 60>>;

void frame_loop( bool                  poll_input,
                 std::function<bool()> finished );

double avg_frame_rate();

uint64_t total_frame_count();

using EventCountMap =
    absl::flat_hash_map<std::string_view,
                        MovingAverage<3 /*seconds*/>>;

EventCountMap& event_counts();

} // namespace rn
