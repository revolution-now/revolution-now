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

// Revolution Now
#include "frame-count.hpp"
#include "moving-avg.hpp"
#include "wait.hpp"

// render
#include "render/renderer.hpp"

// C++ standard library
#include <chrono>
#include <functional>

namespace rn {

struct Planes;

// Will spin until the wait is ready.
void frame_loop( Planes& planes, wait<> const& what,
                 rr::Renderer& renderer );

double avg_frame_rate();

uint64_t total_frame_count();

using FrameSubscriptionFunc = std::function<void( void )>;

// Subscribe to receive a notification after n ticks, or every n
// ticks if repeating == true.
void subscribe_to_frame_tick( FrameSubscriptionFunc f,
                              FrameCount            n,
                              bool repeating = true );
// Subscribe to receive a notification after n microseconds, or
// every n microseconds if repeating == true.
void subscribe_to_frame_tick( FrameSubscriptionFunc,
                              std::chrono::microseconds n,
                              bool repeating = true );

using EventCountMap =
    std::unordered_map<std::string_view,
                       MovingAverageTyped</*seconds=*/3>>;

EventCountMap& event_counts();

} // namespace rn
