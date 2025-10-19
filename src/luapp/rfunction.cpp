/****************************************************************
**rfunction.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-17.
*
* Description: RAII holder for registry references to Lua
*              functions.
*
*****************************************************************/
#include "rfunction.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua {

lua_expect<rfunction> lua_get( cthread L, int idx,
                               tag<rfunction> ) {
  lua::c_api C( L );
  if( C.type_of( idx ) != type::function ) return unexpected{};
  // Copy the requested value to the top of the stack.
  C.pushvalue( idx );
  // Then pop it into a registry reference.
  return rfunction( L, C.ref_registry() );
}

void to_str( rfunction const& o, std::string& out,
             base::tag<rfunction> ) {
  to_str( o, out, base::tag<any>{} );
}

} // namespace lua
