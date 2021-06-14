/****************************************************************
**helper.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: Unit tests for the src/luapp/helper.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/helper.hpp"

// Testing
#include "test/luapp/common.hpp"
#include "test/monitoring-types.hpp"

// luapp
#include "src/luapp/c-api.hpp"

// Lua
#include "lauxlib.h"
#include "lua.h"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::base::valid;
using ::testing::monitoring_types::Tracker;

LUA_TEST_CASE(
    "[helper] push_function, cpp function has upvalue" ) {
  helper h( L );
  C.openlibs();

  h.push_function( []( int n, string const& s,
                       double d ) -> string {
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  } );
  C.setglobal( "go" );

  // Make sure that it has no upvalues.
  C.getglobal( "go" );
  REQUIRE( C.type_of( -1 ) == type::function );
  REQUIRE_FALSE( C.getupvalue( -1, 2 ) );
  REQUIRE( C.getupvalue( -1, 1 ) == true );
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( C.getfield( -1, "__name" ) == type::string );
  REQUIRE( C.stack_size() == 4 );
  // Don't test the string name, because it's long and ugly.
  C.pop( 4 );
}

LUA_TEST_CASE(
    "[helper] push_function, cpp function, trivial" ) {
  helper h( L );

  bool called = false;

  h.push_function( [&] { called = !called; } );
  C.setglobal( "go" );
  REQUIRE_FALSE( called );

  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE_FALSE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE_FALSE( called );
}

LUA_TEST_CASE(
    "[helper] push_function, cpp function, simple/bool" ) {
  helper h( L );

  bool called_with = false;

  h.push_function( [&]( bool b ) { called_with = b; } );
  C.setglobal( "go" );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( false )" ) == valid );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( false )" ) == valid );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( true )" ) == valid );
  REQUIRE( called_with );

  REQUIRE( C.dostring( "go( 'hello' )" ) == valid );
  REQUIRE( called_with );

  REQUIRE( C.dostring( "go( nil )" ) == valid );
  REQUIRE_FALSE( called_with );
}

LUA_TEST_CASE(
    "[helper] push_function, cpp function, calling" ) {
  helper h( L );
  C.openlibs();

  h.push_function( []( int n, string const& s,
                       double d ) -> string {
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  } );
  C.setglobal( "go" );

  SECTION( "successful call" ) {
    REQUIRE( C.dostring( R"(
      local result =
        go( 6, 'hello this is a very long string', 3.5 )
      local expected =
        "args: n=6, s='hello this is a very long string', d=3.5"
      local err = tostring( result ) .. ' not equal to "' ..
                  tostring( expected ) .. '".'
      assert( result == expected, err )
    )" ) == valid );
  }

  SECTION( "too few args" ) {
    // clang-format off
    char const* err =
      "C++ function expected 3 arguments, but received 2 from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on

    REQUIRE( C.dostring( R"(
      go( 6, 'hello this is a very long string' )
    )" ) == lua_invalid( err ) );
  }

  SECTION( "too many args" ) {
    // clang-format off
    char const* err =
      "C++ function expected 3 arguments, but received 4 from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on

    REQUIRE( C.dostring( R"(
      go( 6, 'hello this is a very long string', 3.5, true )
    )" ) == lua_invalid( err ) );
  }

  SECTION( "wrong arg type" ) {
    // clang-format off
    char const* err =
      "C++ function expected type 'double' for argument 3 "
        "(1-based), but received non-convertible type 'string' "
        "from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on
    REQUIRE( C.dostring( R"(
      go( 6, 'hello this is a very long string', 'world' )
    )" ) == lua_invalid( err ) );
  }

  SECTION( "convertible arg types" ) {
    REQUIRE( C.dostring( R"(
      local result = go( '6', 1.23, '3.5' )
      local expected = "args: n=6, s='1.23', d=3.5"
      local err = tostring( result ) .. ' not equal to "' ..
                  tostring( expected ) .. '".'
      assert( result == expected, err )
    )" ) == valid );
  }
}

LUA_TEST_CASE( "[helper] call/pcall" ) {
  helper h( L );
  C.openlibs();

  REQUIRE( C.dostring( R"(
    function foo( n, s, d )
      assert( n ~= nil, 'n is nil' )
      assert( s ~= nil, 's is nil' )
      assert( d ~= nil, 'd is nil' )
      local fmt = string.format
      return fmt( "args: n=%s, s='%s', d=%s", n, s, d )
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "call" ) {
    REQUIRE( h.call( 3, "hello", 3.5 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.5" );
    C.pop();
  }

  SECTION( "pcall" ) {
    REQUIRE( h.pcall( 3, "hello", 3.5 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.5" );
    C.pop();
  }

  SECTION( "pcall with error" ) {
    // clang-format off
    char const* err =
      "[string \"...\"]:4: s is nil\n"
      "stack traceback:\n"
      "\t[C]: in function 'assert'\n"
      "\t[string \"...\"]:4: in function 'foo'";
    // clang-format on

    REQUIRE( h.pcall( 3, nil, 3.5 ) ==
             lua_unexpected<int>( err ) );
  }
}

LUA_TEST_CASE( "[helper] call/pcall multret" ) {
  helper h( L );
  C.openlibs();

  REQUIRE( C.dostring( R"(
    function foo( n, s, d )
      return n+1, s .. '!', d+1.5
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "call" ) {
    REQUIRE( h.call( 3, "hello", 3.5 ) == 3 );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<int>( -3 ) == 4 );
    REQUIRE( C.get<string>( -2 ) == "hello!" );
    REQUIRE( C.get<int>( -1 ) == 5.0 );
    C.pop( 3 );
  }

  SECTION( "pcall" ) {
    REQUIRE( h.pcall( 3, "hello", 3.5 ) == 3 );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<int>( -3 ) == 4 );
    REQUIRE( C.get<string>( -2 ) == "hello!" );
    REQUIRE( C.get<int>( -1 ) == 5.0 );
    C.pop( 3 );
  }
}

LUA_TEST_CASE( "[helper] cpp from cpp via lua" ) {
  helper h( L );
  C.openlibs();

  h.push_function( []( int n, string const& s,
                       double d ) -> string {
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  } );
  C.setglobal( "go" );
  REQUIRE( C.stack_size() == 0 );

  C.getglobal( "go" );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( h.call( 3, "hello", 3.6 ) == 1 );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.get<string>( -1 ) ==
           "args: n=3, s='hello', d=3.6" );
  C.pop();
}

LUA_TEST_CASE( "[helper] cpp->lua->cpp round trip" ) {
  helper h( L );
  C.openlibs();

  h.push_function( [&]( int n, string const& s,
                        double d ) -> string {
    if( n == 4 ) C.error( "n cannot be 4." );
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  } );
  C.setglobal( "go" );

  REQUIRE( C.dostring( R"(
    function foo( n, s, d )
      assert( n ~= nil, 'n is nil' )
      assert( s ~= nil, 's is nil' )
      assert( d ~= nil, 'd is nil' )
      return go( n, s, d )
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "call" ) {
    REQUIRE( h.call( 3, "hello", 3.6 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.6" );
    C.pop();
  }

  SECTION( "call again" ) {
    REQUIRE( h.call( 3, "hello", 3.6 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.6" );
    C.pop();
  }

  SECTION( "pcall" ) {
    REQUIRE( h.pcall( 3, "hello", 3.6 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.6" );
    C.pop();
  }

  SECTION( "pcall with error" ) {
    // clang-format off
    char const* err =
      "[string \"...\"]:4: s is nil\n"
      "stack traceback:\n"
      "\t[C]: in function 'assert'\n"
      "\t[string \"...\"]:4: in function 'foo'";
    // clang-format on

    REQUIRE( h.pcall( 3, nil, 3.6 ) ==
             lua_unexpected<int>( err ) );
  }

  SECTION( "pcall with from C function" ) {
    // clang-format off
    char const* err =
      "[string \"...\"]:6: n cannot be 4.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:6: in function 'foo'";
    // clang-format on

    REQUIRE( h.pcall( 4, "hello", 3.6 ) ==
             lua_unexpected<int>( err ) );
  }
}

} // namespace
} // namespace lua