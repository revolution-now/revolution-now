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

// luapp
#include "src/luapp/ext-base.hpp"
#include "src/luapp/thing.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::base::maybe;
using ::base::valid;

LUA_TEST_CASE( "[lua-call] no args" ) {
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

LUA_TEST_CASE( "[lua-call] multiple args, one result" ) {
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

LUA_TEST_CASE( "[lua-call] with nresults" ) {
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

LUA_TEST_CASE( "[lua-call] call/pcall multret" ) {
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

LUA_TEST_CASE( "[lua-call] call_lua_{un}safe_and_get" ) {
  C.openlibs();

  SECTION( "call" ) {
    REQUIRE( C.dostring( R"(
      function foo( n, s, d )
        return {n+1, s .. '!', d+1.5}
      end
    )" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    thing th =
        call_lua_unsafe_and_get<thing>( L, 3, "hello", 3.5 );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( th.is<table>() );
    table t = th.as<table>();
    REQUIRE( t[1] == 4 );
    REQUIRE( t[2] == "hello!" );
    REQUIRE( t[3] == 5.0 );
  }

  SECTION( "call limit one result" ) {
    REQUIRE( C.dostring( R"(
      function foo( n, s, d )
        return n+1, s .. '!', d+1.5
      end
    )" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    int res = call_lua_unsafe_and_get<int>( L, 3, "hello", 3.5 );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( res == 4 );
  }

  SECTION( "pcall" ) {
    REQUIRE( C.dostring( R"(
      function foo( n, s, d )
        assert( n ~= 9 )
        return {n+1, s .. '!', d+1.5}
      end
    )" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    lua_expect<thing> th =
        call_lua_safe_and_get<thing>( L, 3, "hello", 3.5 );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( th.has_value() );
    REQUIRE( th->is<table>() );
    table t = th->as<table>();
    REQUIRE( t[1] == 4 );
    REQUIRE( t[2] == "hello!" );
    REQUIRE( t[3] == 5.0 );
  }

  SECTION( "pcall with error" ) {
    REQUIRE( C.dostring( R"(
      function foo( n, s, d )
        assert( n ~= 9 )
      end
    )" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    lua_expect<thing> th =
        call_lua_safe_and_get<thing>( L, 9, "hello", 3.5 );
    REQUIRE( th ==
             lua_unexpected<thing>(
                 "[string \"...\"]:3: assertion failed!\n"
                 "stack traceback:\n"
                 "\t[C]: in function 'assert'\n"
                 "\t[string \"...\"]:3: in function 'foo'" ) );
  }

  SECTION( "call with maybe result" ) {
    REQUIRE( C.dostring( R"(
      function foo( s )
        return 'hello' .. s
      end
    )" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    auto n = call_lua_unsafe_and_get<maybe<int>>( L, "hello" );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( !n.has_value() );

    C.getglobal( "foo" );
    auto s =
        call_lua_unsafe_and_get<maybe<string>>( L, "hello" );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( s == "hellohello" );
  }
}

LUA_TEST_CASE(
    "[lua-call] call_lua_{un}safe_and_get void return" ) {
  C.openlibs();

  SECTION( "call" ) {
    REQUIRE( C.dostring( R"(
      function foo( n, s, d )
        res = {n+1, s .. '!', d+1.5}
        -- These results should be ignored.
        return 1, 2, 3
      end
    )" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    static_assert(
        std::is_same_v<decltype( call_lua_unsafe_and_get<void>(
                           L, 3, "hello", 3.5 ) ),
                       void> );
    call_lua_unsafe_and_get<void>( L, 3, "hello", 3.5 );
    REQUIRE( C.stack_size() == 0 );
    C.getglobal( "res" );
    table t( L, C.ref_registry() );
    REQUIRE( t[1] == 4 );
    REQUIRE( t[2] == "hello!" );
    REQUIRE( t[3] == 5.0 );
  }

  SECTION( "pcall" ) {
    REQUIRE( C.dostring( R"(
      function foo( n, s, d )
        assert( n ~= 9 )
        res = {n+1, s .. '!', d+1.5}
        -- Should be ignored
        return 1
      end
    )" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    lua_valid v = call_lua_safe_and_get( L, 3, "hello", 3.5 );
    REQUIRE( v == valid );
    REQUIRE( C.stack_size() == 0 );

    C.getglobal( "res" );
    table t( L, C.ref_registry() );
    REQUIRE( t[1] == 4 );
    REQUIRE( t[2] == "hello!" );
    REQUIRE( t[3] == 5.0 );
  }

  SECTION( "pcall with error" ) {
    REQUIRE( C.dostring( R"(
      function foo( n, s, d )
        assert( n ~= 9 )
      end
    )" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    lua_valid v =
        call_lua_safe_and_get<void>( L, 9, "hello", 3.5 );
    REQUIRE( v ==
             lua_invalid(
                 "[string \"...\"]:3: assertion failed!\n"
                 "stack traceback:\n"
                 "\t[C]: in function 'assert'\n"
                 "\t[string \"...\"]:3: in function 'foo'" ) );
  }
}

} // namespace
} // namespace lua
