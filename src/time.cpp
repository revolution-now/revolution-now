/****************************************************************
**time.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-19.
*
* Description: All things in the fourth dimension.
*
*****************************************************************/
#include "time.hpp"

// Revolution Now
#include "lua.hpp"

// luapp
#include "luapp/state.hpp"

// C++ standard library
#include <chrono>

using namespace std;

namespace rn {

void linker_dont_discard_module_time() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// Lua apparently does not have a way to get the current wall
// time in sub-seconds.
LUA_FN( current_epoch_time_micros, long ) {
  return chrono::duration_cast<chrono::microseconds>(
             Clock_t::now().time_since_epoch() )
      .count();
}

} // namespace

} // namespace rn
