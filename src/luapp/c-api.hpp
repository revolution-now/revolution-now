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

// This represents the signature of a Lua C API function that in-
// teracts with a Lua state (i.e., takes the Lua state as first
// parameter). Any such API function could interact with the Lua
// state and thus could potentially throw an error (at least most
// of them do). So the code that wraps Lua C API calls to detect
// those errors will use this signature.
//
// Takes args by value since they will only be simple types.
template<typename R, typename... Args>
using LuaApiFunc = R( ::lua_State*, Args... );

// This represents the signature of a Lua C library (extension)
// method, i.e., a C function that is called from Lua.
using LuaCFunction = int( ::lua_State* );

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
  c_api( ::lua_State* state );
  ~c_api() noexcept;

  /**************************************************************
  ** Lua C Function Wrappers.
  ***************************************************************/
  void openlibs() noexcept;

  // This will load the given lua file (which puts the code into
  // a function which is then pushed onto the stack) and will
  // then run the function. If it raises an
  // error, that error could represent either a failure to load
  // the file or an exception thrown while running it.
  lua_valid dofile( char const* file ) noexcept;
  lua_valid dofile( std::string const& file ) noexcept;

  // Returns the index of the top element in the stack. Because
  // indices start at 1, this result is equal to the number of
  // elements in the stack; in particular, 0 means empty stack.
  int gettop() const noexcept;
  int stack_size() const noexcept;

  lua_valid setglobal( char const* key ) noexcept;
  lua_valid setglobal( std::string const& key ) noexcept;

  // Gets the global named `name` and pushes it onto the stack.
  // Returns the type of the object. If the object doesn't exist
  // then it will push nil.
  lua_expect<e_lua_type> getglobal( char const* name ) noexcept;
  lua_expect<e_lua_type> getglobal(
      std::string const& name ) noexcept;

  lua_valid loadstring( char const* script ) noexcept;
  lua_valid loadstring( std::string const& script ) noexcept;

  lua_valid dostring( char const* script ) noexcept;
  lua_valid dostring( std::string const& script ) noexcept;

  // If this function returns `valid` then `nresults` from the
  // function will be pushed onto the stack. If it returns an
  // error then nothing needs to be popped from the stack. In all
  // cases, the function and arguments will be popped.
  lua_valid pcall( int nargs, int nresults ) noexcept;

  void push( nil_t ) noexcept;
  void push( LuaCFunction* f ) noexcept;
  void push( void* f ) noexcept;
  // We need to take these "safe" versions otherwise we get im-
  // plicit conversions and ambiguities that mess things up. Note
  // that we don't have one for unsigned integers, since Lua does
  // not support those (it used to, but they are deprecated). You
  // have to cast to one of the signed types before pushing.
  void push( base::safe::boolean b ) noexcept;
  void push( base::safe::integral<lua_Integer> n ) noexcept;
  void push( base::safe::floating<lua_Number> d ) noexcept;

  // We do not have an overload that takes a char const* because
  // then it has to be zero-terminated, which means that Lua has
  // scan it to see how long it is. We want to implement this
  // using lua_pushlstring which takes a size, thereby saving
  // that effort. So we only want to take string parameters that
  // know their size, which would be std::string and
  // std::string_view. However, if we are accepting the latter,
  // then there does not seem to be any gain by also accepting
  // the former.
  void push( std::string_view sv ) noexcept;

  // Will check-fail if there are not enough elements on the
  // stack.
  void pop( int n = 1 ) noexcept;

  // Rotates a window starting at idx n times in the direction of
  // the top of the stack.
  void rotate( int idx, int n ) noexcept;

  template<typename T>
  auto get( int idx ) const noexcept {
    return get( idx, static_cast<T*>( nullptr ) );
  }

  // Returns the type of the value in the given valid index.
  e_lua_type type_of( int idx ) const noexcept;

  // This will yield Lua's name for the type.
  char const* type_name( e_lua_type type ) const noexcept;

  lua_valid enforce_type_of( int        idx,
                             e_lua_type type ) const noexcept;

private:
  bool                     get( int idx, bool* ) const noexcept;
  base::maybe<lua_Integer> get( int idx,
                                lua_Integer* ) const noexcept;
  // This is for when you know the result will fit into an int.
  base::maybe<int>         get( int idx, int* ) const noexcept;
  base::maybe<lua_Number>  get( int idx,
                                lua_Number* ) const noexcept;
  base::maybe<std::string> get( int idx,
                                std::string* ) const noexcept;
  // This is done as light userdata.
  base::maybe<void*> get( int idx, void** ) const noexcept;
  // This is done as light userdata.
  base::maybe<char const*> get( int idx,
                                char const** ) const noexcept;

  e_lua_type lua_type_to_enum( int type ) const noexcept;

  /**************************************************************
  ** Error checking helpers.
  ***************************************************************/
  void enforce_stack_size_ge( int s ) const noexcept;

  void validate_index( int idx ) const noexcept;

  [[nodiscard]] lua_error_t pop_and_return_error() noexcept;

private:
  c_api( c_api const& ) = delete;
  c_api( c_api&& )      = delete;
  c_api& operator=( c_api const& ) = delete;
  c_api& operator=( c_api&& ) = delete;

  // This is used as a wrapper to invoke a Lua C API method, such
  // as e.g. ::lua_getglobal. Functions like that can cause er-
  // rors to be thrown (in the case of ::lua_getglobal, it might
  // run the __index metamethod of the global table to get the
  // value and in doing so an error could be thrown), and so they
  // must be called under the auspices of pcall otherwise an
  // error will cause the process to immediately terminate.
  //
  // Args and Params are all by value, since we assume that we're
  // only going to be dealing with primitive types (objects and
  // strings get passed in as pointers).
  //
  // clang-format off
  template<typename R, typename... Params, typename... Args>
  requires( sizeof...( Params ) == sizeof...( Args ) &&
            std::is_invocable_v<LuaApiFunc<R, Params...>*,
                                ::lua_State*,
                                Args...> )
  auto pinvoke( int ninputs,
                LuaApiFunc<R, Params...>* func,
                Args... args )
    -> std::conditional_t<std::is_same_v<R, void>,
                          lua_valid,
                          lua_expect<R>>;
  // clang-format on

  lua_State* L;
};

} // namespace luapp
