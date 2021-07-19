/****************************************************************
**co-runner.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-19.
*
* Description: Helper to run all scheduled coroutines correctly.
*
*****************************************************************/
#include "co-runner.hpp"

// Revolution Now
#include "co-lua-scheduler.hpp"
#include "co-scheduler.hpp"

namespace rn {

void run_all_coroutines() {
  // We need this while loop because the act of running e.g. a
  // C++ coroutine could be that it results in more C++ and/or
  // Lua coroutines getting queued, so we need an iterative ap-
  // proach until there are none left of either type.
  while( number_of_queued_cpp_coroutines() > 0 ||
         number_of_queued_lua_coroutines() > 0 ) {
    run_all_cpp_coroutines();
    run_all_lua_coroutines();
  }
}

} // namespace rn
