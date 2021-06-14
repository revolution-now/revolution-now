/****************************************************************
**func-push.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: lua::push overload for C/C++ functions.
*
*****************************************************************/
#pragma once

// luapp
#include "types.hpp"

// base
#include "base/func-concepts.hpp"
#include "base/meta.hpp"

// C++ standard library
#include <concepts>
#include <type_traits>

namespace lua {

/****************************************************************
** push overloads.
*****************************************************************/
// These should not be called directly, just use the generic
// `push` overload.
void push_stateless_lua_c_function( cthread       L,
                                    LuaCFunction* func );

/****************************************************************
** Concepts
*****************************************************************/
template<typename T> // clang-format off
concept LuaCExtensionFunction =
    base::NonOverloadedCallable<T> &&
    std::is_same_v<mp::callable_ret_type_t<T>, int> &&
    std::is_same_v<mp::callable_arg_types_t<T>,
                   mp::type_list<lua_State*>>;
// clang-format on

// Example:
//
//   int foo( lua_State* );
//
template<typename T>
concept StatelessLuaCExtensionFunction =
    LuaCExtensionFunction<T> &&
    base::NonOverloadedStatelessCallable<T>;

// Example (note capture):
//
//   auto foo = [&]( lua_State* ) -> int { ... };
//
template<typename T>
concept StatefulLuaCExtensionFunction =
    LuaCExtensionFunction<T> &&
    base::NonOverloadedStatefulCallable<T>;

/****************************************************************
** push overloads.
*****************************************************************/
// clang-format off
template<typename T>
requires( StatelessLuaCExtensionFunction<T> )
void push( cthread L, T const& o ) {
  push_stateless_lua_c_function(L, o );
}

/****************************************************************
** Implementations.
*****************************************************************/

} // namespace lua
