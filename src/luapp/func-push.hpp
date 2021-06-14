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
#include "userdata.hpp"

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

// Pushes as a plane function with no upvalues.
void push_stateless_lua_c_function( cthread       L,
                                    LuaCFunction* func,
                                    int           upvalues = 0 );

// Pushes the function as a Lua closure, where the upvalue is
// function object.
template<typename T>
void push_stateful_lua_c_function( cthread L, T&& func );

/****************************************************************
** Concepts
*****************************************************************/
template<typename T> // clang-format off
concept LuaCExtensionFunction =
  base::NonOverloadedCallable<std::remove_cvref_t<T>> &&
  std::is_same_v<
      mp::callable_ret_type_t<std::remove_cvref_t<T>>,
      int> &&
  std::is_same_v<
      mp::callable_arg_types_t<std::remove_cvref_t<T>>,
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
void push( cthread L, T&& o ) {
  // clang-format on
  push_stateless_lua_c_function( L, std::forward<T>( o ) );
}

// clang-format off
template<typename T>
void push( cthread L, T&& o )
  requires(
    StatefulLuaCExtensionFunction<T> &&
    std::is_rvalue_reference_v<decltype(std::forward<T>( o ))> )
{
  // clang-format on
  push_stateful_lua_c_function( L, std::forward<T>( o ) );
}

/****************************************************************
** Implementations.
*****************************************************************/
template<typename T>
void push_stateful_lua_c_function( cthread L, T&& func ) {
  using fwd_t   = decltype( std::forward<T>( func ) );
  using T_noref = std::remove_reference_t<fwd_t>;
  static_assert( StatefulLuaCExtensionFunction<T> );
  static_assert( std::is_rvalue_reference_v<fwd_t> );

  // 1. Create the closure with one upvalue (the userdata).
  static auto closure_caller = []( lua_State* L ) -> int {
    static std::string const type_name =
        userdata_typename<T_noref>();
    void* ud =
        check_udata( L, upvalue_index( 1 ), type_name.c_str() );
    auto const& closure = *static_cast<T_noref const*>( ud );
    return closure( L );
  };

  push_userdata_by_value( L, std::move( func ) );
  push_stateless_lua_c_function( L, closure_caller,
                                 /*upvalues=*/1 );
}

} // namespace lua
