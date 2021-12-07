/****************************************************************
**as.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-19.
*
* Description: Cast from one Lua-aware type to another via Lua.
*
*****************************************************************/
#include "as.hpp"

// luapp
#include "c-api.hpp"
#include "types.hpp"

using namespace std;

namespace lua {

namespace detail {

void as_pop( cthread L, int n ) { c_api( L ).pop( n ); }

std::string as_type_name( cthread L, int idx ) {
  return fmt::format( "{}", c_api( L ).type_of( idx ) );
}

} // namespace detail

} // namespace lua
