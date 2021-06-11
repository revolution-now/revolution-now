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

void indexer_gettable( lua_State* L ) {
  c_api C = c_api::view( L );
  C.gettable( -2 );
  C.swap_top();
  C.pop();
}

void indexer_settable( lua_State* L ) {
  c_api C = c_api::view( L );
  C.settable( -3 );
  C.pop();
}

} // namespace lua
