/****************************************************************
**unit-id.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-12.
*
* Description: Id for units.
*
*****************************************************************/
#include "unit-id.hpp"

// luapp
#include "luapp/types.hpp"

// Cdr
#include "cdr/ext-builtin.hpp"

namespace rn {

/****************************************************************
** lua
*****************************************************************/
LUA_TYPED_INT_IMPL( ::rn::UnitId );

/****************************************************************
** cdr
*****************************************************************/
CDR_TYPED_INT_IMPL( ::rn::UnitId );

} // namespace rn
