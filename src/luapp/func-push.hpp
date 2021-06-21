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
#include "error.hpp"
#include "types.hpp"
#include "userdata.hpp"

// base
#include "base/func-concepts.hpp"
#include "base/macros.hpp"
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

// Pushes the function as a Lua closure, where the upvalue is a
// lambda that captures the callable (which itself may be an ob-
// ject, lambda, or simply a function pointers). But there will
// always be an upvalue.
template<typename Func>
auto push_cpp_function( cthread L, Func&& func ) noexcept;

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

// Just any old C++ function.
template<typename T>
concept RegularNonOverloadedCppFunction =
    !LuappInternal<T> && base::NonOverloadedCallable<T> &&
    !LuaCExtensionFunction<T>;

/****************************************************************
** push overloads.
*****************************************************************/
void lua_push( cthread                               L,
               StatelessLuaCExtensionFunction auto&& o ) {
  push_stateless_lua_c_function( L, FWD( o ) );
}

void lua_push( cthread                              L,
               StatefulLuaCExtensionFunction auto&& o ) {
  push_stateful_lua_c_function( L, FWD( o ) );
}

void lua_push( cthread                                L,
               RegularNonOverloadedCppFunction auto&& o ) {
  push_cpp_function( L, FWD( o ) );
}

/****************************************************************
** Implementation: push_stateful_lua_c_function
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

/****************************************************************
** Implementation: push_cpp_function
*****************************************************************/
namespace detail {

// Will throw a Lua error if the number of Lua arguments on the
// stack is not equal to the number of cpp arguments.
void func_push_cpp_check_args( cthread L, int num_cpp_args );

template<typename T>
concept PushableOrVoid = Pushable<T> || std::same_as<void, T>;

template<typename Func, PushableOrVoid R,
         StorageGettable... Args, size_t... Idx>
void push_cpp_function_impl(
    cthread L, Func&& func, R*, mp::type_list<Args...>*,
    std::index_sequence<Idx...> ) noexcept {
  auto runner = [func = std::move( func )]( lua_State* L ) {
    func_push_cpp_check_args( L, sizeof...( Args ) );

    using CalledArgs  = std::tuple<Args...>;
    using StorageArgs = std::tuple<storage_type_for<Args>...>;

    auto get_arg = [&]<size_t Index>(
                       std::integral_constant<size_t, Index> )
        -> std::tuple_element_t<Index, StorageArgs> {
      using elem_t =
          typename std::tuple_element_t<Index, StorageArgs>;
      int  lua_idx = Index + 1;
      auto m       = lua::get<elem_t>( L, lua_idx );
      if( !m.has_value() )
        throw_lua_error(
            L,
            "Native function expected type '{}' for "
            "argument {} (1-based), but received "
            "non-convertible type '{}' from Lua.",
            base::demangled_typename<elem_t>(), Index + 1,
            type_name( L, lua_idx ) );
      return *m;
    };

    StorageArgs args{
        get_arg( std::integral_constant<size_t, Idx>{} )... };

    if constexpr( std::is_same_v<R, void> ) {
      std::apply( func, std::move( args ) );
      return 0;
    } else {
      push( L, std::apply( func, std::move( args ) ) );
      return 1;
    }
  };
  push_stateful_lua_c_function( L, std::move( runner ) );
}

} // namespace detail

template<typename Func>
auto push_cpp_function( cthread L, Func&& func ) noexcept {
  using ret_t  = mp::callable_ret_type_t<Func>;
  using args_t = mp::callable_arg_types_t<Func>;
  detail::push_cpp_function_impl(
      L, std::forward<Func>( func ), (ret_t*)nullptr,
      (args_t*)nullptr,
      std::make_index_sequence<mp::type_list_size_v<args_t>>() );
}

} // namespace lua
