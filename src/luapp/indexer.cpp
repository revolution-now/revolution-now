/****************************************************************
**indexer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-10.
*
* Description: Proxy for composing chains of index operators.
*
*****************************************************************/
#include "indexer.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua {

// Pushes (-2)[-1] onto the stack, and pops both table and key.
void indexer_gettable( lua_State* L ) {
  // Stack: key, table
  c_api C = c_api::view( L );
  C.gettable( -2 );
  // Stack: value, table
  C.swap_top();
  // Stack: table, value
  C.pop();
  // Stack: value
}

} // namespace lua
