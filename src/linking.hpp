/****************************************************************
**linking.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-03.
*
* Description: Tells the linker to include all modules.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

// This function should be called from any executable; it in turn
// will call a series of no-op functions defined in various mod-
// ules in order to force the linker to include all of the object
// files in the rn static library in the executable even if they
// would otherwise not be directly used. We need to do this be-
// cause in general modules have registration functions that need
// to run for the game to load properly.
void linker_dont_discard_me();

// Pass a pointer to an object to this function, which will do
// nothing with it, but should prevent the compiler from opti-
// mizing something away. This is useful for benchmarking.
void dont_optimize_me( void* p );

} // namespace rn
