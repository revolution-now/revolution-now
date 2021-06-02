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

// Lua
#include "lauxlib.h"
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

namespace luapp {
namespace {

using ::base::maybe;
using ::base::nothing;

// Not sure if these are mandatory, but if they fail they will at
// least alert us that something has changed.
static_assert( sizeof( ::lua_Integer ) == sizeof( long long ) );
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
c_api::c_api( ::lua_State* state ) {
  L = state;
  CHECK( L != nullptr );
}

c_api::c_api() : c_api( ::luaL_newstate() ) {}

c_api::~c_api() noexcept {
  // Not sure if this check is a good idea...
  DCHECK( stack_size() == 0 );
  ::lua_close( L );
}

/****************************************************************
** Lua C Function Wrappers.
*****************************************************************/
void c_api::openlibs() noexcept { luaL_openlibs( L ); }

lua_valid c_api::dofile( char const* file ) noexcept {
  // luaL_dofile: [-0, +?, e]
  if( luaL_dofile( L, file ) ) return pop_and_return_error();
  return base::valid;
}

lua_valid c_api::dofile( string const& file ) noexcept {
  return dofile( file.c_str() );
}

int c_api::gettop() const noexcept { return lua_gettop( L ); }
int c_api::stack_size() const noexcept { return gettop(); }

lua_valid c_api::setglobal( char const* key ) noexcept {
  DECLARE_NUM_CONSUMED_VALUES( 1 );
  // [-1,+0,e]
  return pinvoke( ninputs, ::lua_setglobal, key );
}

lua_valid c_api::setglobal( string const& key ) noexcept {
  return setglobal( key.c_str() );
}

lua_expect<e_lua_type> c_api::getglobal(
    char const* name ) noexcept {
  DECLARE_NUM_CONSUMED_VALUES( 0 );
  UNWRAP_RETURN( type,
                 pinvoke( ninputs, ::lua_getglobal, name ) );
  // Should get here even when the name does not exist, in which
  // case it will have returned nil and pushed it onto the
  // stack..
  enforce_stack_size_ge( 1 );
  CHECK_EQ( lua_type_to_enum( type ), type_of( -1 ) );
  return lua_type_to_enum( type );
}

lua_expect<e_lua_type> c_api::getglobal(
    string const& name ) noexcept {
  return getglobal( name.c_str() );
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

lua_valid c_api::loadstring( string const& script ) noexcept {
  return loadstring( script.c_str() );
}

lua_valid c_api::dostring( char const* script ) noexcept {
  HAS_VALUE_OR_RET( loadstring( script ) );
  enforce_stack_size_ge( 1 );
  return pcall( /*nargs=*/0, /*nresults=*/LUA_MULTRET );
}

lua_valid c_api::dostring( string const& script ) noexcept {
  return dostring( script.c_str() );
}

// See header file for explanation of this function.
// clang-format off
template<typename R, typename... Params, typename... Args>
requires( sizeof...( Params ) == sizeof...( Args ) &&
          std::is_invocable_v<LuaApiFunc<R, Params...>*,
                              ::lua_State*,
                              Args...> )
auto c_api::pinvoke( int ninputs,
                     LuaApiFunc<R, Params...>* func,
                     Args... args )
    -> std::conditional_t<std::is_same_v<R, void>,
                          lua_valid,
                          lua_expect<R>> {
  // clang-format on
  constexpr bool has_result = !is_same_v<R, void>;

  constexpr int kCApiPtrIdx = 1;
  constexpr int kNInputsIdx = 2;
  constexpr int kFuncIdx    = 3;
  // equal to last one.
  constexpr int kNumRunnerArgs = 3;

  auto runner = []( ::lua_State* L ) -> int {
    // 1. Get c_api pointer from stack.
    CHECK( lua_islightuserdata( L, kCApiPtrIdx ) );
    c_api* api = (c_api*)::lua_touserdata( L, kCApiPtrIdx );
    CHECK( api );

    // 2. Get number of arguments that were pushed onto the stack
    //    for this Lua API function to consume.
    CHECK( lua_isinteger( L, kNInputsIdx ) );
    UNWRAP_CHECK( ninputs, api->get<int>( kNInputsIdx ) );
    CHECK_GE( ninputs, 0 );

    // 3. Get pointer to Lua API function that we will call.
    //    e.g., this will be a pointer to ::lua_gettable.
    CHECK( lua_islightuserdata( L, kFuncIdx ) );
    UNWRAP_CHECK( void_func, api->get<void*>( kFuncIdx ) );
    auto* func =
        reinterpret_cast<LuaApiFunc<R, Params...>*>( void_func );
    CHECK( func );

    // 4. Check stack size.
    CHECK_GE( api->stack_size(), int( kNumRunnerArgs + ninputs +
                                      sizeof...( Args ) ) );

    // 5. Pop parameters into tuple.
    int  idx    = kNumRunnerArgs + 1 + ninputs;
    auto getter = [&]<typename Arg>( Arg* ) {
      UNWRAP_CHECK( res, api->get<Arg>( idx ) );
      ++idx;
      return res;
    };
    auto tuple_args = tuple{ L, getter( (Args*)nullptr )... };

    // 6. Now pop everything so that it doesn't get in the way of
    //    our stack size calculation.
    api->pop( sizeof...( Args ) );
    CHECK( api->stack_size() == kNumRunnerArgs + ninputs );
    api->rotate( -ninputs - kNumRunnerArgs, ninputs );
    api->pop( kNumRunnerArgs );

    // At this point on the stack we have only this C function
    // object (just below the zero point which we cannot access),
    // then all of the values that will be popped by the Lua API
    // function we are about to call.
    CHECK( api->stack_size() == ninputs );

    // 7. Run it
    if constexpr( has_result ) {
      R res = apply( func, tuple_args );
      api->push( res );
    } else {
      apply( func, tuple_args );
    }

    return api->stack_size();
  };

  // For sanity checking.
  e_lua_type tip_type =
      ninputs > 0 ? type_of( -1 ) : e_lua_type::nil;

  // 1. Push func onto the stack.
  push( (LuaCFunction*)+runner );

  // 2. Push `this`.
  push( static_cast<void*>( this ) );

  // 3. Push initial stack size so that our helper function above
  //    can compute how many results are being returned.
  push( ninputs );

  // 4. Push the actual Lua API function we want to run.
  push( reinterpret_cast<void*>( func ) );

  // 5. Put the initial args behind those that are to be read
  //    by the Lua API function we will call.
  rotate( -ninputs - kNumRunnerArgs - 1, kNumRunnerArgs + 1 );

  // For sanity checking.
  if( ninputs > 0 ) { CHECK( type_of( -1 ) == tip_type ); }

  // 5. Push arguments.
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

lua_valid c_api::pcall( int nargs, int nresults ) noexcept {
  CHECK( nargs >= 0 );
  CHECK( nresults >= 0 || nresults == LUA_MULTRET );
  // Function object plus args should be on the stack at least.
  enforce_stack_size_ge( nargs + 1 );
  lua_valid res = base::valid;
  // No matter what happens, lua_pcall will remove the function
  // and arguments from the stack.
  int err = ::lua_pcall( L, nargs, nresults, /*msgh=*/0 );
  if( err == LUA_OK ) {
    if( nresults != LUA_MULTRET )
      enforce_stack_size_ge( nresults );
  } else {
    // lua_pcall will have pushed a single value onto the stack,
    // which will be the error object.
    return pop_and_return_error();
  }
  return res;
}

void c_api::push( nil_t ) noexcept { ::lua_pushnil( L ); }

void c_api::push( LuaCFunction* f ) noexcept {
  lua_pushcfunction( L, f );
}

void c_api::push( void* p ) noexcept {
  lua_pushlightuserdata( L, p );
}

void c_api::push( base::safe::boolean b ) noexcept {
  int b_int = b; // just to make it explicit.
  ::lua_pushboolean( L, b_int );
}

void c_api::push(
    base::safe::integral<lua_Integer> n ) noexcept {
  ::lua_pushinteger( L, n );
}

void c_api::push( base::safe::floating<lua_Number> d ) noexcept {
  ::lua_pushnumber( L, d );
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
  ::lua_pushlstring( L, sv.data(), sv.size() );
}

void c_api::pop( int n ) noexcept {
  CHECK_GE( stack_size(), n );
  lua_pop( L, n );
}

void c_api::rotate( int idx, int n ) noexcept {
  // [-0,+0,–]
  ::lua_rotate( L, idx, n );
}

bool c_api::get( int idx, bool* ) const noexcept {
  validate_index( idx );
  // Converts the Lua value at the given index to a C boolean
  // value (0 or 1). Like all tests in Lua, lua_toboolean returns
  // true for any Lua value different from false and nil; other-
  // wise it returns false. (If you want to accept only actual
  // boolean values, use lua_isboolean to test the value's type.)
  // [-0, +0, –]
  int i = ::lua_toboolean( L, idx );
  CHECK( i == 0 || i == 1 );
  return bool( i );
}

maybe<lua_Integer> c_api::get( int idx,
                               lua_Integer* ) const noexcept {
  validate_index( idx );
  int is_num = 0;
  // Converts the Lua value at the given index to the signed in-
  // tegral type lua_Integer. The Lua value must be an integer,
  // or a number or string convertible to an integer (see
  // §3.4.3); otherwise, lua_tointegerx returns 0. If isnum is
  // not NULL, its referent is assigned a boolean value that in-
  // dicates whether the operation succeeded.
  // [-0, +0, –]
  lua_Integer i = ::lua_tointegerx( L, idx, &is_num );
  if( is_num != 0 ) return i;
  return nothing;
}

base::maybe<int> c_api::get( int idx, int* ) const noexcept {
  validate_index( idx );
  auto i = get<lua_Integer>( idx );
  if( i )
    return (int)*i;
  else
    return nothing;
}

maybe<lua_Number> c_api::get( int idx,
                              lua_Number* ) const noexcept {
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
  lua_Number num = ::lua_tonumberx( L, idx, &is_num );
  if( is_num != 0 ) return num;
  return nothing;
}

maybe<string> c_api::get( int idx, string* ) const noexcept {
  validate_index( idx );
  // lua_tolstring:  [-0, +0, m]
  //
  // Converts the Lua value at the given index to a C string.If
  // len is not NULL, it sets *len with the string length.The Lua
  // value must be a string or a number; otherwise, the function
  // returns NULL.If the value is a number, then lua_tolstring
  // also changes the actual value in the stack to a string
  // .(This change confuses lua_next when lua_tolstring is ap-
  // plied to keys during a table traversal.)
  //
  // lua_tolstring returns a pointer to a string inside the Lua
  // state.This string always has a zero( '\0' ) after its last
  // character( as in C ), but can contain other zeros in its
  // body.
  //
  // Because Lua has garbage collection, there is no guarantee
  // that the pointer returned by lua_tolstring will be valid
  // after the corresponding Lua value is removed from the stack.
  size_t      len = 0;
  char const* p   = ::lua_tolstring( L, idx, &len );
  if( p == nullptr ) return nothing;
  DCHECK( int( len ) >= 0 );
  // Use the (pointer, size) constructor because we need to
  // specify the length, 1) so that std::string can pre-allocate,
  // and 2) because there may be zeroes inside the string before
  // the final null terminater.
  return string( p, len );
}

base::maybe<void*> c_api::get( int idx, void** ) const noexcept {
  validate_index( idx );
  void* p = ::lua_touserdata( L, idx );
  // Not sure if this check is needed.
  CHECK( p );
  return p;
}

base::maybe<char const*> c_api::get(
    int idx, char const** ) const noexcept {
  validate_index( idx );
  // We are susceptible to this because of the char const*.
  CHECK( !lua_isstring( L, idx ) );
  CHECK( lua_islightuserdata( L, idx ),
         "index {} is not a light userdata.", idx );
  auto* p =
      static_cast<const char*>( ::lua_touserdata( L, idx ) );
  // Not sure if this check is needed.
  CHECK( p );
  return p;
}

e_lua_type c_api::lua_type_to_enum( int type ) const noexcept {
  CHECK( type != LUA_TNONE, "type ({}) not valid.", type );
  CHECK( type >= 0 );
  CHECK( type < kNumLuaTypes,
         "a new lua type may have been added." );
  return static_cast<e_lua_type>( type );
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
e_lua_type c_api::type_of( int idx ) const noexcept {
  validate_index( idx );
  int res = ::lua_type( L, idx );
  CHECK( res != LUA_TNONE, "index ({}) not valid.", idx );
  return lua_type_to_enum( res );
}

char const* c_api::type_name( e_lua_type type ) const noexcept {
  return ::lua_typename( L, static_cast<int>( type ) );
}

/****************************************************************
** Error checking helpers.
*****************************************************************/
void c_api::enforce_stack_size_ge( int s ) const noexcept {
  CHECK( s >= 0 );
  if( stack_size() >= s ) return;
  FATAL(
      "stack size expected to have size at least {} but "
      "instead found {}.",
      s, stack_size() );
}

lua_valid c_api::enforce_type_of(
    int idx, e_lua_type type ) const noexcept {
  validate_index( idx );
  if( type_of( idx ) == type ) return base::valid;
  return "type of element at index " + to_string( idx ) +
         " expected to be " + string( type_name( type ) ) +
         ", but instead is " +
         string( type_name( type_of( idx ) ) );
}

lua_error_t c_api::pop_and_return_error() noexcept {
  enforce_stack_size_ge( 1 );
  lua_error_t res( lua_tostring( L, -1 ) );
  pop();
  return res;
}

void c_api::validate_index( int idx ) const noexcept {
  enforce_stack_size_ge( abs( idx ) );
}

} // namespace luapp
