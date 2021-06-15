/****************************************************************
**helper.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: High-level Lua helper object.
*
*****************************************************************/
#pragma once

// luapp
#include "c-api.hpp"
#include "func-push.hpp"
#include "types.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"
#include "base/meta.hpp"
#include "base/unique-func.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <string_view>
#include <tuple>

namespace lua {

struct helper {
  helper( cthread helper );

  using c_string_list = std::vector<char const*>;

  template<typename Func>
  auto push_cpp_function( Func&& func ) noexcept;

  // Expects a function on the top of the stack, and will call it
  // with the given C++ arguments. Returns the number of argu-
  // ments returned by the Lua function.
  template<typename... Args>
  int call( Args&&... args );

  // Expects a function on the top of the stack, and will pcall
  // it with the given C++ arguments. If successful, returns the
  // number of arguments returned by the Lua function.
  template<typename... Args>
  lua_expect<int> pcall( Args&&... args ) noexcept;

private:
  helper( helper const& ) = delete;
  helper( helper&& )      = delete;
  helper& operator=( helper const& ) = delete;
  helper& operator=( helper&& ) = delete;

  template<typename Func, typename R, typename... Args>
  void push_cpp_function_impl(
      Func&& func, R*, mp::type_list<Args...>* ) noexcept;

  static int noref();

  c_api C;
};

template<typename Func>
auto helper::push_cpp_function( Func&& func ) noexcept {
  using ret_t  = mp::callable_ret_type_t<Func>;
  using args_t = mp::callable_arg_types_t<Func>;
  push_cpp_function_impl( std::forward<Func>( func ),
                          (ret_t*)nullptr, (args_t*)nullptr );
}

template<typename Func, typename R, typename... Args>
void helper::push_cpp_function_impl(
    Func&& func, R*, mp::type_list<Args...>* ) noexcept {
  static auto const runner =
      [func = std::move( func )]( lua_State* L ) -> int {
    c_api C( L );
    using ArgsTuple = std::tuple<std::remove_cvref_t<Args>...>;
    ArgsTuple args;

    int num_args = C.gettop();
    if( num_args != sizeof...( Args ) ) {
      C.push( fmt::format(
          "Native function expected {} arguments, but "
          "received {} from Lua.",
          sizeof...( Args ), num_args ) );
      C.error();
    }

    auto to_cpp_arg =
        [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
          using elem_t = std::tuple_element_t<Idx, ArgsTuple>;
          int  lua_idx = Idx + 1;
          auto m       = C.get<elem_t>( lua_idx );
          if constexpr( !std::is_same_v<bool, decltype( m )> ) {
            if( !m.has_value() ) {
              C.push( fmt::format(
                  "C++ function expected type '{}' for argument "
                  "{} (1-based), but received non-convertible "
                  "type '{}' from Lua.",
                  base::demangled_typename<elem_t>(), Idx + 1,
                  C.type_of( lua_idx ) ) );
              C.error();
            }
            get<Idx>( args ) = *m;
          } else {
            // for bools
            get<Idx>( args ) = m;
          }
        };
    mp::for_index_seq<sizeof...( Args )>( to_cpp_arg );

    if constexpr( std::is_same_v<R, void> ) {
      std::apply( func, args );
      return 0;
    } else {
      C.push( std::apply( func, args ) );
      return 1;
    }
  };
  push_stateful_lua_c_function( C.this_cthread(),
                                std::move( runner ) );
}

template<typename... Args>
int helper::call( Args&&... args ) {
  CHECK( C.stack_size() >= 1 );
  CHECK( C.type_of( -1 ) == type::function );
  // Get size of stack before function was pushed.
  int starting_stack_size = C.stack_size() - 1;

  ( C.push( std::forward<Args>( args ) ), ... );
  C.call( /*nargs=*/sizeof...( Args ),
          /*nresults=*/c_api::multret() );

  int nresults = C.stack_size() - starting_stack_size;
  CHECK_GE( nresults, 0 );
  return nresults;
}

template<typename... Args>
lua_expect<int> helper::pcall( Args&&... args ) noexcept {
  CHECK( C.stack_size() >= 1 );
  CHECK( C.type_of( -1 ) == type::function );
  // Get size of stack before function was pushed.
  int starting_stack_size = C.stack_size() - 1;

  ( C.push( std::forward<Args>( args ) ), ... );
  HAS_VALUE_OR_RET( C.pcall( /*nargs=*/sizeof...( Args ),
                             /*nresults=*/c_api::multret() ) );

  int nresults = C.stack_size() - starting_stack_size;
  CHECK_GE( nresults, 0 );
  return nresults;
}

} // namespace lua
