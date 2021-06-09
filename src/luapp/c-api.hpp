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
  // Initialize with a Lua state and whether we own it.
  c_api( ::lua_State* state, bool own );
  ~c_api() noexcept;

  /**************************************************************
  ** Lua C Function Wrappers.
  ***************************************************************/
  void openlibs() noexcept;

  // Returns the index of the top element in the stack. Because
  // indices start at 1, this result is equal to the number of
  // elements in the stack; in particular, 0 means empty stack.
  int gettop() const noexcept;
  int stack_size() const noexcept;

  /**************************************************************
  ** running lua code
  ***************************************************************/
  // This will load the given lua file by loading the code into a
  // function with varargs, then running the function. If it
  // raises an error, that error could represent either a failure
  // to load the file or an exception thrown while running it.
  lua_valid dofile( char const* file ) noexcept;
  lua_valid dofile( std::string const& file ) noexcept;

  // Loads the string into a function which is pushed onto the
  // stack, but does not run it.
  lua_valid loadstring( char const* script ) noexcept;

  // Runs the string of lua code, first wrapping it in a function
  // with varargs parameters.
  lua_valid dostring( char const* script ) noexcept;

  /**************************************************************
  ** call / pcall
  ***************************************************************/
  // Calls (-nargs-1)( ... )
  void call( int nargs, int nresults ) noexcept;

  // If this function returns `valid` then `nresults` from the
  // function will be pushed onto the stack. If it returns an
  // error then nothing needs to be popped from the stack. In all
  // cases, the function and arguments will be popped.
  lua_valid pcall( int nargs, int nresults ) noexcept;

  /**************************************************************
  ** pushing & popping, stack manipulation
  ***************************************************************/
  // Pushes _G.
  void pushglobaltable() noexcept;

  void push( nil_t ) noexcept;
  void push( LuaCFunction* f, int upvalues = 0 ) noexcept;
  void push( base::safe::void_p p ) noexcept;
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

  // Moves the top element into the given valid index, shifting
  // up the elements above this index to open space. This func-
  // tion cannot be called with a pseudo-index, because a
  // pseudo-index is not an actual stack position.
  //
  //           [A]                            [B]
  //           [B]    --> insert( -3 ) -->    [C]
  //           [C]           -or-             [A]
  //           [D]      rotate( -3, 1 )       [D]
  //
  void insert( int idx ) noexcept;

  // Swap the top two elements on the stack. There must be at
  // least two items on the stack.
  void swap_top() noexcept;

  /**************************************************************
  ** Table manipulation
  ***************************************************************/
  // Creates a new table and pushes it onto the stack.
  void newtable() noexcept;

  // (table_idx)[-2] = -1
  void settable( int table_idx ) noexcept;

  // (table_idx)[k] = -1
  void setfield( int table_idx, char const* k ) noexcept;

  // Pushes (table_idx)[k].  Returns type of pushed value.
  e_lua_type getfield( int table_idx, char const* k ) noexcept;

  // Pushes (idx)[n] onto the stack without invoking __index, and
  // returns type of value pushed.
  e_lua_type rawgeti( int idx, lua_Integer n ) noexcept;

  // Does (idx)[n] = -1, but does not invoke the __newindex
  // metamethod. Pops the value from the stack.
  void rawseti( int idx, lua_Integer n ) noexcept;

  /**************************************************************
  ** get/set globals
  ***************************************************************/
  void setglobal( char const* key ) noexcept;
  void setglobal( std::string const& key ) noexcept;

  // These versions are much slower, but will run in a protected
  // environment.
  lua_valid setglobal_safe( char const* key ) noexcept;
  lua_valid setglobal_safe( std::string const& key ) noexcept;

  // Gets the global named `name` and pushes it onto the stack.
  // Returns the type of the object. If the object doesn't exist
  // then it will push nil.
  e_lua_type getglobal( char const* name ) noexcept;
  e_lua_type getglobal( std::string const& name ) noexcept;

  // These versions are much slower, but will run in a protected
  // environment.
  lua_expect<e_lua_type> getglobal_safe(
      char const* name ) noexcept;
  lua_expect<e_lua_type> getglobal_safe(
      std::string const& name ) noexcept;

  /**************************************************************
  ** Get value from stack
  ***************************************************************/
  template<typename T>
  auto get( int idx ) const noexcept {
    return get( idx, static_cast<T*>( nullptr ) );
  }

  // Pushes onto the stack the value t[i], where t is the value
  // at the given index. As in Lua, this function may trigger a
  // metamethod for the "index" event.4). Returns the type of the
  // pushed value.
  e_lua_type geti( int idx, lua_Integer i ) noexcept;

  /**************************************************************
  ** registry references
  ***************************************************************/
  // Creates and returns a reference, in the table at index t,
  // for the object at the top of the stack (and pops the ob-
  // ject).
  int ref( int idx ) noexcept;
  // Same as above, but for the registry table.
  int ref_registry() noexcept;

  // Pushes (LUA_REGISTRYINDEX)[id] onto the stack, returning the
  // type of value pushed.
  e_lua_type registry_get( int id ) noexcept;

  // Releases reference ref from the table at index t (see
  // luaL_ref). The entry is removed from the table, so that the
  // referred object can be collected. The reference ref is also
  // freed to be used again. If ref is LUA_NOREF or LUA_REFNIL,
  // luaL_unref does nothing.
  void unref( int t, int ref ) noexcept;
  // Same as above but for the registry table.
  void unref_registry( int ref ) noexcept;

  /**************************************************************
  ** metatables
  ***************************************************************/
  // Pops a table from the stack and sets it as the new metatable
  // for the value at the given index.
  void setmetatable( int idx ) noexcept;

  // If the value at the given index has a metatable, the func-
  // tion pushes that metatable onto the stack and returns true.
  // Otherwise, the function returns false and pushes nothing on
  // the stack.
  bool getmetatable( int idx ) noexcept;

  /**************************************************************
  ** upvalues
  ***************************************************************/
  // Gets information about the n-th upvalue of the closure at
  // index funcindex. It pushes the upvalue's value onto the
  // stack. This is really only useful for C functions. Hence,
  // the raw Lua function, which returns the variable name of the
  // up value, is suppressed here, because upvalues of C func-
  // tions have no names (empty strings).
  //
  // Returns false (and pushes nothing) when the index n is
  // greater than the number of upvalues. Otherwise returns true.
  bool getupvalue( int funcindex, int n ) noexcept;

  /**************************************************************
  ** userdata
  ***************************************************************/
  // This function allocates a new block of memory with the given
  // size, pushes onto the stack a new full userdata with the
  // block address, and returns this address. The host program
  // can freely use this memory.
  void* newuserdata( int size ) noexcept;

  // If the registry already has the key tname, returns false.
  // Otherwise, creates a new table to be used as a metatable for
  // userdata, adds to this new table the pair __name = tname,
  // adds to the registry the pair [tname] = new table, and re-
  // turns true.
  //
  // In both cases pushes onto the stack the final value associ-
  // ated with tname in the registry.
  bool udata_newmetatable( char const* tname ) noexcept;

  // Pushes onto the stack the metatable associated with name
  // tname in the registry (nil if there is no metatable associ-
  // ated with that name). Returns the type of the pushed value.
  // This is used for userdata.
  e_lua_type udata_getmetatable( char const* tname ) noexcept;

  // Sets the metatable of the object at the top of the stack as
  // the metatable associated with name tname in the registry
  // (see luaL_newmetatable).
  void udata_setmetatable( char const* tname ) noexcept;

  // Checks whether the function argument arg is a userdata of
  // the type tname (see luaL_newmetatable) and returns the user-
  // data address (see lua_touserdata). If it is not, then it
  // raises an error.
  void* checkudata( int arg, char const* tname ) noexcept;

  // This function works like luaL_checkudata, except that, when
  // the test fails, it returns NULL instead of raising an error.
  void* testudata( int arg, char const* tname ) noexcept;

  /**************************************************************
  ** length
  ***************************************************************/
  // Returns the length of the value at the given index. It is
  // equivalent to the '#' operator in Lua (see §3.4.7) and may
  // trigger a metamethod for the "length" event (see §2.4). The
  // result is pushed on the stack since it could theoretically
  // be an object of any type (because of the __len metamethod).
  void len( int idx ) noexcept;
  // Same as above, but result is popped from the stack and re-
  // turned.
  int len_pop( int idx ) noexcept;

  /**************************************************************
  ** types
  ***************************************************************/
  // Returns the type of the value in the given valid index.
  e_lua_type type_of( int idx ) const noexcept;

  // This will yield Lua's name for the type.
  char const* type_name( e_lua_type type ) const noexcept;

  lua_valid enforce_type_of( int        idx,
                             e_lua_type type ) const noexcept;

  /**************************************************************
  ** error
  ***************************************************************/
  // Generates a Lua error, using the value at the top of the
  // stack as the error object. This function does a long jump,
  // and therefore never returns (see luaL_error).
  //
  // These are not noexcept because, in general, Lua needs to be
  // able to throw from these functions. This matters when we
  // need to call one of these from a Lua C function to throw a
  // Lua error that can be caught by Lua.
  void error() noexcept( false );
  void error( std::string const& msg ) noexcept( false );

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
  // Do we own the Lua state.
  bool own_;
};

} // namespace luapp
