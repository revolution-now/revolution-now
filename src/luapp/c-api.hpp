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
#include "ext.hpp"
#include "thread-status.hpp"
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

  cthread this_cthread() noexcept { return L; }
  cthread main_cthread() noexcept;

  /**************************************************************
  ** Lua C Function Wrappers.
  ***************************************************************/
  void openlibs() noexcept;

  // Returns the index of the top element in the stack. Because
  // indices start at 1, this result is equal to the number of
  // elements in the stack; in particular, 0 means empty stack.
  int gettop() noexcept;
  // Could throw because it could cause __close to be called on
  // some objects.
  void settop( int top );
  int  stack_size() noexcept;

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

  // Loads a file; if the loading is successful then it will be
  // pushed on the stack as a function (not run). If the loading
  // is not successfull then there will be an error message re-
  // turned.
  lua_valid loadfile( const char* filename );

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

  // This function behaves exactly like lua_pcall, except that it
  // allows the called function to yield.
  //
  // NOTE: If/when a yield happens, it will be done (by Lua) by
  // throwing an exception which is of type int in practice, and
  // so this function is really only intended to be run under a
  // protected environment, particularly lua_resume.
  //
  // NOTE: this function will leave the message handler on the
  // stack since it would be tricky to clean it up given that the
  // function can yield and resume. So it is the caller's respon-
  // sibility to remove it if necessary (which would have to be
  // done in both the function that calls pcallk and in the con-
  // tinuation).
  //
  // In the case of an error or a yield, this function will not
  // return. It only returns when the function finished success-
  // fully without yielding, and so there is no point in having a
  // return value. A typical scenario is:
  //
  //   1. lua_resume starts a coroutine that, at some point in
  //      the call stack, calls a C function.
  //   2. The C function wants to call into Lua in a way that al-
  //      lows Lua to yield (and it wants to trap errors like
  //      pcall normally does).
  //   3. The C function calls pcallk which calls into Lua, and
  //      then Lua throws an error.
  //   4. The Lua interpreter C code throws an exceptions which
  //      unwinds the C call stack back through this call to
  //      pcallk (so pcallk will never return).
  //   5. lua_resume catches the C++ exception and it then starts
  //      to unwind the Lua stack, until it hits the frame repre-
  //      senting the C function that called lua_pcallk.
  //   6. It cannot resume that function since its call frame is
  //      gone, so it just calls the continuation function (`k`)
  //      passed into pcallk.
  //   7. When the continuation function returns, Lua resumes,
  //      the Lua function that called the original C function.
  //
  void pcallk( int nargs, int nresults, LuaKContext ctx,
               LuaKFunction k );

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

  // Pushes (idx)[-1], popping key from stack, but not table. Un-
  // like lua_gettable, this will not consult the metatable.
  // Pushes the resulting value and returns its type.
  type rawget( int idx ) noexcept;

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
  auto get( int idx ) noexcept {
    return get( idx, static_cast<T*>( nullptr ) );
  }

  // Pushes onto the stack the value t[i], where t is the value
  // at the given index. As in Lua, this function may trigger a
  // metamethod for the "index" event.4). Returns the type of the
  // pushed value.
  type geti( int idx, integer i ) noexcept;

  // Checks whether the function argument arg is an integer (or
  // can be converted to an integer) and returns this integer
  // cast to a lua_Integer. `arg` should be positive here since
  // it is referring to an argument. This will raise a Lua error
  // if it can't deliver an integer.
  int checkinteger( int arg );

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
  ** iteration
  ***************************************************************/
  // Pops a key from the stack, and pushes a key–value pair from
  // the table at the given index (the "next" pair after the
  // given key). If there are no more elements in the table, then
  // next returns false (and pushes nothing).
  //
  // This does NOT call the __pairs method.
  bool next( int idx ) noexcept;

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

  // Converts the value at the given index to a Lua thread (rep-
  // resented as lua_State*). This value must be a thread; other-
  // wise, the function returns NULL.
  cthread tothread( int idx );

  // Returns the status of the thread L.
  //
  // The status can be LUA_OK for a normal thread, an error code
  // if the thread finished the execution of a lua_resume with an
  // error, or LUA_YIELD if the thread is suspended.
  //
  // You can call functions only in threads with status LUA_OK.
  // You can resume threads with status LUA_OK (to start a new
  // coroutine) or LUA_YIELD (to resume a coroutine).
  //
  // WARNING: if the thread status is error then you should not
  // push anything onto the stack until it is reset (this seems
  // to yield a Lua stack overflow).
  thread_status status() noexcept;

  // This is equivalent to calling Lua's coroutine.status. The
  // thread (coroutine) is taken to be the one associated with
  // this c_api object. The only difference is that the result
  // does not include the "running" option, since that wouldn't
  // make sense given that this is a C function not called from
  // Lua.
  coroutine_status coro_status() noexcept;

  // If the thread state is not in error then this will return
  // valid, otherwise it will assume that the error object is on
  // the top of the stack and will return it WITHOUT popping it
  // from the stack. So this function can be called multiple
  // times on the same thread.
  //
  // WARNING: if the thread status is error then you should not
  // push anything onto the stack until it is reset (this seems
  // to yield a Lua stack overflow).
  lua_valid thread_ok() noexcept;

  // Resets a thread, cleaning its call stack and closing all
  // pending to-be-closed variables. In case of error (either the
  // original error that stopped the thread or errors in closing
  // methods), leaves the error object on the top of the stack
  // and also returns it.
  lua_valid resetthread() noexcept;

  // Starts and resumes a coroutine in the given thread
  // L_toresume.
  //
  // To start a coroutine, you push the main function plus any
  // arguments onto the empty stack of the thread. then you call
  // resume, with nargs being the number of arguments. This call
  // returns when the coroutine suspends or finishes its execu-
  // tion. When it returns, the return value will contain either
  // an error (in which case the coroutine failed with an error)
  // or it will contain the function/yield results. Specifically,
  // it returns LUA_YIELD if the coroutine yields, LUA_OK if the
  // coroutine finishes its execution without errors, or an error
  // code in case of errors.4.1).
  //
  // In case of errors, the error object is left on the top of
  // the stack, but it is returned in the lua_expect object.
  //
  // To resume a coroutine, you remove the *nresults yielded
  // values from its stack, push the values to be passed as re-
  // sults from yield, and then call resume.
  //
  // NOTE: you should probably prefer to call resume_or_reset,
  // since that one will guarantee that if the coroutine fails
  // with an error that all of it's to-be-closed variables will
  // be closed. This one is suffixed with _or_leak to indicate
  // that if it finishes with an error, the thread will not be
  // closed, and so any to-be-closed variables will not be closed
  // and hence may leak resources (and the documentation seems to
  // say that they won't even be closed when the thread is
  // garbage collected). Therefore, the caller must take care to
  // call resetthread on the L_toresume thread at some point if
  // this function fails. The `resume_or_reset' function handles
  // this automatically, and so should probably be preferred.
  //
  // This function is noexcept because when an error is thrown
  // from another thread, it will not translate to an error in
  // the calling thread.
  //
  // WARNING: if the thread status is error then you should not
  // push anything onto the stack until it is reset (this seems
  // to yield a Lua stack overflow).
  lua_expect<resume_result> resume_or_leak( cthread L_toresume,
                                            int nargs ) noexcept;

  // Same as above, but will call resetthread on L_toresume if it
  // finishes with an error (this is done in order to ensure that
  // all to-be-closed variables get closed). In such a case, the
  // error returned will be the original error that terminated
  // the thread, and any errors that happen during closing will
  // be logged, but then dropped.
  lua_expect<resume_result> resume_or_reset(
      cthread L_toresume, int nargs ) noexcept;

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
  type type_of( int idx ) noexcept;

  // This will yield Lua's name for the type.
  char const* type_name( type type ) noexcept;

  lua_valid enforce_type_of( int idx, type type ) noexcept;

  // Returns true if the value at the given index is an integer
  // (that is, the value is a number and is represented as an in-
  // teger), and false otherwise.
  bool isinteger( int idx ) noexcept;

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
  [[noreturn]] void error() noexcept( false );
  [[noreturn]] void error( std::string const& msg ) noexcept(
      false );

  /**************************************************************
  ** Debugging
  ***************************************************************/
  void print_stack( std::string_view label = "" ) noexcept;

private:
  bool                 get( int idx, bool* ) noexcept;
  boolean              get( int idx, boolean* ) noexcept;
  base::maybe<integer> get( int idx, integer* ) noexcept;
  // This is for when you know the result will fit into an int.
  base::maybe<int>         get( int idx, int* ) noexcept;
  base::maybe<double>      get( int idx, double* ) noexcept;
  base::maybe<floating>    get( int idx, floating* ) noexcept;
  base::maybe<std::string> get( int idx, std::string* ) noexcept;
  // This is done as light userdata.
  base::maybe<void*>         get( int idx, void** ) noexcept;
  base::maybe<lightuserdata> get( int idx,
                                  lightuserdata* ) noexcept;
  // This is done as light userdata.
  base::maybe<char const*> get( int idx, char const** ) noexcept;

  type lua_type_to_enum( int type ) noexcept;

  /**************************************************************
  ** Error checking helpers.
  ***************************************************************/
  void enforce_stack_size_ge( int s ) noexcept;

  void validate_index( int idx ) noexcept;

  int pcall_preamble( int nargs, int nresults ) noexcept;

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
    -> error_type_for_return_type<R>;
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
base::maybe<T> get( c_api& C, int idx, tag<T> t ) {
  return get( C.this_cthread(), idx, t );
}

/****************************************************************
** Macro helpers
*****************************************************************/
// This requires including base/scope-exit.hpp and also requires
// C to be in scope.
#define SCOPE_CHECK_STACK_UNCHANGED         \
  int starting_stack_size = C.stack_size(); \
  SCOPE_EXIT( CHECK_EQ( C.stack_size(), starting_stack_size ) )

} // namespace lua
