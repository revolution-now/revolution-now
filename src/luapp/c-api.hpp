/****************************************************************
**c-api.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-27.
*
* Description: Simple wrapper for Lua C API calls.
*
*****************************************************************/
#pragma once

// luapp
#include "types.hpp"

// base
#include "base/maybe.hpp"
#include "base/safe-num.hpp"

// {fmt}
#include "fmt/format.h"

// Lua
#include "lua.h"

namespace luapp {

/****************************************************************
** c_api
*****************************************************************/
// This is a wrapper around the raw lua C API calls that basi-
// cally do some error checking on parameters and stack sizes, as
// well as reporting errors with base::valid_or. This is intended
// to be an intermediate step or building block to the ultimate
// lua C++ interface.
struct c_api {
  c_api();
  ~c_api() noexcept;

  /**************************************************************
  ** Lua C Function Wrappers.
  ***************************************************************/
  void openlibs();

  // This will load the given lua file (which puts the code into
  // a function which is then pushed onto the stack) and will
  // then run the function. If it raises an
  // error, that error could represent either a failure to load
  // the file or an exception thrown while running it.
  lua_valid dofile( char const* file );
  lua_valid dofile( std::string const& file );

  // Returns the index of the top element in the stack. Because
  // indices start at 1, this result is equal to the number of
  // elements in the stack; in particular, 0 means empty stack.
  int gettop() const;
  int stack_size() const;

  lua_valid setglobal( char const* key );
  lua_valid setglobal( std::string const& key );

  // Gets the global named `name` and pushes it onto the stack.
  // Returns the type of the object. If the object doesn't exist
  // then it will push nil.
  lua_expect<e_lua_type> getglobal( char const* name );
  lua_expect<e_lua_type> getglobal( std::string const& name );

  lua_valid loadstring( char const* script );
  lua_valid loadstring( std::string const& script );

  lua_valid dostring( char const* script );
  lua_valid dostring( std::string const& script );

  // If this function returns `valid` then `nresults` from the
  // function will be pushed onto the stack. If it returns an
  // error then nothing needs to be popped from the stack. In all
  // cases, the function and arguments will be popped.
  lua_valid pcall( int nargs, int nresults );

  void push( nil_t );
  // We need to take these "safe" versions otherwise we get im-
  // plicit conversions and ambiguities that mess things up. Note
  // that we don't have one for unsigned integers, since Lua does
  // not support those (it used to, but they are deprecated). You
  // have to cast to one of the signed types before pushing.
  void push( base::safe::boolean b );
  void push( base::safe::integral<lua_Integer> n );
  void push( base::safe::floating<lua_Number> d );

  // We do not have an overload that takes a char const* because
  // then it has to be zero-terminated, which means that Lua has
  // scan it to see how long it is. We want to implement this
  // using lua_pushlstring which takes a size, thereby saving
  // that effort. So we only want to take string parameters that
  // know their size, which would be std::string and
  // std::string_view. However, if we are accepting the latter,
  // then there does not seem to be any gain by also accepting
  // the former.
  void push( std::string_view sv );

  // Will check-fail if there are not enough elements on the
  // stack.
  void pop( int n = 1 );

  bool                     get( int idx, bool* ) const;
  base::maybe<lua_Integer> get( int idx, lua_Integer* ) const;
  base::maybe<lua_Number>  get( int idx, lua_Number* ) const;
  base::maybe<std::string> get( int idx, std::string* ) const;

  template<typename T>
  auto get( int idx ) const {
    return get( idx, static_cast<T*>( nullptr ) );
  }

  // Returns the type of the value in the given valid index.
  e_lua_type type_of( int idx ) const;

  // This will yield Lua's name for the type.
  char const* type_name( e_lua_type type ) const;

  lua_valid enforce_type_of( int idx, e_lua_type type ) const;

private:
  e_lua_type lua_type_to_enum( int type ) const;

  /**************************************************************
  ** Error checking helpers.
  ***************************************************************/
  void enforce_stack_size_ge( int s ) const;

  void validate_index( int idx ) const;

  [[nodiscard]] lua_error_t pop_and_return_error();

private:
  lua_State* L;
};

} // namespace luapp
