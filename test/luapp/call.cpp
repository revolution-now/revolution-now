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

// Must be last.
#include "test/catch-common.hpp"

namespace lua {
namespace {

using namespace std;

using ::base::maybe;
using ::base::valid;

LUA_TEST_CASE( "[lua-call] no args" ) {
  C.openlibs();

  REQUIRE( C.dostring( R"lua(
    function foo()
      return "hello"
    end
  )lua" ) == valid );

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

  REQUIRE( C.dostring( R"lua(
    function foo( n, s, d )
      assert( n ~= nil, 'n is nil' )
      assert( s ~= nil, 's is nil' )
      assert( d ~= nil, 'd is nil' )
      local fmt = string.format
      return fmt( "args: n=%s, s='%s', d=%s", n, s, d )
    end
  )lua" ) == valid );

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

  REQUIRE( C.dostring( R"lua(
    function foo()
      return 1, 2, 3, 4, 5
    end
  )lua" ) == valid );

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

  REQUIRE( C.dostring( R"lua(
    function foo( n, s, d )
      return n+1, s .. '!', d+1.5
    end
  )lua" ) == valid );

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
    REQUIRE( C.dostring( R"lua(
      function foo( n, s, d )
        return {n+1, s .. '!', d+1.5}
      end
    )lua" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    any a = call_lua_unsafe_and_get<any>( L, 3, "hello", 3.5 );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( type_of( a ) == type::table );
    table t = as<table>( a );
    REQUIRE( t[1] == 4 );
    REQUIRE( t[2] == "hello!" );
    REQUIRE( t[3] == 5.0 );
  }

  SECTION( "call limit one result" ) {
    REQUIRE( C.dostring( R"lua(
      function foo( n, s, d )
        return n+1, s .. '!', d+1.5
      end
    )lua" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    int res = call_lua_unsafe_and_get<int>( L, 3, "hello", 3.5 );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( res == 4 );
  }

  SECTION( "pcall" ) {
    REQUIRE( C.dostring( R"lua(
      function foo( n, s, d )
        assert( n ~= 9 )
        return {n+1, s .. '!', d+1.5}
      end
    )lua" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    lua_expect<any> a =
        call_lua_safe_and_get<any>( L, 3, "hello", 3.5 );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( a.has_value() );
    REQUIRE( type_of( *a ) == type::table );
    table t = as<table>( *a );
    REQUIRE( t[1] == 4 );
    REQUIRE( t[2] == "hello!" );
    REQUIRE( t[3] == 5.0 );
  }

  SECTION( "pcall with error" ) {
    REQUIRE( C.dostring( R"lua(
      function foo( n, s, d )
        assert( n ~= 9 )
      end
    )lua" ) == valid );

    C.getglobal( "foo" );
    REQUIRE( C.stack_size() == 1 );

    lua_expect<any> a =
        call_lua_safe_and_get<any>( L, 9, "hello", 3.5 );
    REQUIRE( a ==
             lua_unexpected<any>(
                 "[string \"...\"]:3: assertion failed!\n"
                 "stack traceback:\n"
                 "\t[C]: in function 'assert'\n"
                 "\t[string \"...\"]:3: in function 'foo'" ) );
  }

  SECTION( "call with maybe result" ) {
    REQUIRE( C.dostring( R"lua(
      function foo( s )
        return 'hello' .. s
      end
    )lua" ) == valid );

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
    REQUIRE( C.dostring( R"lua(
      function foo( n, s, d )
        res = {n+1, s .. '!', d+1.5}
        -- These results should be ignored.
        return 1, 2, 3
      end
    )lua" ) == valid );

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
    REQUIRE( C.dostring( R"lua(
      function foo( n, s, d )
        assert( n ~= 9 )
        res = {n+1, s .. '!', d+1.5}
        -- Should be ignored
        return 1
      end
    )lua" ) == valid );

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
    REQUIRE( C.dostring( R"lua(
      function foo( n, s, d )
        assert( n ~= 9 )
      end
    )lua" ) == valid );

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

LUA_TEST_CASE( "[lua-call] call_lua_resume_safe" ) {
  C.openlibs();
  st.script.run( R"lua(
  function f( n, s )
    n = coroutine.yield( s .. tostring( n ) )
    local _<close> = setmetatable( {}, {
      __close = function()
          f_is_closed = true
        end
    } )
    s = coroutine.yield( s .. tostring( n ) )
    coroutine.yield( s .. tostring( n ) )
    return n*3
  end
  )lua" );
  REQUIRE( C.stack_size() == 0 );
  cthread L2 = C.newthread();
  c_api   C2( L2 );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  REQUIRE( C2.stack_size() == 0 );
  C2.getglobal( "f" );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( st["f_is_closed"] == nil );
  resume_result expected{
      .status   = resume_status::yield,
      .nresults = 1,
  };
  REQUIRE( call_lua_resume_safe( L2, 5, "hello" ) == expected );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( C2.type_of( -1 ) == type::string );
  REQUIRE( C2.get<string>( -1 ) == "hello5" );
  C2.pop();
  expected = resume_result{ .status   = resume_status::yield,
                            .nresults = 1 };
  REQUIRE( call_lua_resume_safe( L2, 6 ) == expected );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( C2.type_of( -1 ) == type::string );
  REQUIRE( C2.get<string>( -1 ) == "hello6" );
  C2.pop();
  expected = resume_result{ .status   = resume_status::yield,
                            .nresults = 1 };
  REQUIRE( call_lua_resume_safe( L2, "world" ) == expected );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( C2.type_of( -1 ) == type::string );
  REQUIRE( C2.get<string>( -1 ) == "world6" );
  C2.pop();
  REQUIRE( st["f_is_closed"] == nil );
  expected = resume_result{ .status   = resume_status::ok,
                            .nresults = 1 };
  REQUIRE( call_lua_resume_safe( L2 ) == expected );
  // The coro_status thinks that we're suspended because we
  // haven't yet popped the result off of the stack (that's a
  // quirk of the implementation that was taken from Lua and that
  // is not really expecting to be called from C++ I think.
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( st["f_is_closed"] == true );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( C2.type_of( -1 ) == type::number );
  REQUIRE( C2.get<int>( -1 ) == 18 );
  C2.pop();
  // Now it's considered dead after popping.
  REQUIRE( C2.coro_status() == coroutine_status::dead );

  REQUIRE( C.stack_size() == 1 );
  C.pop(); // new thread
}

LUA_TEST_CASE( "[lua-call] call_lua_resume_safe w/ error" ) {
  C.openlibs();
  st.script.run( R"lua(
  function f( n, s )
    n = coroutine.yield( s .. tostring( n ) )
    local _<close> = setmetatable( {}, {
      __close = function()
          f_is_closed = true
        end
    } )
    error( 'some error' )
  end
  )lua" );
  REQUIRE( C.stack_size() == 0 );
  cthread L2 = C.newthread();
  c_api   C2( L2 );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  REQUIRE( C2.stack_size() == 0 );
  C2.getglobal( "f" );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( st["f_is_closed"] == nil );
  resume_result expected{
      .status   = resume_status::yield,
      .nresults = 1,
  };
  REQUIRE( call_lua_resume_safe( L2, 5, "hello" ) == expected );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( C2.type_of( -1 ) == type::string );
  REQUIRE( C2.get<string>( -1 ) == "hello5" );
  C2.pop();
  REQUIRE( st["f_is_closed"] == nil );
  REQUIRE( call_lua_resume_safe( L2, 6 ) ==
           lua_unexpected<resume_result>(
               "[string \"...\"]:9: some error" ) );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  REQUIRE( st["f_is_closed"] == true );
  // Error object is still on the stack.
  REQUIRE( C2.stack_size() == 1 );
  C2.pop();
  REQUIRE( C2.status() == thread_status::err );

  // Ensure that we can push things once again to the L2 stack.
  C2.push( 5 );
  REQUIRE( C2.stack_size() == 1 );
  C2.pop();

  REQUIRE( C.stack_size() == 1 );
  C.pop(); // new thread
}

LUA_TEST_CASE( "[lua-call] call_lua_resume_safe_and_get" ) {
  C.openlibs();
  st.script.run( R"lua(
  function f( n, s )
    n = coroutine.yield( s .. tostring( n ) )
    local _<close> = setmetatable( {}, {
      __close = function()
          f_is_closed = true
        end
    } )
    s = coroutine.yield( s .. tostring( n ) )
    coroutine.yield( s .. tostring( n ) )
    return n*3
  end
  )lua" );
  REQUIRE( C.stack_size() == 0 );
  cthread L2 = C.newthread();
  c_api   C2( L2 );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  REQUIRE( C2.stack_size() == 0 );
  C2.getglobal( "f" );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( st["f_is_closed"] == nil );
  resume_result_with_value<string> expected1{
      .status = resume_status::yield,
      .value  = "hello5",
  };
  REQUIRE( call_lua_resume_safe_and_get<string>(
               L2, 5, "hello" ) == expected1 );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 0 );
  resume_result_with_value<string> expected2{
      .status = resume_status::yield, .value = "hello6" };
  REQUIRE( call_lua_resume_safe_and_get<string>( L2, 6 ) ==
           expected2 );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 0 );
  resume_result_with_value<string> expected3{
      .status = resume_status::yield, .value = "world6" };
  REQUIRE( call_lua_resume_safe_and_get<string>( L2, "world" ) ==
           expected3 );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 0 );
  REQUIRE( st["f_is_closed"] == nil );
  resume_result_with_value<int> expected4{
      .status = resume_status::ok, .value = 18 };
  REQUIRE( call_lua_resume_safe_and_get<int>( L2 ) ==
           expected4 );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  REQUIRE( st["f_is_closed"] == true );
  REQUIRE( C2.stack_size() == 0 );

  REQUIRE( C.stack_size() == 1 );
  C.pop(); // new thread
}

LUA_TEST_CASE(
    "[lua-call] call_lua_resume_safe_and_get w/ error" ) {
  C.openlibs();
  st.script.run( R"lua(
  function f( n, s )
    n = coroutine.yield( s .. tostring( n ) )
    local _<close> = setmetatable( {}, {
      __close = function()
          f_is_closed = true
        end
    } )
    error( 'some error' )
  end
  )lua" );
  REQUIRE( C.stack_size() == 0 );
  cthread L2 = C.newthread();
  c_api   C2( L2 );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  REQUIRE( C2.stack_size() == 0 );
  C2.getglobal( "f" );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( st["f_is_closed"] == nil );
  resume_result_with_value<string> expected1{
      .status = resume_status::yield,
      .value  = "hello5",
  };
  REQUIRE( call_lua_resume_safe_and_get<string>(
               L2, 5, "hello" ) == expected1 );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 0 );
  REQUIRE( st["f_is_closed"] == nil );
  REQUIRE( call_lua_resume_safe_and_get<string>( L2, 6 ) ==
           lua_unexpected<resume_result_with_value<string>>(
               "[string \"...\"]:9: some error" ) );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  REQUIRE( st["f_is_closed"] == true );
  // Error object is still on the stack.
  REQUIRE( C2.stack_size() == 1 );
  C2.pop();
  REQUIRE( C2.status() == thread_status::err );

  // Ensure that we can push things once again to the L2 stack.
  C2.push( 5 );
  REQUIRE( C2.stack_size() == 1 );
  C2.pop();

  // Let's try to run another coroutine on it.
  st.script.run( R"lua(
  function f( n )
    return coroutine.yield( n+1 )
  end
  )lua" );
  C2.getglobal( "f" );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( call_lua_resume_safe_and_get<int>( L2, 5 ) ==
           lua_unexpected<resume_result_with_value<int>>(
               "cannot resume dead coroutine" ) );
  REQUIRE( C2.coro_status() == coroutine_status::dead );

  REQUIRE( C.stack_size() == 1 );
  C.pop(); // new thread
}

LUA_TEST_CASE(
    "[lua-call] call_lua_resume_safe_and_get w/ conversion "
    "error" ) {
  C.openlibs();
  st.script.run( R"lua(
  function f()
    coroutine.yield( "hello" )
  end
  )lua" );
  REQUIRE( C.stack_size() == 0 );
  cthread L2 = C.newthread();
  c_api   C2( L2 );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  REQUIRE( C2.stack_size() == 0 );
  C2.getglobal( "f" );
  REQUIRE( C2.coro_status() == coroutine_status::suspended );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE(
      call_lua_resume_safe_and_get<int>( L2 ) ==
      lua_unexpected<resume_result_with_value<int>>(
          "native code expected type `int' as a return value "
          "(which requires 1 Lua value), but the values "
          "returned by Lua were not convertible to that native "
          "type.  The Lua values received were: [string]." ) );
  REQUIRE( C2.coro_status() == coroutine_status::dead );
  // No error object on the stack.
  REQUIRE( C2.stack_size() == 0 );
  REQUIRE( C2.status() == thread_status::ok );

  REQUIRE( C.stack_size() == 1 );
  C.pop(); // new thread
}

} // namespace
} // namespace lua
