/****************************************************************
**any.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-17.
*
* Description: Registry reference to any Lua type.
*
*****************************************************************/
#include "any.hpp"

// luapp
#include "c-api.hpp"

// base
#include "error.hpp"

using namespace std;

namespace lua {

base::maybe<any> lua_get( cthread L, int idx, tag<any> ) {
  lua::c_api C( L );
  // Copy the requested value to the top of the stack.
  C.pushvalue( idx );
  // Then pop it into a registry reference.
  return any( L, C.ref_registry() );
}

} // namespace lua
