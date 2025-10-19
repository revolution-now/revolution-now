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

lua_expect<userdata> lua_get( cthread L, int idx,
                              tag<userdata> ) {
  lua::c_api C( L );
  if( C.type_of( idx ) != type::userdata ) return unexpected{};
  // Copy the requested value to the top of the stack.
  C.pushvalue( idx );
  // Then pop it into a registry reference.
  return userdata( L, C.ref_registry() );
}

std::string userdata::name() const {
  return ( *this )[metatable_key]["__name"].as<string>();
}

void to_str( userdata const& o, std::string& out,
             base::tag<userdata> ) {
  to_str( o, out, base::tag<any>{} );
}

} // namespace lua
