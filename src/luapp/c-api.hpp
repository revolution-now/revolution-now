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

// base
#include "base/valid.hpp"

// {fmt}
#include "fmt/format.h"

// Lua
#include "lua.h"

namespace luapp {

using lua_error_t = std::string;

using lua_valid = base::valid_or<lua_error_t>;

template<typename T>
using lua_expect = base::expect<T, lua_error_t>;

template<typename T>
lua_expect<T> lua_expected( T&& arg ) {
  return base::expected<lua_error_t>( std::forward<T>( arg ) );
}

template<typename T, typename Arg>
lua_expect<T> lua_unexpected( Arg&& arg ) {
  return base::unexpected<T, lua_error_t>(
      std::forward<Arg>( arg ) );
}

enum class e_lua_type {
  nil            = LUA_TNIL,
  boolean        = LUA_TBOOLEAN,
  light_userdata = LUA_TLIGHTUSERDATA,
  number         = LUA_TNUMBER,
  string         = LUA_TSTRING,
  table          = LUA_TTABLE,
  function       = LUA_TFUNCTION,
  userdata       = LUA_TUSERDATA,
  thread         = LUA_TTHREAD,
};

inline constexpr int kNumLuaTypes = 9;

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

  struct pcall_options {
    // Setting these to -1 allows us to ensure that the user has
    // explicitly set them.
    int nargs    = -1;
    int nresults = -1;
  };

  // If this function returns `valid` then `nresults` from the
  // function will be pushed onto the stack. If it returns an
  // error then nothing needs to be popped from the stack. In all
  // cases, the function and arguments will be popped.
  lua_valid pcall( pcall_options const& o );

  // Will check-fail if there are not enough elements on the
  // stack.
  void pop( int n = 1 );

  // Returns the type of the value in the given valid index.
  e_lua_type type_of( int idx );

  // This will yield Lua's name for the type.
  char const* type_name( e_lua_type type );

  /**************************************************************
  ** Error checking helpers.
  ***************************************************************/
  void enforce_stack_size_ge( int s );

  lua_valid enforce_type_of( int idx, e_lua_type type );

private:
  e_lua_type lua_type_to_enum( int type ) const;

  lua_State* L;
};

/****************************************************************
** to_str
*****************************************************************/
void to_str( luapp::e_lua_type t, std::string& out );

} // namespace luapp

/****************************************************************
** {fmt}
*****************************************************************/
namespace fmt {

// {fmt} formatter for e_lua_type.
template<>
struct formatter<luapp::e_lua_type>
  : fmt::formatter<std::string> {
  using formatter_base = fmt::formatter<std::string>;
  template<typename FormatContext>
  auto format( luapp::e_lua_type o, FormatContext& ctx ) {
    std::string res;
    to_str( o, res );
    return formatter_base::format( res, ctx );
  }
};

} // namespace fmt
