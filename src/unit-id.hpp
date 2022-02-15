/****************************************************************
**unit-id.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-12.
*
* Description: Id for units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "typed-int.hpp"

TYPED_ID( UnitId )
UD_LITERAL( UnitId, id ) // FIXME: get rid of this.

/****************************************************************
** std::hash
*****************************************************************/
namespace std {
DEFINE_HASH_FOR_TYPED_INT( ::rn::UnitId )
} // namespace std

/****************************************************************
** lua
*****************************************************************/
namespace rn {
LUA_TYPED_INT_DECL( ::rn::UnitId );
} // namespace rn

/****************************************************************
** cdr
*****************************************************************/
namespace rn {
CDR_TYPED_INT_DECL( ::rn::UnitId );
}
