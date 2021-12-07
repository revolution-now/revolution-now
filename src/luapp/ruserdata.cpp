/****************************************************************
**ruserdata.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-26.
*
* Description: RAII holder for registry references to Lua
*              userdata.
*
*****************************************************************/
#include "ruserdata.hpp"

// luapp
#include "as.hpp"
#include "c-api.hpp"

using namespace std;

namespace lua {

base::maybe<userdata> lua_get( cthread L, int idx,
                               tag<userdata> ) {
  lua::c_api C( L );
  if( C.type_of( idx ) != type::userdata ) return base::nothing;
  // Copy the requested value to the top of the stack.
  C.pushvalue( idx );
  // Then pop it into a registry reference.
  return userdata( L, C.ref_registry() );
}

std::string userdata::name() const {
  return as<string>( ( *this )[metatable_key]["__name"] );
}

} // namespace lua
