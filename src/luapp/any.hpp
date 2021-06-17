/****************************************************************
**any.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-17.
*
* Description: Registry reference to any Lua type.
*
*****************************************************************/
#pragma once

// luapp
#include "ext.hpp"
#include "ref.hpp"

namespace lua {

/****************************************************************
** any
*****************************************************************/
// This is essentially just a `reference', though one of the no-
// table differences is that an `any' is both Pushable and Get-
// table, while a `reference' is only Pushable.
struct any : reference {
  using Base = reference;
  using Base::Base;

  friend base::maybe<any> lua_get( cthread L, int idx,
                                   tag<any> );
};

static_assert( Stackable<any> );

} // namespace lua
