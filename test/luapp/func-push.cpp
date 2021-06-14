/****************************************************************
**func-push.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: Unit tests for the src/luapp/func-push.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/func-push.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace lua {
namespace {

using namespace std;

using ::base::valid;

/****************************************************************
** Function props.
*****************************************************************/
namespace func {

struct A {};

int luac_stateless_foo( lua_State* ) { return 0; }
int not_luac_foo( lua_State& ) { return 0; }

// lua-c-extension-function.
struct StatelessLCEF {
  static int fun( lua_State* );
};
struct LCEF {
  int operator()( lua_State* ) const;
  int x;
};
struct NonLCEF {
  int operator()( lua_State*, int ) const;
  int x;
};

auto c_ext_lambda = []( lua_State* ) -> int { return 0; };
auto c_ext_lambda_capture = [x = 1.0]( lua_State* ) -> int {
  (void)x;
  return 0;
};

} // namespace func

// Use the above functions so that the compiler does not emit
// warnings about them not being used.
TEST_CASE( "[lfunction] use funcs" ) {
  func::c_ext_lambda( nullptr );
  func::c_ext_lambda_capture( nullptr );
  func::luac_stateless_foo( nullptr );
  int x = 0;
  func::not_luac_foo( *(lua_State*)&x );
}

/****************************************************************
** LuaCExtensionFunction
*****************************************************************/
static_assert(
    LuaCExtensionFunction<decltype( func::c_ext_lambda )> );
static_assert( LuaCExtensionFunction<
               decltype( func::c_ext_lambda_capture )> );

static_assert( !LuaCExtensionFunction<int> );
static_assert( !LuaCExtensionFunction<int ( * )()> );
static_assert(
    !LuaCExtensionFunction<void ( * )( lua_State* )> );
static_assert(
    !LuaCExtensionFunction<char ( * )( lua_State* )> );
static_assert( !LuaCExtensionFunction<int( void* )> );
static_assert( !LuaCExtensionFunction<int( lua_State const* )> );

static_assert( LuaCExtensionFunction<int ( * )( lua_State* )> );
static_assert( LuaCExtensionFunction<int ( & )( lua_State* )> );
static_assert( LuaCExtensionFunction<int( lua_State* ) const> );
static_assert( LuaCExtensionFunction<int( lua_State* ) const&> );
static_assert(
    LuaCExtensionFunction<int( lua_State* ) const&&> );
static_assert( LuaCExtensionFunction<int( lua_State* ) &> );
static_assert( LuaCExtensionFunction<int( lua_State* ) &&> );

static_assert( LuaCExtensionFunction<decltype( func::LCEF{} )> );
static_assert(
    !LuaCExtensionFunction<decltype( func::NonLCEF{} )> );
static_assert( LuaCExtensionFunction<
               decltype( func::StatelessLCEF::fun )> );

static_assert( LuaCExtensionFunction<
               decltype( func::luac_stateless_foo )> );
static_assert(
    !LuaCExtensionFunction<decltype( func::not_luac_foo )> );

/****************************************************************
** {Stateless,Stateful}LuaCExtensionFunction
*****************************************************************/
static_assert( StatelessLuaCExtensionFunction<
               decltype( func::c_ext_lambda )> );
static_assert( !StatelessLuaCExtensionFunction<
               decltype( func::c_ext_lambda_capture )> );

static_assert( !StatelessLuaCExtensionFunction<int> );
static_assert( !StatelessLuaCExtensionFunction<int ( * )()> );
static_assert(
    !StatelessLuaCExtensionFunction<void ( * )( lua_State* )> );
static_assert(
    !StatelessLuaCExtensionFunction<char ( * )( lua_State* )> );
static_assert( !StatelessLuaCExtensionFunction<int( void* )> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State const* )> );

static_assert(
    StatelessLuaCExtensionFunction<int ( * )( lua_State* )> );
static_assert(
    StatelessLuaCExtensionFunction<int ( & )( lua_State* )> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) const> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) const&> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) const&&> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) &> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) &&> );

static_assert(
    !StatelessLuaCExtensionFunction<decltype( func::LCEF{} )> );
static_assert( !StatelessLuaCExtensionFunction<
               decltype( func::NonLCEF{} )> );
static_assert( StatelessLuaCExtensionFunction<
               decltype( func::StatelessLCEF::fun )> );

static_assert( StatelessLuaCExtensionFunction<
               decltype( func::luac_stateless_foo )> );
static_assert( !StatelessLuaCExtensionFunction<
               decltype( func::not_luac_foo )> );

static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::c_ext_lambda )> );
static_assert( StatefulLuaCExtensionFunction<
               decltype( func::c_ext_lambda_capture )> );

static_assert( !StatefulLuaCExtensionFunction<int> );
static_assert( !StatefulLuaCExtensionFunction<int ( * )()> );
static_assert(
    !StatefulLuaCExtensionFunction<void ( * )( lua_State* )> );
static_assert(
    !StatefulLuaCExtensionFunction<char ( * )( lua_State* )> );
static_assert( !StatefulLuaCExtensionFunction<int( void* )> );
static_assert(
    !StatefulLuaCExtensionFunction<int( lua_State const* )> );

static_assert(
    !StatefulLuaCExtensionFunction<int ( * )( lua_State* )> );
static_assert(
    !StatefulLuaCExtensionFunction<int ( & )( lua_State* )> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) const> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) const&> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) const&&> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) &> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) &&> );
static_assert( !StatefulLuaCExtensionFunction<int( int* ) &&> );

static_assert(
    StatefulLuaCExtensionFunction<decltype( func::LCEF{} )> );
static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::NonLCEF{} )> );
static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::StatelessLCEF::fun )> );

static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::luac_stateless_foo )> );
static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::not_luac_foo )> );

/****************************************************************
** push for functions.
*****************************************************************/
LUA_TEST_CASE( "[func-push] stateless lua C function" ) {
  C.openlibs();

  auto f = []( lua_State* L ) -> int {
    c_api C( L );
    UNWRAP_CHECK( n, C.get<int>( 1 ) );
    C.push( n + 1 );
    return 1;
  };
  push( L, f );
  C.setglobal( "add_one" );

  SECTION( "once" ) {
    REQUIRE( C.dostring( "assert( add_one( 6 ) == 7 )" ) ==
             valid );
  }
  SECTION( "twice" ) {
    REQUIRE( C.dostring( "assert( add_one( 6 ) == 7 )" ) ==
             valid );
  }

  // Make sure that it has no upvalues.
  C.getglobal( "add_one" );
  REQUIRE( C.type_of( -1 ) == type::function );
  REQUIRE_FALSE( C.getupvalue( -1, 1 ) );
  C.pop();
}

// LUA_TEST_CASE( "[func-push] can push stateful lua C extension
// function" ) {
//   int  x  = 1;
//   auto f = [x]( lua_State* ) -> int { return x; };
//
//   push( L, f );
// }

} // namespace
} // namespace lua
