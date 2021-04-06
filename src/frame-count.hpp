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

// You can use this like so:
//
//   auto f = 5_frames;
//   FrameCount f{5};
//
TYPED_INT( FrameCount, frames );
