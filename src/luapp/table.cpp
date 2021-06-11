/****************************************************************
**table.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua tables.
*
*****************************************************************/
#include "table.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua {

/****************************************************************
** table
*****************************************************************/
table table::global( cthread L ) noexcept {
  c_api C( L );
  C.pushglobaltable();
  return table( C.this_cthread(), C.ref_registry() );
}

table table::new_empty( cthread L ) noexcept {
  c_api C( L );
  C.newtable();
  return table( C.this_cthread(), C.ref_registry() );
}

} // namespace lua
