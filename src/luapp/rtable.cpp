/****************************************************************
**rtable.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua tables.
*
*****************************************************************/
#include "rtable.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua {

base::maybe<table> lua_get( cthread L, int idx, tag<table> ) {
  lua::c_api C( L );
  if( C.type_of( idx ) != type::table ) return base::nothing;
  // Copy the requested value to the top of the stack.
  C.pushvalue( idx );
  // Then pop it into a registry reference.
  return table( L, C.ref_registry() );
}

} // namespace lua
