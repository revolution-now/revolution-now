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

// C++ standard library
#include <chrono>
#include <functional>

namespace rn {

struct IEngine;
struct Planes;

// Will spin until the wait is ready.
void frame_loop( IEngine& engine, Planes& planes,
                 wait<> const& what );

double avg_frame_rate();

uint64_t total_frame_count();

using FrameSubscriptionFunc = std::function<void( void )>;

// TODO: find a better way to handle these subscriptions and the
// testing of them. Basically all code in the game that uses
// timing of any kind will use this.
//
// Subscribe to receive a notification after n ticks, or every n
// ticks if repeating == true.
[[nodiscard]] int64_t subscribe_to_frame_tick(
    FrameSubscriptionFunc f, FrameCount n,
    bool repeating = true );
// Subscribe to receive a notification after n microseconds, or
// every n microseconds if repeating == true.
[[nodiscard]] int64_t subscribe_to_frame_tick(
    FrameSubscriptionFunc, std::chrono::microseconds n,
    bool repeating = true );

// If a subscription is still active then this will unsubscribe
// it. Note that the IDs are unique across both types of sub-
// scriptions.
void unsubscribe_frame_tick( int64_t id );

// TODO: find a better way to handle these subscriptions and the
// testing of them. Basically all code in the game that uses
// timing of any kind will use this.
void testing_notify_all_subscribers();

using EventCountMap =
    std::unordered_map<std::string_view,
                       MovingAverageTyped</*seconds=*/3>>;

EventCountMap& event_counts();

} // namespace rn
