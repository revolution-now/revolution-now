/****************************************************************
**colony-id.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Id for colonies.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "typed-int.hpp"

// luapp
#include "luapp/ext.hpp"

TYPED_ID( ColonyId )

/****************************************************************
** std::hash
*****************************************************************/
namespace std {
DEFINE_HASH_FOR_TYPED_INT( ::rn::ColonyId )
} // namespace std

/****************************************************************
** lua
*****************************************************************/
namespace rn {
LUA_TYPED_INT_DECL( ::rn::ColonyId );
} // namespace rn

/****************************************************************
** cdr
*****************************************************************/
namespace rn {
CDR_TYPED_INT_DECL( ::rn::ColonyId );
}
