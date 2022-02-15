/****************************************************************
**colony-id.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Id for colonies.
*
*****************************************************************/
#include "colony-id.hpp"

// luapp
#include "luapp/types.hpp"

// Cdr
#include "cdr/ext-builtin.hpp"

namespace rn {

/****************************************************************
** Lua Bindings
*****************************************************************/
LUA_TYPED_INT_IMPL( ::rn::ColonyId );

/****************************************************************
** cdr
*****************************************************************/
CDR_TYPED_INT_IMPL( ::rn::ColonyId );

} // namespace rn
