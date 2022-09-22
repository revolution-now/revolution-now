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

namespace lua {
namespace {

static_assert( is_same_v<LuaKContext, lua_KContext> );

using ::base::maybe;
using ::base::nothing;

// Not sure if these are mandatory, but if they fail they will at
// least alert us that something has changed.
static_assert( sizeof( integer ) == sizeof( long long ) );
static_assert( sizeof( long long ) >= 8 );

int msghandler( lua_State* L ) {
  const char* msg = lua_tostring( L, 1 );
  // is error object not a string?
  if( msg == nullptr ) {
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
  return base::invalid<lua_error_t>( std::move( err ) );
}

/****************************************************************
** c_api
*****************************************************************/
void c_api::openlibs() noexcept { luaL_openlibs( L_ ); }

lua_valid c_api::dofile( char const* file ) noexcept {
  // luaL_dofile: [-0, +?, e]
  if( luaL_dofile( L_, file ) ) return pop_and_return_error();
  return base::valid;
}

lua_valid c_api::dofile( std::string const& file ) noexcept {
  return dofile( file.c_str() );
}

int c_api::gettop() noexcept { return lua_gettop( L_ ); }
int c_api::stack_size() noexcept { return gettop(); }

void c_api::setglobal( char const* key ) noexcept {
  enforce_stack_size_ge( 1 );
  // [-1,+0,e]
  lua_setglobal( L_, key );
}

void c_api::setglobal( string const& key ) noexcept {
  return setglobal( key.c_str() );
}

type c_api::getglobal( char const* name ) noexcept {
  return lua_type_to_enum( lua_getglobal( L_, name ) );
}

type c_api::getglobal( string const& name ) noexcept {
  return getglobal( name.c_str() );
}

lua_valid c_api::loadstring( char const* script ) noexcept {
  lua_valid res = base::valid;
  // [-0, +1, –]
  if( luaL_loadstring( L_, script ) == LUA_OK )
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
  SCOPE_EXIT( lua_remove( L_, msghandler_idx ) );

  // No matter what happens, lua_pcall will remove the function
  // and arguments from the stack.
  int status = lua_pcall( L_, nargs, nresults, msghandler_idx );

  if( status == LUA_OK ) return base::valid;
  return pop_and_return_error();
}

void c_api::pcallk( int nargs, int nresults, LuaKContext ctx,
                    LuaKFunction k ) {
  // Lua defines lua_pcall as lua_pcallk with no continuation
  // function, so if there is no continuation function then you
  // should instead just call pcall. Requiring this allows us to
  // put some additional restrictions on the return value of
  // lua_pcallk should we actually call it.
  CHECK( k != nullptr, "continuation must be non-null." );
  // We have a continuation function.
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
      lua_pcallk( L_, nargs, nresults, msghandler_idx, ctx, k );
  // If we get here then the function finished successfully
  // without yielding.
  CHECK( status == LUA_OK );
}

void c_api::call( int nargs, int nresults ) {
  CHECK( nargs >= 0 );
  CHECK( nresults >= 0 || nresults == LUA_MULTRET );
  // Function object plus args should be on the stack at least.
  enforce_stack_size_ge( nargs + 1 );
  // No matter what happens, lua_call will remove the function
  // and arguments from the stack.
  lua_call( L_, nargs, nresults );
  if( nresults != LUA_MULTRET )
    enforce_stack_size_ge( nresults );
}

void c_api::pushglobaltable() noexcept {
  lua_pushglobaltable( L_ );
}

void c_api::push( nil_t ) noexcept { lua_pushnil( L_ ); }

void c_api::push( LuaCFunction* f, int upvalues ) noexcept {
  enforce_stack_size_ge( upvalues );
  lua_pushcclosure( L_, f, upvalues );
}

void c_api::push( lightuserdata p ) noexcept {
  lua_pushlightuserdata( L_, p.get() );
}

void c_api::push( boolean b ) noexcept {
  int b_int = b; // just to make it explicit.
  lua_pushboolean( L_, b_int );
}

void c_api::push( integer n ) noexcept {
  lua_pushinteger( L_, n );
}

void c_api::push( floating d ) noexcept {
  lua_pushnumber( L_, d );
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
  lua_pushlstring( L_, sv.data(), sv.size() );
}

void c_api::pop( int n ) noexcept {
  CHECK_GE( stack_size(), n );
  lua_pop( L_, n );
}

void c_api::rotate( int idx, int n ) noexcept {
  validate_index( idx );
  // [-0,+0,–]
  lua_rotate( L_, idx, n );
}

void c_api::insert( int idx ) noexcept {
  validate_index( idx );
  lua_insert( L_, idx );
}

void c_api::swap_top() noexcept {
  enforce_stack_size_ge( 2 );
  rotate( -2, 1 );
}

void c_api::newtable() noexcept { lua_newtable( L_ ); }

// (table_idx)[-2] = -1
void c_api::settable( int table_idx ) {
  validate_index( table_idx );
  lua_settable( L_, table_idx );
}

// (table_idx)[k] = -1
void c_api::setfield( int table_idx, char const* k ) {
  validate_index( table_idx );
  lua_setfield( L_, table_idx, k );
}

type c_api::getfield( int table_idx, char const* k ) {
  validate_index( table_idx );
  return lua_type_to_enum( lua_getfield( L_, table_idx, k ) );
}

type c_api::gettable( int idx ) {
  validate_index( idx );
  return lua_type_to_enum( lua_gettable( L_, idx ) );
}

type c_api::rawget( int idx ) noexcept {
  validate_index( idx );
  return lua_type_to_enum( lua_rawget( L_, idx ) );
}

type c_api::rawgeti( int idx, int n ) noexcept {
  validate_index( idx );
  return lua_type_to_enum( lua_rawgeti( L_, idx, n ) );
}

void c_api::rawseti( int idx, int n ) noexcept {
  validate_index( idx );
  lua_rawseti( L_, idx, n );
}

bool c_api::get( int idx, bool* ) noexcept {
  validate_index( idx );
  // Converts the Lua value at the given index to a C boolean
  // value (0 or 1). Like all tests in Lua, lua_toboolean returns
  // true for any Lua value different from false and nil; other-
  // wise it returns false. (If you want to accept only actual
  // boolean values, use lua_isboolean to test the value's type.)
  // [-0, +0, –]
  int i = lua_toboolean( L_, idx );
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
  integer i = lua_tointegerx( L_, idx, &is_num );
  if( is_num != 0 ) return i;
  return nothing;
}

base::maybe<int> c_api::get( int idx, int* ) noexcept {
  validate_index( idx );
  auto i = get<integer>( idx );
  if( i )
    return static_cast<int>( *i );
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
  double num = lua_tonumberx( L_, idx, &is_num );
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
    char const* p = lua_tolstring( L_, idx, &len );
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
  char const* p = lua_tolstring( L_, -1, &len );
  if( p == nullptr ) return nothing;
  // Use the (pointer, size) constructor because we need to
  // specify the length, 1) so that std::string can pre-allocate,
  // and 2) because there may be zeroes inside the string before
  // the final null terminater.
  return string( p, len );
}

base::maybe<void*> c_api::get( int idx, void** ) noexcept {
  validate_index( idx );
  void* p = lua_touserdata( L_, idx );
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
  CHECK( !lua_isstring( L_, idx ) );
  CHECK( lua_islightuserdata( L_, idx ),
         "index {} is not a light userdata.", idx );
  auto* p =
      static_cast<const char*>( lua_touserdata( L_, idx ) );
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
  return lua_type_to_enum( lua_geti( L_, idx, i ) );
}

int c_api::ref( int idx ) noexcept {
  validate_index( idx );
  return luaL_ref( L_, idx );
}

int c_api::ref_registry() noexcept {
  return ref( LUA_REGISTRYINDEX );
}

type c_api::registry_get( int id ) noexcept {
  return rawgeti( LUA_REGISTRYINDEX, id );
}

void c_api::unref( int t, int ref ) noexcept {
  luaL_unref( L_, t, ref );
}

void c_api::unref_registry( int ref ) noexcept {
  unref( LUA_REGISTRYINDEX, ref );
}

void c_api::len( int idx ) {
  validate_index( idx );
  lua_len( L_, idx );
}

int c_api::len_pop( int idx ) {
  validate_index( idx );
  return static_cast<int>( luaL_len( L_, idx ) );
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
  int res = lua_type( L_, idx );
  CHECK( res != LUA_TNONE, "index ({}) not valid.", idx );
  return lua_type_to_enum( res );
}

char const* c_api::type_name( type type ) noexcept {
  return lua_typename( L_, static_cast<int>( type ) );
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
  lua_error_t res( lua_tostring( L_, -1 ) );
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
  return lua_newuserdata( L_, static_cast<size_t>( size ) );
}

bool c_api::udata_newmetatable( char const* tname ) noexcept {
  return luaL_newmetatable( L_, tname ) != 0;
}

type c_api::udata_getmetatable( char const* tname ) noexcept {
  return lua_type_to_enum( luaL_getmetatable( L_, tname ) );
}

void c_api::udata_setmetatable( char const* tname ) noexcept {
  luaL_setmetatable( L_, tname );
}

void* c_api::checkudata( int arg, char const* tname ) noexcept {
  return luaL_checkudata( L_, arg, tname );
}

void* c_api::testudata( int arg, char const* tname ) noexcept {
  return luaL_testudata( L_, arg, tname );
}

void c_api::setmetatable( int idx ) noexcept {
  validate_index( idx );
  enforce_stack_size_ge( 2 );
  lua_setmetatable( L_, idx );
}

bool c_api::getmetatable( int idx ) noexcept {
  validate_index( idx );
  return lua_getmetatable( L_, idx ) != 0;
}

bool c_api::getupvalue( int funcindex, int n ) noexcept {
  validate_index( funcindex );
  char const* name = lua_getupvalue( L_, funcindex, n );
  // `name` will be "" for C upvalues, NULL for nonexistent
  // upvalues.
  return ( name != nullptr );
}

void c_api::error() noexcept( false ) {
  lua_error( L_ );
  // We won't get here, it is just to suppress the warning
  // telling us that this function should not return, since we
  // cannot mark lua_error as [[noreturn]].
  throw 0;
}

void c_api::error( std::string const& msg ) noexcept( false ) {
  luaL_error( L_, msg.c_str() );
  // We won't get here, it is just to suppress the warning
  // telling us that this function should not return, since we
  // cannot mark lua_error as [[noreturn]].
  throw 0;
}

int c_api::noref() noexcept { return LUA_NOREF; }

void c_api::gc_collect() {
  // The last parameter are unused by the LUA_GCCOLLECT mode.
  lua_gc( L_, LUA_GCCOLLECT, 0 );
}

cthread c_api::newthread() noexcept {
  return lua_newthread( L_ );
}

void c_api::pushvalue( int idx ) noexcept {
  validate_index( idx );
  lua_pushvalue( L_, idx );
}

bool c_api::compare_eq( int idx1, int idx2 ) {
  validate_index( idx1 );
  validate_index( idx2 );
  constexpr int op = LUA_OPEQ;
  return ( lua_compare( L_, idx1, idx2, op ) == 1 );
}

bool c_api::compare_lt( int idx1, int idx2 ) {
  validate_index( idx1 );
  validate_index( idx2 );
  constexpr int op = LUA_OPLT;
  return ( lua_compare( L_, idx1, idx2, op ) == 1 );
}

bool c_api::compare_le( int idx1, int idx2 ) {
  validate_index( idx1 );
  validate_index( idx2 );
  constexpr int op = LUA_OPLE;
  return ( lua_compare( L_, idx1, idx2, op ) == 1 );
}

void c_api::concat( int n ) noexcept {
  enforce_stack_size_ge( n );
  lua_concat( L_, n );
}

char const* c_api::tostring( int idx, size_t* len ) noexcept {
  validate_index( idx );
  return luaL_tolstring( L_, idx, len );
}

bool c_api::isinteger( int idx ) noexcept {
  validate_index( idx );
  return ( lua_isinteger( L_, idx ) == 1 );
}

bool c_api::pushthread() noexcept {
  return ( lua_pushthread( L_ ) == 1 );
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
  return int( lua_rawlen( L_, idx ) );
}

int c_api::checkinteger( int arg ) {
  return static_cast<int>( luaL_checkinteger( L_, arg ) );
}

bool c_api::next( int idx ) noexcept {
  validate_index( idx );
  return lua_next( L_, idx ) == 1;
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

void c_api::settop( int top ) { lua_settop( L_, top ); }

lua_valid c_api::loadfile( const char* filename ) {
  int res = luaL_loadfile( L_, filename );
  switch( res ) {
    case LUA_OK: return base::valid;
    case LUA_ERRSYNTAX: {
      string err = pop_tostring();
      return lua_invalid( fmt::format(
          "syntax error during precompilation: {}", err ) );
    }
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
  return lua_tothread( L_, idx );
}

thread_status c_api::status() noexcept {
  return to_thread_status( lua_status( L_ ) );
}

lua_valid c_api::thread_ok() noexcept {
  thread_status stat = status();
  lua_valid     res  = base::valid;
  switch( stat ) {
    case thread_status::err: {
      enforce_stack_size_ge( 1 );
      CHECK( type_of( -1 ) == type::string );
      res = lua_tostring( L_, -1 );
      // NOTE: we do not pop the error off of the stack so that
      // we can retrieve it a second time if needed.
      break;
    }
    case thread_status::ok:
    case thread_status::yield: break;
  }
  return res;
}

lua_valid c_api::resetthread() noexcept {
  // As of Lua 5.4.4, this will reset the error state of the
  // thread as well, though it will still return the error from
  // this function (presumably just the first time).
  int res = lua_resetthread( L_ );
  if( res == LUA_OK ) return base::valid;
  // We have an error, either in closing the to-be-closed vari-
  // ables, or the original error that caused the coroutine to
  // stop, which is on the top of the stack.
  enforce_stack_size_ge( 1 );
  CHECK( type_of( -1 ) == type::string );
  return lua_tostring( L_, -1 );
}

lua_expect<resume_result> c_api::resume_or_leak(
    cthread L_toresume, int nargs ) noexcept {
  // L_ and L_toresume may or may not be the same here.
  c_api C_toresume( L_toresume );
  C_toresume.enforce_stack_size_ge( nargs );
  lua_State*    L_from   = L_;
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
  // L_ and L_toresume may or may not be the same here.
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
  switch( lua_status( L_ ) ) {
    case LUA_YIELD: return coroutine_status::suspended;
    case LUA_OK: {
      lua_Debug ar;
      if( lua_getstack( L_, 0, &ar ) )   // does it have frames?
        return coroutine_status::normal; // it is running
      else if( lua_gettop( L_ ) == 0 )
        return coroutine_status::dead;
      else
        return coroutine_status::suspended; // initial state
    }
    default: // some error occurred
      return coroutine_status::dead;
  }
}

} // namespace lua
