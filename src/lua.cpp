/****************************************************************
**lua.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-13.
*
* Description: Interface to lua.
*
*****************************************************************/
#include "lua.hpp"

// Revolution Now
#include "logging.hpp"

// Won't be needed in future versions.
#define SOL_CXX17_FEATURES 1

#ifdef L
#  undef L
#  include "sol/sol.hpp"
#  define L( a ) []( auto const& _ ) { return a; }
#else
#  include "sol/sol.hpp"
#endif

namespace rn {

namespace {} // namespace

void test_lua() {
  sol::state lua;
  int        x = 0;
  lua.set_function( "beep", [&x] { ++x; } );
  lua.script( "beep()" );
  lg.info( "x: {}", x );
}

} // namespace rn
