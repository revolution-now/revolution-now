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
    std::is_rvalue_reference_v<decltype(std::forward<T>( o ))> ) {
  // clang-format on
  push_stateful_lua_c_function( L, std::forward<T>( o ) );
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

[[noreturn]] void func_push_throw_lua_error(
    cthread L, std::string_view msg );

char const* func_push_lua_type_at_idx( cthread L, int idx );

template<typename Func, typename R, typename... Args>
void push_cpp_function_impl( cthread L, Func&& func, R*,
                             mp::type_list<Args...>* ) noexcept {
  auto runner = [func = std::move( func )]( lua_State* L ) {
    using ArgsTuple = std::tuple<std::remove_cvref_t<Args>...>;
    ArgsTuple args;

    func_push_cpp_check_args( L, sizeof...( Args ) );

    auto to_cpp_arg = [&]<size_t Idx>(
                          std::integral_constant<size_t, Idx> ) {
      using elem_t = std::tuple_element_t<Idx, ArgsTuple>;
      int  lua_idx = Idx + 1;
      auto m       = lua::get<elem_t>( L, lua_idx );
      if constexpr( !std::is_same_v<bool, decltype( m )> ) {
        if( !m.has_value() )
          func_push_throw_lua_error(
              L,
              fmt::format(
                  "Native function expected type '{}' for "
                  "argument "
                  "{} (1-based), but received non-convertible "
                  "type '{}' from Lua.",
                  base::demangled_typename<elem_t>(), Idx + 1,
                  func_push_lua_type_at_idx( L, lua_idx ) ) );
        std::get<Idx>( args ) = *m;
      } else {
        // for bools
        std::get<Idx>( args ) = m;
      }
    };
    mp::for_index_seq<sizeof...( Args )>( to_cpp_arg );

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
  detail::push_cpp_function_impl( L, std::forward<Func>( func ),
                                  (ret_t*)nullptr,
                                  (args_t*)nullptr );
}

} // namespace lua
