/****************************************************************
**recursive-binder.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-30.
*
* Description: Recursively traverses a reflected data structure
*              and exposes it to Lua.
*
*****************************************************************/
#pragma once

// luapp
#include "ext-usertype.hpp"
#include "ext.hpp"
#include "register.hpp"
#include "usertype.hpp"

// traverse
#include "traverse/type-ext.hpp"

// base
#include "base/odr.hpp"

/****************************************************************
** Macros.
*****************************************************************/
#define RUN_RECURSIVE_LUA_BINDER( type ) \
  TRV_RUN_TYPE_TRAVERSE( ::lua::RecursiveLuaBinder, type )

namespace lua {

/****************************************************************
** RecursiveLuaBinder
*****************************************************************/
// This will recursively traverse a type and register a method
// for each sub type encountered that will define its Lua bind-
// ings. The end result is that, whenever a new Lua environment
// is created and all the registration functions are run, each
// type touched by this registrar will have its
// define_usertype_for method called which will define its Lua
// bindings within that state. So the registration methods only
// run once per process, but the usertype definition functions
// will run once for each Lua state.
//
// It can be run on a top-level type X like so:
//
//   RUN_RECURSIVE_LUA_BINDER( X );
//
// This should be done in a cpp file; does not need to be visible
// in a header. Just make sure the cpp file gets linked in.
template<typename T>
struct RecursiveLuaBinder {
  using type = ::trv::TypeTraverse<RecursiveLuaBinder, T>::type;

  // Ensure that we're never Pushable in more than one way.
  static_assert( HasUniqueOwnershipCategory<T> );

  inline static int _ = [] {
    if constexpr( DefinesUsertype<T> ) {
      static_assert( CanHaveUsertype<T> );
      static auto constexpr p_register_fn = +[]( state& st ) {
        define_usertype_for( st, tag<T>{} );
      };
      detail::register_lua_fn( &p_register_fn );
    }
    return 0;
  }();
  ODR_USE_MEMBER( _ );
};

} // namespace lua
