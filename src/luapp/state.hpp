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
#include "types.hpp"

// base
#include "base/unique-func.hpp"

// C++ standard library
#include <memory>
#include <string_view>

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
    if constexpr( std::is_convertible_v<Func, LuaCFunction*> )
      push_stateless_function( +func );
    else
      return push_closure( std::forward<Func>( func ) );
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
  bool push_closure( base::unique_func<int( lua_State* ) const>
                         closure ) noexcept;

  void push_stateless_function( LuaCFunction* func ) noexcept;

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

} // namespace luapp
