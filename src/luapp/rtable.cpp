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

table table::pop_or_create_table( cthread L ) {
  lua::c_api C( L );
  CHECK( C.stack_size() >= 1 );
  switch( C.type_of( -1 ) ) {
    case type::nil: {
      C.newtable();
      table res = table( L, C.ref_registry() );
      C.pop();
      return res;
    }
    case type::table: {
      return table( L, C.ref_registry() );
    }
    default:
      throw_lua_error( L,
                       "expected either a table or nil but "
                       "found an object of type {}.",
                       C.type_of( -1 ) );
  }
}

} // namespace lua
