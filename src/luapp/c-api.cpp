/****************************************************************
**c-api.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-27.
*
* Description: Simple wrapper for Lua C API calls.
*
*****************************************************************/
#include "c-api.hpp"

// base
#include "base/error.hpp"
#include "base/scope-exit.hpp"

// Lua
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

// C++ standard library
#include <array>
#include <cmath>

using namespace std;

// Number of arguments that the caller is expected to have pushed
// before calling this.
#define DECLARE_NUM_CONSUMED_VALUES( n ) \
  int ninputs = n;                       \
  enforce_stack_size_ge( ninputs )

namespace lua {
namespace {

static_assert( is_same_v<LuaKContext, lua_KContext> );

using ::base::maybe;
using ::base::nothing;

// Not sure if these are mandatory, but if they fail they will at
// least alert us that something has changed.
static_assert( sizeof( integer ) == sizeof( long long ) );
static_assert( sizeof( long long ) >= 8 );

// This is used when we are pushing values onto the stack and all
// pointers should be pushed as light userdata. If we didn't use
// this, e.g. pushing a `char const*` would push it as a string
// instead of a light user data.
template<typename T>
auto to_void_star_if_ptr( T v ) {
  if constexpr( is_pointer_v<T> )
    return (void*)v;
  else
    return v;
}

int msghandler( lua_State* L ) {
  const char* msg = lua_tostring( L, 1 );
  // is error object not a string?
  if( msg == NULL ) {
    // does it...
    if( luaL_callmeta( L, 1,
                       "__tostring" ) && // have a metamethod
        lua_type( L, -1 ) ==
            LUA_TSTRING ) // that produces a string?
      return 1;           // Then that is the message.
    else
      msg = lua_pushfstring( L, "(error object is a %s value)",
                             luaL_typename( L, 1 ) );
  }
  luaL_traceback( L, L, msg,
                  1 ); /* append a standard traceback */
  return 1;            // return the traceback.
}

thread_status to_thread_status( int status ) noexcept {
  switch( status ) {
    case LUA_OK: return thread_status::ok;
    case LUA_YIELD: return thread_status::yield;
  }
  return thread_status::err;
}

} // namespace

/****************************************************************
** errors
*****************************************************************/
lua_valid lua_invalid( lua_error_t err ) {
  return base::invalid<lua_error_t>( move( err ) );
}

/****************************************************************
** c_api
*****************************************************************/
void c_api::openlibs() noexcept { luaL_openlibs( L ); }

lua_valid c_api::dofile( char const* file ) noexcept {
  // luaL_dofile: [-0, +?, e]
  if( luaL_dofile( L, file ) ) return pop_and_return_error();
  return base::valid;
}

lua_valid c_api::dofile( std::string const& file ) noexcept {
  return dofile( file.c_str() );
}

int c_api::gettop() noexcept { return lua_gettop( L ); }
int c_api::stack_size() noexcept { return gettop(); }

void c_api::setglobal( char const* key ) noexcept {
  enforce_stack_size_ge( 1 );
  // [-1,+0,e]
  lua_setglobal( L, key );
}

void c_api::setglobal( string const& key ) noexcept {
  return setglobal( key.c_str() );
}

lua_valid c_api::setglobal_safe( char const* key ) noexcept {
  DECLARE_NUM_CONSUMED_VALUES( 1 );
  // [-1,+0,e]
  return pinvoke( ninputs, lua_setglobal, key );
}

lua_valid c_api::setglobal_safe( string const& key ) noexcept {
  return setglobal_safe( key.c_str() );
}

type c_api::getglobal( char const* name ) noexcept {
  return lua_type_to_enum( lua_getglobal( L, name ) );
}

type c_api::getglobal( string const& name ) noexcept {
  return getglobal( name.c_str() );
}

lua_expect<type> c_api::getglobal_safe(
    char const* name ) noexcept {
  DECLARE_NUM_CONSUMED_VALUES( 0 );
  UNWRAP_RETURN( type, pinvoke( ninputs, lua_getglobal, name ) );
  // Should get here even when the name does not exist, in which
  // case it will have returned nil and pushed it onto the
  // stack..
  enforce_stack_size_ge( 1 );
  CHECK_EQ( lua_type_to_enum( type ), type_of( -1 ) );
  return lua_type_to_enum( type );
}

lua_expect<type> c_api::getglobal_safe(
    string const& name ) noexcept {
  return getglobal_safe( name.c_str() );
}

lua_valid c_api::loadstring( char const* script ) noexcept {
  lua_valid res = base::valid;
  // [-0, +1, –]
  if( luaL_loadstring( L, script ) == LUA_OK )
    // Pushes a function onto the stack.
    enforce_stack_size_ge( 1 );
  else
    return pop_and_return_error();
  return res;
}

lua_valid c_api::dostring( char const* script ) noexcept {
  HAS_VALUE_OR_RET( loadstring( script ) );
  enforce_stack_size_ge( 1 );
  return pcall( /*nargs=*/0, /*nresults=*/LUA_MULTRET );
}

// See header file for explanation of this function.
// clang-format off
template<typename R, typename... Params, typename... Args>
requires( sizeof...( Params ) == sizeof...( Args ) &&
          std::is_invocable_v<LuaApiFunc<R, Params...>*,
                              cthread,
                              Args...> )
auto c_api::pinvoke( int ninputs,
                     LuaApiFunc<R, Params...>* func,
                     Args... args )
    -> error_type_for_return_type<R> {
  // clang-format on
  constexpr bool has_result = !is_same_v<R, void>;

  constexpr int kNInputsIdx = 1;
  constexpr int kFuncIdx    = 2;
  // equal to last one.
  constexpr int kNumRunnerArgs = 2;

  auto runner = []( lua_State* L ) -> int {
    c_api api( L );

    // 1. Get number of arguments that were pushed onto the stack
    //    for this Lua API function to consume.
    CHECK( lua_isinteger( L, kNInputsIdx ) );
    UNWRAP_CHECK( ninputs, api.get<int>( kNInputsIdx ) );
    CHECK_GE( ninputs, 0 );

    // 2. Get pointer to Lua API function that we will call.
    //    e.g., this will be a pointer to lua_gettable.
    CHECK( lua_islightuserdata( L, kFuncIdx ) );
    UNWRAP_CHECK( void_func, api.get<void*>( kFuncIdx ) );
    auto* func =
        reinterpret_cast<LuaApiFunc<R, Params...>*>( void_func );
    CHECK( func );

    // 3. Check stack size.
    CHECK_GE( api.stack_size(), int( kNumRunnerArgs + ninputs +
                                     sizeof...( Args ) ) );

    // 4. Pop parameters into tuple.
    int  idx    = kNumRunnerArgs + 1 + ninputs;
    auto getter = [&]<typename Arg>( Arg* ) {
      UNWRAP_CHECK( res, api.get<Arg>( idx ) );
      ++idx;
      return res;
    };
    auto tuple_args = tuple{ L, getter( (Args*)nullptr )... };

    // 5. Now pop everything so that it doesn't get in the way of
    //    our stack size calculation.
    api.pop( sizeof...( Args ) );
    CHECK( api.stack_size() == kNumRunnerArgs + ninputs );
    api.rotate( -ninputs - kNumRunnerArgs, ninputs );
    api.pop( kNumRunnerArgs );

    // At this point on the stack we have only this C function
    // object (just below the zero point which we cannot access),
    // then all of the values that will be popped by the Lua API
    // function we are about to call.
    CHECK( api.stack_size() == ninputs );

    // 6. Run it
    if constexpr( has_result ) {
      R res = apply( func, tuple_args );
      api.push( res );
    } else {
      apply( func, tuple_args );
    }

    return api.stack_size();
  };

  // For sanity checking.
  type tip_type = ninputs > 0 ? type_of( -1 ) : type::nil;

  // 1. Push func onto the stack.
  push( runner );

  // 2. Push initial stack size so that our helper function above
  //    can compute how many results are being returned.
  push( ninputs );

  // 3. Push the actual Lua API function we want to run.
  push( reinterpret_cast<void*>( func ) );

  // 4. Put the initial args behind those that are to be read
  //    by the Lua API function we will call.
  rotate( -ninputs - kNumRunnerArgs - 1, kNumRunnerArgs + 1 );

  // For sanity checking.
  if( ninputs > 0 ) { CHECK( type_of( -1 ) == tip_type ); }

  // k. Push arguments.
  ( push( to_void_star_if_ptr( args ) ), ... );

  // 6. Run it.
  HAS_VALUE_OR_RET( pcall(
      /*ninputs=*/kNumRunnerArgs + ninputs + sizeof...( args ),
      /*nresults=*/LUA_MULTRET ) );

  if constexpr( has_result ) {
    UNWRAP_CHECK( ret, get<R>( -1 ) );
    pop();
    return ret;
  } else {
    return base::valid;
  }
}

int c_api::multret() noexcept { return LUA_MULTRET; }

int c_api::pcall_preamble( int nargs, int nresults ) noexcept {
  CHECK( nargs >= 0 );
  CHECK( nresults >= 0 || nresults == LUA_MULTRET );
  // Function object plus args should be on the stack at least.
  enforce_stack_size_ge( nargs + 1 );
  // Put the message handler on the stack below the function.
  int fn_idx = gettop() - nargs; // function index.
  push( msghandler );
  // Put handler under function and args.
  insert( fn_idx );
  int msghandler_idx = fn_idx;
  return msghandler_idx;
}

lua_valid c_api::pcall( int nargs, int nresults ) noexcept {
  int msghandler_idx = pcall_preamble( nargs, nresults );
  DCHECK( msghandler_idx > 0 );
  // Remove message handler from the stack. This index will re-
  // main valid because it is positive.
  SCOPE_EXIT( lua_remove( L, msghandler_idx ) );

  // No matter what happens, lua_pcall will remove the function
  // and arguments from the stack.
  int status = lua_pcall( L, nargs, nresults, msghandler_idx );

  if( status == LUA_OK ) return base::valid;
  return pop_and_return_error();
}

lua_valid c_api::pcallk( int nargs, int nresults,
                         LuaKContext ctx, LuaKFunction k ) {
  int msghandler_idx = pcall_preamble( nargs, nresults );
  DCHECK( msghandler_idx > 0 );
  // !! Note that we cannot use SCOPE_EXIT to remove the message
  // handler from the stack here because it will get called pre-
  // maturely if the lua_pcallk yields, so it is unfortunately
  // the responsibility of the caller of pcallk to do it.

  // No matter what happens, lua_pcallk will remove the function
  // and arguments from the stack when it returns. But note that
  // it may never return, if the function being called yields.
  int status =
      lua_pcallk( L, nargs, nresults, msghandler_idx, ctx, k );

  // NOTE: we may never get here if the function yields. In that
  // case, the continuation k will be called.
  if( status == LUA_OK ) return base::valid;
  return pop_and_return_error();
}

void c_api::call( int nargs, int nresults ) noexcept {
  CHECK( nargs >= 0 );
  CHECK( nresults >= 0 || nresults == LUA_MULTRET );
  // Function object plus args should be on the stack at least.
  enforce_stack_size_ge( nargs + 1 );
  // No matter what happens, lua_call will remove the function
  // and arguments from the stack.
  lua_call( L, nargs, nresults );
  if( nresults != LUA_MULTRET )
    enforce_stack_size_ge( nresults );
}

void c_api::pushglobaltable() noexcept {
  lua_pushglobaltable( L );
}

void c_api::push( nil_t ) noexcept { lua_pushnil( L ); }

void c_api::push( LuaCFunction* f, int upvalues ) noexcept {
  enforce_stack_size_ge( upvalues );
  lua_pushcclosure( L, f, upvalues );
}

void c_api::push( lightuserdata p ) noexcept {
  lua_pushlightuserdata( L, p );
}

void c_api::push( boolean b ) noexcept {
  int b_int = b; // just to make it explicit.
  lua_pushboolean( L, b_int );
}

void c_api::push( integer n ) noexcept {
  lua_pushinteger( L, n );
}

void c_api::push( floating d ) noexcept {
  lua_pushnumber( L, d );
}

void c_api::push( string_view sv ) noexcept {
  CHECK( sv.data() != nullptr );
  // Pushes the string pointed to by s with size len onto the
  // stack. Lua makes (or reuses) an internal copy of the given
  // string, so the memory at s can be freed or reused immedi-
  // ately after the function returns. The string can contain any
  // binary data, including embedded zeros.
  //
  // Returns a pointer to the internal copy of the string.
  // [-0, +1, m]
  lua_pushlstring( L, sv.data(), sv.size() );
}

void c_api::pop( int n ) noexcept {
  CHECK_GE( stack_size(), n );
  lua_pop( L, n );
}

void c_api::rotate( int idx, int n ) noexcept {
  validate_index( idx );
  // [-0,+0,–]
  lua_rotate( L, idx, n );
}

void c_api::insert( int idx ) noexcept {
  validate_index( idx );
  lua_insert( L, idx );
}

void c_api::swap_top() noexcept {
  enforce_stack_size_ge( 2 );
  rotate( -2, 1 );
}

void c_api::newtable() noexcept { lua_newtable( L ); }

// (table_idx)[-2] = -1
void c_api::settable( int table_idx ) {
  validate_index( table_idx );
  lua_settable( L, table_idx );
}

// (table_idx)[k] = -1
void c_api::setfield( int table_idx, char const* k ) {
  validate_index( table_idx );
  lua_setfield( L, table_idx, k );
}

type c_api::getfield( int table_idx, char const* k ) {
  validate_index( table_idx );
  return lua_type_to_enum( lua_getfield( L, table_idx, k ) );
}

type c_api::gettable( int idx ) {
  validate_index( idx );
  return lua_type_to_enum( lua_gettable( L, idx ) );
}

type c_api::rawgeti( int idx, int n ) noexcept {
  validate_index( idx );
  return lua_type_to_enum( lua_rawgeti( L, idx, n ) );
}

void c_api::rawseti( int idx, int n ) noexcept {
  validate_index( idx );
  lua_rawseti( L, idx, n );
}

bool c_api::get( int idx, bool* ) noexcept {
  validate_index( idx );
  // Converts the Lua value at the given index to a C boolean
  // value (0 or 1). Like all tests in Lua, lua_toboolean returns
  // true for any Lua value different from false and nil; other-
  // wise it returns false. (If you want to accept only actual
  // boolean values, use lua_isboolean to test the value's type.)
  // [-0, +0, –]
  int i = lua_toboolean( L, idx );
  CHECK( i == 0 || i == 1 );
  return bool( i );
}

boolean c_api::get( int idx, boolean* ) noexcept {
  return get<bool>( idx );
}

maybe<integer> c_api::get( int idx, integer* ) noexcept {
  validate_index( idx );
  int is_num = 0;
  // Converts the Lua value at the given index to the signed in-
  // tegral type lua_Integer. The Lua value must be an integer,
  // or a number or string convertible to an integer (see
  // §3.4.3); otherwise, lua_tointegerx returns 0. If isnum is
  // not NULL, its referent is assigned a boolean value that in-
  // dicates whether the operation succeeded.
  // [-0, +0, –]
  integer i = lua_tointegerx( L, idx, &is_num );
  if( is_num != 0 ) return i;
  return nothing;
}

base::maybe<int> c_api::get( int idx, int* ) noexcept {
  validate_index( idx );
  auto i = get<integer>( idx );
  if( i )
    return (int)*i;
  else
    return nothing;
}

maybe<double> c_api::get( int idx, double* ) noexcept {
  validate_index( idx );
  int is_num = 0;
  // lua_tonumberx: [-0, +0, –]
  //
  // Converts the Lua value at the given index to the C type
  // lua_Number (see lua_Number). The Lua value must be a number
  // or a string convertible to a number (see §3.4.3); otherwise,
  // lua_tonumberx returns 0.
  //
  // If isnum is not NULL, its referent is assigned a boolean
  // value that indicates whether the operation succeeded.
  double num = lua_tonumberx( L, idx, &is_num );
  if( is_num != 0 ) return num;
  return nothing;
}

base::maybe<floating> c_api::get( int idx, floating* ) noexcept {
  return get<double>( idx );
}

maybe<string> c_api::get( int idx, string* ) noexcept {
  validate_index( idx );
  size_t len = 0;
  // The function we will use below, lua_tolstring, will actually
  // change the value on the stack to a string if it is not al-
  // ready a string (i.e., if it is a number). We don't really
  // want this, so we're going to push a copy of the value on the
  // stack (this should be cheap, I don't believe it should make
  // a copy of the string...but not sure).
  //
  // The exception is if the thread is in an error state in which
  // case it is not safe to push things onto the stack (it seems
  // to yield a Lua stack overflow error). So in that case we
  // will just call lua_tolstring in place.
  if( status() == thread_status::err ) {
    char const* p = lua_tolstring( L, idx, &len );
    if( p == nullptr ) return nothing;
    // See below for why we use this constructor.
    return string( p, len );
  }
  // Thread is not in an error state, so we can push things.
  pushvalue( idx );
  SCOPE_EXIT( pop() );
  // lua_tolstring:  [-0, +0, m]
  //
  // Converts the Lua value at the given index to a C string. If
  // len is not NULL, it sets *len with the string length.The Lua
  // value must be a string or a number; otherwise, the function
  // returns NULL.If the value is a number, then lua_tolstring
  // also changes the actual value in the stack to a string, but
  // this won't be a problem for us because we are operating on a
  // temporary value at the top of the stack.
  //
  // lua_tolstring returns a pointer to a string inside the Lua
  // state. This string always has a zero( '\0' ) after its last
  // character( as in C ), but can contain other zeros in its
  // body.
  //
  // Because Lua has garbage collection, there is no guarantee
  // that the pointer returned by lua_tolstring will be valid
  // after the corresponding Lua value is removed from the stack.
  char const* p = lua_tolstring( L, -1, &len );
  if( p == nullptr ) return nothing;
  // Use the (pointer, size) constructor because we need to
  // specify the length, 1) so that std::string can pre-allocate,
  // and 2) because there may be zeroes inside the string before
  // the final null terminater.
  return string( p, len );
}

base::maybe<void*> c_api::get( int idx, void** ) noexcept {
  validate_index( idx );
  void* p = lua_touserdata( L, idx );
  if( !p ) return nothing;
  return p;
}

base::maybe<lightuserdata> c_api::get(
    int idx, lightuserdata* ) noexcept {
  return get<void*>( idx );
}

base::maybe<char const*> c_api::get( int idx,
                                     char const** ) noexcept {
  validate_index( idx );
  // We are susceptible to this because of the char const*.
  CHECK( !lua_isstring( L, idx ) );
  CHECK( lua_islightuserdata( L, idx ),
         "index {} is not a light userdata.", idx );
  auto* p = static_cast<const char*>( lua_touserdata( L, idx ) );
  // Not sure if this check is needed.
  CHECK( p );
  return p;
}

type c_api::lua_type_to_enum( int type ) noexcept {
  CHECK( type != LUA_TNONE, "type ({}) not valid.", type );
  CHECK( type >= 0 );
  CHECK( type < kNumLuaTypes,
         "a new lua type may have been added." );
  return static_cast<lua::type>( type );
}

type c_api::geti( int idx, integer i ) noexcept {
  validate_index( idx );
  return lua_type_to_enum( lua_geti( L, idx, i ) );
}

int c_api::ref( int idx ) noexcept {
  validate_index( idx );
  return luaL_ref( L, idx );
}

int c_api::ref_registry() noexcept {
  return ref( LUA_REGISTRYINDEX );
}

type c_api::registry_get( int id ) noexcept {
  return rawgeti( LUA_REGISTRYINDEX, id );
}

void c_api::unref( int t, int ref ) noexcept {
  luaL_unref( L, t, ref );
}

void c_api::unref_registry( int ref ) noexcept {
  unref( LUA_REGISTRYINDEX, ref );
}

void c_api::len( int idx ) {
  validate_index( idx );
  lua_len( L, idx );
}

int c_api::len_pop( int idx ) {
  validate_index( idx );
  return luaL_len( L, idx );
}

// The Lua types are defined in lua.h, as of Lua 5.3:
//
//   LUA_TNIL		        0
//   LUA_TBOOLEAN		    1
//   LUA_TLIGHTUSERDATA	2
//   LUA_TNUMBER		    3
//   LUA_TSTRING		    4
//   LUA_TTABLE		      5
//   LUA_TFUNCTION		  6
//   LUA_TUSERDATA		  7
//   LUA_TTHREAD		    8
//
type c_api::type_of( int idx ) noexcept {
  validate_index( idx );
  int res = lua_type( L, idx );
  CHECK( res != LUA_TNONE, "index ({}) not valid.", idx );
  return lua_type_to_enum( res );
}

char const* c_api::type_name( type type ) noexcept {
  return lua_typename( L, static_cast<int>( type ) );
}

/****************************************************************
** Error checking helpers.
*****************************************************************/
void c_api::enforce_stack_size_ge( int s ) noexcept {
  CHECK( s >= 0 );
  if( stack_size() >= s ) return;
  FATAL(
      "stack size expected to have size at least {} but "
      "instead found {}.",
      s, stack_size() );
}

lua_valid c_api::enforce_type_of( int idx, type type ) noexcept {
  validate_index( idx );
  if( type_of( idx ) == type ) return base::valid;
  return "type of element at index " + to_string( idx ) +
         " expected to be " + string( type_name( type ) ) +
         ", but instead is " +
         string( type_name( type_of( idx ) ) );
}

lua_error_t c_api::pop_and_return_error() noexcept {
  enforce_stack_size_ge( 1 );
  CHECK( type_of( -1 ) == type::string );
  lua_error_t res( lua_tostring( L, -1 ) );
  pop();
  return res;
}

void c_api::validate_index( int idx ) noexcept {
  // This should allow all valid pseudo indices to pass. Cur-
  // rently, this means the registry index and up value indices.
  if( idx <= LUA_REGISTRYINDEX ) return;
  enforce_stack_size_ge( abs( idx ) );
}

void* c_api::newuserdata( int size ) noexcept {
  // Pushes userdata onto stack, returns pointer to buffer.
  return lua_newuserdata( L, size );
}

bool c_api::udata_newmetatable( char const* tname ) noexcept {
  return luaL_newmetatable( L, tname ) != 0;
}

type c_api::udata_getmetatable( char const* tname ) noexcept {
  return lua_type_to_enum( luaL_getmetatable( L, tname ) );
}

void c_api::udata_setmetatable( char const* tname ) noexcept {
  luaL_setmetatable( L, tname );
}

void* c_api::checkudata( int arg, char const* tname ) noexcept {
  return luaL_checkudata( L, arg, tname );
}

void* c_api::testudata( int arg, char const* tname ) noexcept {
  return luaL_testudata( L, arg, tname );
}

void c_api::setmetatable( int idx ) noexcept {
  validate_index( idx );
  enforce_stack_size_ge( 2 );
  lua_setmetatable( L, idx );
}

bool c_api::getmetatable( int idx ) noexcept {
  validate_index( idx );
  return lua_getmetatable( L, idx ) != 0;
}

bool c_api::getupvalue( int funcindex, int n ) noexcept {
  validate_index( funcindex );
  char const* name = lua_getupvalue( L, funcindex, n );
  // `name` will be "" for C upvalues, NULL for nonexistent
  // upvalues.
  return ( name != nullptr );
}

void c_api::error() noexcept( false ) {
  lua_error( L );
  // We won't get here, it is just to suppress the warning
  // telling us that this function should not return, since we
  // cannot mark lua_error as [[noreturn]].
  throw 0;
}

void c_api::error( std::string const& msg ) noexcept( false ) {
  luaL_error( L, msg.c_str() );
  // We won't get here, it is just to suppress the warning
  // telling us that this function should not return, since we
  // cannot mark lua_error as [[noreturn]].
  throw 0;
}

int c_api::noref() noexcept { return LUA_NOREF; }

void c_api::gc_collect() {
  // The last parameter are unused by the LUA_GCCOLLECT mode.
  lua_gc( L, LUA_GCCOLLECT, 0 );
}

cthread c_api::newthread() noexcept {
  return lua_newthread( L );
}

void c_api::pushvalue( int idx ) noexcept {
  validate_index( idx );
  lua_pushvalue( L, idx );
}

bool c_api::compare_eq( int idx1, int idx2 ) {
  validate_index( idx1 );
  validate_index( idx2 );
  constexpr int op = LUA_OPEQ;
  return ( lua_compare( L, idx1, idx2, op ) == 1 );
}

bool c_api::compare_lt( int idx1, int idx2 ) {
  validate_index( idx1 );
  validate_index( idx2 );
  constexpr int op = LUA_OPLT;
  return ( lua_compare( L, idx1, idx2, op ) == 1 );
}

bool c_api::compare_le( int idx1, int idx2 ) {
  validate_index( idx1 );
  validate_index( idx2 );
  constexpr int op = LUA_OPLE;
  return ( lua_compare( L, idx1, idx2, op ) == 1 );
}

void c_api::concat( int n ) noexcept {
  enforce_stack_size_ge( n );
  lua_concat( L, n );
}

char const* c_api::tostring( int idx, size_t* len ) noexcept {
  validate_index( idx );
  return luaL_tolstring( L, idx, len );
}

bool c_api::isinteger( int idx ) noexcept {
  validate_index( idx );
  return ( lua_isinteger( L, idx ) == 1 );
}

bool c_api::pushthread() noexcept {
  return ( lua_pushthread( L ) == 1 );
}

string c_api::pop_tostring() noexcept {
  enforce_stack_size_ge( 1 );
  size_t      len = 0;
  char const* p   = tostring( -1, &len );
  string_view sv( p, len );
  string      res = string( sv );
  pop( 2 );
  return res;
}

int c_api::rawlen( int idx ) noexcept {
  validate_index( idx );
  return int( lua_rawlen( L, idx ) );
}

int c_api::checkinteger( int arg ) {
  return luaL_checkinteger( L, arg );
}

bool c_api::next( int idx ) noexcept {
  validate_index( idx );
  return lua_next( L, idx ) == 1;
}

void c_api::print_stack( string_view label ) noexcept {
  fmt::print( "[{}] Lua Stack:\n", label );
  for( int i = stack_size(); i >= 1; --i ) {
    string s = tostring( i, nullptr );
    pop();
    fmt::print( "  {: 2}. {}: {}\n", i - stack_size() - 1,
                type_of( i ), s );
  }
}

void c_api::settop( int top ) { lua_settop( L, top ); }

lua_valid c_api::loadfile( const char* filename ) {
  int res = luaL_loadfile( L, filename );
  switch( res ) {
    case LUA_OK: return base::valid;
    case LUA_ERRSYNTAX:
      return lua_invalid(
          "syntax error during precompilation." );
    case LUA_ERRMEM:
      return lua_invalid(
          "memory allocation (out-of-memory) error." );
    case LUA_ERRFILE:
      return lua_invalid(
          fmt::format( "error opening file {}", filename ) );
    default:
      return lua_invalid(
          fmt::format( "unknown error: {}", res ) );
  }
}

cthread c_api::tothread( int idx ) {
  validate_index( idx );
  return lua_tothread( L, idx );
}

thread_status c_api::status() noexcept {
  return to_thread_status( lua_status( L ) );
}

lua_valid c_api::thread_ok() noexcept {
  thread_status stat = status();
  lua_valid     res  = base::valid;
  switch( stat ) {
    case thread_status::err: {
      enforce_stack_size_ge( 1 );
      CHECK( type_of( -1 ) == type::string );
      res = lua_tostring( L, -1 );
      // NOTE: we do not pop the error off of the stack so that
      // we can retrieve it a second time if needed.
    }
    default: break;
  }
  return res;
}

lua_valid c_api::resetthread() noexcept {
  int res = lua_resetthread( L );
  if( res == LUA_OK ) return base::valid;
  // We have an error, either in closing the to-be-closed vari-
  // ables, or the original error that caused the coroutine to
  // stop, which is on the top of the stack.
  enforce_stack_size_ge( 1 );
  CHECK( type_of( -1 ) == type::string );
  return lua_tostring( L, -1 );
}

lua_expect<resume_result> c_api::resume_or_leak(
    cthread L_toresume, int nargs ) noexcept {
  // L and L_toresume may or may not be the same here.
  c_api C_toresume( L_toresume );
  C_toresume.enforce_stack_size_ge( nargs );
  lua_State*    L_from   = L;
  int           nresults = 0;
  thread_status status   = to_thread_status(
        lua_resume( L_toresume, L_from, nargs, &nresults ) );
  HAS_VALUE_OR_RET( C_toresume.thread_ok() );
  CHECK( status != thread_status::err );
  return resume_result{ .status = ( status == thread_status::ok )
                                      ? resume_status::ok
                                      : resume_status::yield,
                        .nresults = nresults };
}

lua_expect<resume_result> c_api::resume_or_reset(
    cthread L_toresume, int nargs ) noexcept {
  // L and L_toresume may or may not be the same here.
  lua_expect<resume_result> res =
      resume_or_leak( L_toresume, nargs );
  if( !res ) {
    c_api     C_toresume( L_toresume );
    lua_valid close_result = C_toresume.resetthread();
    // close_result will always just contain the original error
    // even if there is an error while closing, so there is no
    // point in looking at it.
    (void)close_result;
    // resetthread is supposed to leave the error object on the
    // top of the stack, and it appears that it is the ONLY thing
    // left on the stack.
    DCHECK( C_toresume.stack_size() == 1 );
  }
  return res;
}

// The implementation of this function was taken from the Lua
// source code.
coroutine_status c_api::coro_status() noexcept {
  switch( lua_status( L ) ) {
    case LUA_YIELD: return coroutine_status::suspended;
    case LUA_OK: {
      lua_Debug ar;
      if( lua_getstack( L, 0, &ar ) )    // does it have frames?
        return coroutine_status::normal; // it is running
      else if( lua_gettop( L ) == 0 )
        return coroutine_status::dead;
      else
        return coroutine_status::suspended; // initial state
    }
    default: // some error occurred
      return coroutine_status::dead;
  }
}

} // namespace lua
