/****************************************************************
**frame-count.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-04.
*
* Description: Defines a strong int representing a frame count.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "typed-int.hpp"
#include "wait.hpp"

// Strongly-typed integer for representing frame counts.
TYPED_INT( FrameCount, frames );

namespace rn {

wait<> co_await_transform( FrameCount count );

} // namespace rn
