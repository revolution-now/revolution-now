/****************************************************************
**turn.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Main loop that processes a turn.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct Planes;
struct SS;
struct TS;

wait<> turn_loop( Planes& planes, SS& ss, TS& ts );

} // namespace rn
