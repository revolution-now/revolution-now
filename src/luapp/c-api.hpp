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
#include "error.hpp"
#include "types.hpp"

// base
#include "base/maybe.hpp"

namespace lua {

/****************************************************************
** c_api
*****************************************************************/
// This is a wrapper around the raw lua C API calls that basi-
// cally do some error checking on parameters and stack sizes, as
// well as reporting errors with base::valid_or. This is intended
// to be an intermediate step or building block to the ultimate
// lua C++ interface.
struct c_api {
  c_api( cthread L_ ) noexcept : L( L_ ) {}

  cthread this_cthread() const noexcept { return L; }
  cthread main_cthread() const noexcept;

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
  // Returns the LUA_MULTRET constant.
  static int multret() noexcept;

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
  void push( lightuserdata p ) noexcept;
  // We need to take these "safe" versions otherwise we get im-
  // plicit conversions and ambiguities that mess things up. Note
  // that we don't have one for unsigned integers, since Lua does
  // not support those (it used to, but they are deprecated). You
  // have to cast to one of the signed types before pushing.
  void push( boolean b ) noexcept;
  void push( integer n ) noexcept;
  void push( floating d ) noexcept;

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

  // Pushes a copy of the element at the given index onto the
  // stack.
  void pushvalue( int idx ) noexcept;

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

  // (table_idx)[-2] = -1. Pops both key and value, leaving
  // table.
  void settable( int table_idx );

  // Pushes (idx)[-1], Pops the key from the stack, but not the
  // table. Returns the type of the pushed value.
  type gettable( int idx );

  // (table_idx)[k] = -1
  void setfield( int table_idx, char const* k );

  // Pushes (table_idx)[k].  Returns type of pushed value.
  type getfield( int table_idx, char const* k );

  // Pushes (idx)[n] onto the stack without invoking __index, and
  // returns type of value pushed.
  type rawgeti( int idx, int n ) noexcept;

  // Does (idx)[n] = -1, but does not invoke the __newindex
  // metamethod. Pops the value from the stack.
  void rawseti( int idx, int n ) noexcept;

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
  type getglobal( char const* name ) noexcept;
  type getglobal( std::string const& name ) noexcept;

  // These versions are much slower, but will run in a protected
  // environment.
  lua_expect<type> getglobal_safe( char const* name ) noexcept;
  lua_expect<type> getglobal_safe(
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
  type geti( int idx, integer i ) noexcept;

  /**************************************************************
  ** registry references
  ***************************************************************/
  // Returns a value that will never be held by any valid reg-
  // istry reference. This can be thought of as a kind of "null"
  // for registry references.
  static int noref() noexcept;

  // Creates and returns a reference, in the table at index t,
  // for the object at the top of the stack (and pops the ob-
  // ject).
  int ref( int idx ) noexcept;
  // Same as above, but for the registry table.
  int ref_registry() noexcept;

  // Pushes (LUA_REGISTRYINDEX)[id] onto the stack, returning the
  // type of value pushed.
  type registry_get( int id ) noexcept;

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
  type udata_getmetatable( char const* tname ) noexcept;

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
  //
  // NOTE: for tables, it only works for arrays, up until the
  // first nil, as in the usual Lua behavior.
  void len( int idx );
  // Same as above, but result is popped from the stack and re-
  // turned.
  int len_pop( int idx );

  // Returns the raw "length" of the value at the given index:
  // for strings, this is the string length; for tables, this is
  // the result of the length operator ('#') with no metamethods;
  // for userdata, this is the size of the block of memory allo-
  // cated for the userdata; for other values, it is 0.
  int rawlen( int idx ) noexcept;

  /**************************************************************
  ** comparison
  ***************************************************************/
  // Compares two Lua values. Returns true if the value at index
  // idx1 satisfies the given operation when compared with the
  // value at index idx2, following the semantics of the corre-
  // sponding Lua operator (that is, it may call metamethods).
  // Otherwise returns false.
  [[nodiscard]] bool compare_eq( int idx1, int idx2 );
  [[nodiscard]] bool compare_lt( int idx1, int idx2 );
  [[nodiscard]] bool compare_le( int idx1, int idx2 );

  /**************************************************************
  ** string conversion
  ***************************************************************/
  // Concatenates the n values at the top of the stack, pops
  // them, and leaves the result at the top. If n is 1, the re-
  // sult is the single value on the stack (that is, the function
  // does nothing)! If n is 0, the result is the empty string.
  // Concatenation is performed following the usual semantics of
  // Lua (see §3.4.6). The types must be either strings or num-
  // bers. Otherwise either the value will be left untouched, or
  // an error will be thrown (in the case of attempting to con-
  // catenate an invalid value to a valid value).
  //
  // To convert a single value to a string, use tostring.
  void concat( int n ) noexcept;

  // Converts any Lua value at the given index to a C string in a
  // reasonable format. The resulting string is pushed onto the
  // stack and also returned by the function. If len is not NULL,
  // the function also sets *len with the string length.
  //
  // If the value has a metatable with a __tostring field, then
  // luaL_tolstring calls the corresponding metamethod with the
  // value as argument, and uses the result of the call as its
  // result.
  //
  // Note: calls luaL_tolstring, and thus it is NOT equivalent to
  // calling get<string>(), which calls lua_tolstring and so only
  // works for strings and numbers.
  //
  // The returned pointer may not be valid after the string gets
  // removed from the stack.
  char const* tostring( int idx, size_t* len ) noexcept;

  // This will call tostring(-1), which will convert any Lua
  // value to a string (potentially callint the __tostring
  // metamethod) and then this method will efficiently extract it
  // into a C++ string. It will then pop the string and the orig-
  // inal value off of the stack. Essentially, this is equivalent
  // to calling the above `tostring` method, popping and putting
  // the result into a C++ string efficiently, then calling pop()
  // once again.
  std::string pop_tostring() noexcept;

  /**************************************************************
  ** threads
  ***************************************************************/
  // Creates a new thread, pushes it on the stack, and returns a
  // pointer to a lua_State that represents this new thread. The
  // new thread returned by this function shares with the orig-
  // inal thread its global environment, but has an independent
  // execution stack.
  //
  // There is no explicit function to close or to destroy a
  // thread. Threads are subject to garbage collection, like any
  // Lua object.
  [[nodiscard]] cthread newthread() noexcept;

  // Pushes the thread held by this object onto the stack. Re-
  // turns true if this thread is the main thread of its state.
  bool pushthread() noexcept;

  /**************************************************************
  ** garbage collection
  ***************************************************************/
  // Runs a full garbage collection cycle. This should probably
  // only be used for testing.
  void gc_collect();

  /**************************************************************
  ** types
  ***************************************************************/
  // Returns the type of the value in the given valid index.
  type type_of( int idx ) const noexcept;

  // This will yield Lua's name for the type.
  char const* type_name( type type ) const noexcept;

  lua_valid enforce_type_of( int idx, type type ) const noexcept;

  // Returns true if the value at the given index is an integer
  // (that is, the value is a number and is represented as an in-
  // teger), and false otherwise.
  bool isinteger( int idx ) const noexcept;

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
  bool                 get( int idx, bool* ) const noexcept;
  boolean              get( int idx, boolean* ) const noexcept;
  base::maybe<integer> get( int idx, integer* ) const noexcept;
  // This is for when you know the result will fit into an int.
  base::maybe<int>      get( int idx, int* ) const noexcept;
  base::maybe<double>   get( int idx, double* ) const noexcept;
  base::maybe<floating> get( int idx, floating* ) const noexcept;
  base::maybe<std::string> get( int idx,
                                std::string* ) const noexcept;
  // This is done as light userdata.
  base::maybe<void*> get( int idx, void** ) const noexcept;
  base::maybe<lightuserdata> get(
      int idx, lightuserdata* ) const noexcept;
  // This is done as light userdata.
  base::maybe<char const*> get( int idx,
                                char const** ) const noexcept;

  type lua_type_to_enum( int type ) const noexcept;

  /**************************************************************
  ** Error checking helpers.
  ***************************************************************/
  void enforce_stack_size_ge( int s ) const noexcept;

  void validate_index( int idx ) const noexcept;

  [[nodiscard]] lua_error_t pop_and_return_error() noexcept;

  // Initialize with a Lua state and whether we own it.
  c_api( cthread state, bool own );

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
                                cthread,
                                Args...> )
  auto pinvoke( int ninputs,
                LuaApiFunc<R, Params...>* func,
                Args... args )
    -> std::conditional_t<std::is_same_v<R, void>,
                          lua_valid,
                          lua_expect<R>>;
  // clang-format on

  // Not necessarily the main thread.
  cthread L;
};

// For convenience.
template<typename T>
void push( c_api& C, T&& o ) {
  push( C.this_cthread(), std::forward<T>( o ) );
}

// For convenience.
template<typename T>
base::maybe<T> get( c_api& C, int idx, T* p ) {
  return get( C.this_cthread(), idx, p );
}

} // namespace lua
