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

// base
#include "base/fmt.hpp"

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

  // clang-format off
  template<typename T>
  requires( Pushable<T> && HasCthread<T> )
  explicit any( T&& o ) : reference((
                   lua::push( o.this_cthread(), o ),
                   pop_ref_from_stack(o.this_cthread()))) {
    // clang-format on
  }

  // clang-format off
private:
  static reference pop_ref_from_stack(cthread L);
  // clang-format on
};

static_assert( Stackable<any> );

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::any );
