/****************************************************************
**call.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: Unit tests for the src/luapp/call.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/call.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::base::valid;

LUA_TEST_CASE( "[func-call] no args" ) {
  C.openlibs();

  REQUIRE( C.dostring( R"(
    function foo()
      return "hello"
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "call" ) {
    REQUIRE( call_lua_unsafe( L ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    C.pop();
  }

  SECTION( "call with args" ) {
    REQUIRE( call_lua_unsafe( L, 1, 2, 3 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    C.pop();
  }

  SECTION( "pcall" ) {
    REQUIRE( call_lua_safe( L ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    C.pop();
  }
}

LUA_TEST_CASE( "[func-call] multiple args, one result" ) {
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
    REQUIRE( call_lua_unsafe( L, 3, "hello", 3.5 ) == 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) ==
             "args: n=3, s='hello', d=3.5" );
    C.pop();
  }

  SECTION( "pcall" ) {
    REQUIRE( call_lua_safe( L, 3, "hello", 3.5 ) == 1 );
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

    REQUIRE( call_lua_safe( L, 3, nil, 3.5 ) ==
             lua_unexpected<int>( err ) );
  }
}

LUA_TEST_CASE( "[func-call] with nresults" ) {
  C.openlibs();

  REQUIRE( C.dostring( R"(
    function foo()
      return 1, 2, 3, 4, 5
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "multret/unsafe" ) {
    REQUIRE( call_lua_unsafe( L ) == 5 );
    REQUIRE( C.stack_size() == 5 );
    REQUIRE( C.get<int>( -1 ) == 5 );
    REQUIRE( C.get<int>( -2 ) == 4 );
    REQUIRE( C.get<int>( -3 ) == 3 );
    REQUIRE( C.get<int>( -4 ) == 2 );
    REQUIRE( C.get<int>( -5 ) == 1 );
    C.pop( 5 );
  }

  SECTION( "request 5/safe" ) {
    REQUIRE( call_lua_safe_nresults( L, /*nresults=*/5 ) ==
             valid );
    REQUIRE( C.stack_size() == 5 );
    REQUIRE( C.get<int>( -1 ) == 5 );
    REQUIRE( C.get<int>( -2 ) == 4 );
    REQUIRE( C.get<int>( -3 ) == 3 );
    REQUIRE( C.get<int>( -4 ) == 2 );
    REQUIRE( C.get<int>( -5 ) == 1 );
    C.pop( 5 );
  }

  SECTION( "request 3/unsafe" ) {
    call_lua_unsafe_nresults( L, /*nresults=*/3 );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<int>( -1 ) == 3 );
    REQUIRE( C.get<int>( -2 ) == 2 );
    REQUIRE( C.get<int>( -3 ) == 1 );
    C.pop( 3 );
  }

  SECTION( "request 1/safe" ) {
    REQUIRE( call_lua_safe_nresults( L, /*nresults=*/1 ) ==
             valid );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<int>( -1 ) == 1 );
    C.pop();
  }

  SECTION( "request 0/unsafe" ) {
    call_lua_unsafe_nresults( L, /*nresults=*/0 );
    REQUIRE( C.stack_size() == 0 );
  }
}

LUA_TEST_CASE( "[func-call] call/pcall multret" ) {
  C.openlibs();

  REQUIRE( C.dostring( R"(
    function foo( n, s, d )
      return n+1, s .. '!', d+1.5
    end
  )" ) == valid );

  C.getglobal( "foo" );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "call" ) {
    REQUIRE( call_lua_unsafe( L, 3, "hello", 3.5 ) == 3 );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<int>( -3 ) == 4 );
    REQUIRE( C.get<string>( -2 ) == "hello!" );
    REQUIRE( C.get<int>( -1 ) == 5.0 );
    C.pop( 3 );
  }

  SECTION( "pcall" ) {
    REQUIRE( call_lua_safe( L, 3, "hello", 3.5 ) == 3 );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<int>( -3 ) == 4 );
    REQUIRE( C.get<string>( -2 ) == "hello!" );
    REQUIRE( C.get<int>( -1 ) == 5.0 );
    C.pop( 3 );
  }
}

} // namespace
} // namespace lua
