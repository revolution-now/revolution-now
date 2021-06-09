/****************************************************************
**state.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: High-level Lua state object.
*
*****************************************************************/
#pragma once

// luapp
#include "c-api.hpp"
#include "types.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"
#include "base/meta.hpp"
#include "base/unique-func.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <memory>
#include <string_view>
#include <tuple>

struct lua_State;

namespace luapp {

struct c_api;

struct state {
  state();
  // TODO: this constructor can be removed (as well as the lua.h
  // include) after migration away from sol2.
  state( lua_State* state );

  using c_string_list = std::vector<char const*>;

  void openlibs() noexcept;

  template<typename Func>
  auto push_function( Func&& func ) noexcept {
    using args_t = mp::callable_arg_types_t<Func>;
    if constexpr( std::is_same_v<args_t,
                                 mp::type_list<lua_State*>> ) {
      static_assert(
          std::is_same_v<std::invoke_result_t<Func, lua_State*>,
                         int> );
      if constexpr( std::is_convertible_v<Func, LuaCFunction*> )
        push_stateless_lua_c_function( +func );
      else
        return push_stateful_lua_c_function(
            std::forward<Func>( func ) );
    } else {
      return push_cpp_function( std::forward<Func>( func ) );
    }
  }

  void tables( c_string_list const& path ) noexcept;

  // TODO: use this as a model for loading a piece of code once
  // into the registry. Can remove eventually.
  void tables_slow( std::string_view path ) noexcept;
  int  tables_func_ref = noref();

  // Will traverse the list of components in `path` and assume
  // that they are all tables (except for possibly the last one),
  // and will leave the final object on the stack, and will re-
  // turn its type.
  e_lua_type push_path( c_string_list const& path ) noexcept;

  c_api& api() noexcept;

private:
  state( state const& ) = delete;
  state( state&& )      = delete;
  state& operator=( state const& ) = delete;
  state& operator=( state&& ) = delete;

  template<typename T>
  bool create_userdata( T&& object ) noexcept;

  // Creates the closure and sets it on the path. The return
  // value, which indicates whether it created a new metatable,
  // is mainly used for testing.
  bool push_stateful_lua_c_function(
      base::unique_func<int( lua_State* ) const>
          closure ) noexcept;

  void push_stateless_lua_c_function(
      LuaCFunction* func ) noexcept;

  template<typename Func, typename R, typename... Args>
  bool push_cpp_function_impl(
      Func&& func, R*, mp::type_list<Args...>* ) noexcept;

  template<typename Func>
  bool push_cpp_function( Func&& func ) noexcept;

  // Given t1.t2.t3.key, it will assume that t1,t2,t3 are tables
  // and will traverse them, leaving t3 pushed onto the stack
  // followed by 'key'. This function is therefore meant to be
  // followed by a push of a value, and then a call to settable.
  // If there is only one element in the path ('key') then the
  // global table will be pushed, followed by 'key'.
  void traverse_and_push_table_and_key(
      c_string_list const& path ) noexcept;

  static int noref();

  std::unique_ptr<c_api> api_;
  c_api&                 C;
};

template<typename Func, typename R, typename... Args>
bool state::push_cpp_function_impl(
    Func&& func, R*, mp::type_list<Args...>* ) noexcept {
  static auto runner =
      [func = std::move( func )]( lua_State* L ) -> int {
    c_api C( L, /*own=*/false );
    using ArgsTuple = std::tuple<std::remove_cvref_t<Args>...>;
    ArgsTuple args;

    int num_args = C.gettop();
    if( num_args != sizeof...( Args ) ) {
      C.push(
          fmt::format( "C++ function expected {} arguments, but "
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
  return push_stateful_lua_c_function( std::move( runner ) );
}

template<typename Func>
bool state::push_cpp_function( Func&& func ) noexcept {
  using ret_t  = mp::callable_ret_type_t<Func>;
  using args_t = mp::callable_arg_types_t<Func>;
  return push_cpp_function_impl( std::forward<Func>( func ),
                                 (ret_t*)nullptr,
                                 (args_t*)nullptr );
}

} // namespace luapp
